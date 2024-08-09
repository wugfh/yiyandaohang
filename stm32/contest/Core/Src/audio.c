#include <string.h>
#include <stdio.h>
#include "audio.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "hi3861.h"
#include "syn6288.h"

static const osThreadAttr_t audio_task_attr = {
    .name = "maintask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

extern float latitude, longitute, altitude;
extern hi3861_t hi;
extern char ucs_intrution[200];
extern char syn_buffer[500];
extern int ucs_len;
extern syn_t syn;

extern int target_index;

void audio_init(audio_t* au, USART_TypeDef* audio_usart){
    au->usart = audio_usart;
    au->recv_buf.index = 0;
    au->recv_buf.buffer = NULL;
    au->revent_queue=xQueueCreate(RECV_QUEUE_LEN, sizeof(au->recv_buf));
    au->woken = pdFALSE;
    au->qraddr = xQueueCreate(RECV_QUEUE_LEN, sizeof(au->recv_buf.buffer));
    LL_USART_EnableIT_RXNE(au->usart);
    LL_USART_EnableIT_IDLE(au->usart);
    for(int i = 0; i < RECV_QUEUE_LEN; ++i){
        char* addr = pvPortMalloc(AUDIO_BUFFER_LEN);
        xQueueSend(au->qraddr, &addr, 0);
    }

    xQueueReceive(au->qraddr, &au->recv_buf.buffer, 0);

    au->audio_task_thread = osThreadNew(audio_task, au, &audio_task_attr);
    printf("init audio uart:ok\r\n");
}

void audio_callback(audio_t* au){
    if (LL_USART_IsActiveFlag_RXNE(au->usart) && au->recv_buf.buffer != NULL){
		uint8_t tmp = LL_USART_ReceiveData8(au->usart);
        au->recv_buf.buffer[au->recv_buf.index] = tmp;
        ++au->recv_buf.index;
        LL_USART_ClearFlag_RXNE(au->usart);
	}
    else if(LL_USART_IsActiveFlag_IDLE(au->usart)){
        LL_USART_ClearFlag_IDLE(au->usart);
        if(xQueueSendFromISR(au->revent_queue, &au->recv_buf, &au->woken) != pdTRUE){
            printf("audio send msg failed\r\n");
        }
        if(xQueueReceiveFromISR(au->qraddr, &au->recv_buf.buffer, &au->woken) != pdTRUE){
            au->recv_buf.buffer = NULL;
        }
        au->recv_buf.index = 0;
        if(au->woken == pdTRUE) taskYIELD();
        au->woken = pdFALSE;
    }
}

void audio_handle(audio_msg *msg){
    int len = msg->index;
    msg->buffer[len] = '\0';
    for(int i = 0; i < len; ++i){
        printf("%2x ", msg->buffer[i]);
    }
    printf("\r\n");
    if(msg->buffer[0] == 0x02 && msg->buffer[1] == 0x01){
        hi3861_send_target_pos(&hi, latitude, longitute, altitude);
    }
    else if(msg->buffer[0] == 0x03){
        target_index = msg->buffer[1]-1;
    }
    else if(msg->buffer[0] == 0x04 && msg->buffer[1] == 0x01){
        printf("go:%s\r\n", syn_buffer);
        syn6288_send(&syn, ucs_intrution, ucs_len);
    }
}

void audio_task(void* args){
    audio_t* au = (audio_t*)args;
    audio_msg msg;
    while(1){
        while(xQueueReceive(au->revent_queue, &msg, 0) != pdTRUE)
            osDelay(100/portTICK_PERIOD_MS);
        audio_handle(&msg);
        xQueueSend(au->qraddr, &msg.buffer, 0);
        osDelay(1);
    }
}

void audio_attach_header(audio_msg* msg){
    msg->buffer[0] = 0xAA;
    msg->buffer[1] = 0x55;
    msg->index += 2;
}

void audio_attach_pos(audio_msg* msg, double latitude, double longitude, double altitude){
    memcpy(msg->buffer+msg->index, &latitude, sizeof(latitude));
    msg->index += sizeof(latitude);
    memcpy(msg->buffer+msg->index, &longitude, sizeof(longitude));
    msg->index += sizeof(longitude);
    memcpy(msg->buffer+msg->index, &altitude, sizeof(altitude));
    msg->index += sizeof(altitude);
}

void audio_attach_how(audio_msg* msg, int dir, int dis, int nextdir){
    memcpy(msg->buffer+msg->index, &dir, sizeof(dir));
    msg->index += sizeof(dir);
    memcpy(msg->buffer+msg->index, &dis, sizeof(dis));
    msg->index += sizeof(dis);
    memcpy(msg->buffer+msg->index, &nextdir, sizeof(nextdir));
    msg->index += sizeof(nextdir);
}

void audio_attach_tailer(audio_msg* msg){
    msg->buffer[msg->index] = 0x55;
    msg->buffer[msg->index+1] = 0xAA;
    msg->index += 2;
}

void audio_attach_seq(audio_msg* msg, uint8_t seq){
    msg->buffer[msg->index] = seq;
    msg->index += 1;
}

int audio_send_msg(audio_t* au, audio_msg* data){
    for(int i = 0; i < data->index; ++i){
        while(LL_USART_IsActiveFlag_TC(au->usart) == RESET);
        LL_USART_TransmitData8(au->usart, data->buffer[i]);
    }
    while(LL_USART_IsActiveFlag_TC(au->usart) == RESET);
    return 1;
}

int audio_send_pos(audio_t* au, audio_msg* data, double latitude, double longitude, double altitude){
    audio_attach_header(data);
    audio_attach_pos(data, latitude, longitude, altitude);
    audio_attach_tailer(data);
    return audio_send_msg(au, data);
}

int audio_send_how(audio_t* au, audio_msg* data, int dir, int dis, int dura, double latitude, double longitude , double altitude){
    audio_attach_header(data);
    audio_attach_seq(data, 2);
    audio_attach_how(data, dir, dis, dura);
    audio_attach_pos(data, latitude, longitude, altitude);
    audio_attach_tailer(data);
    return audio_send_msg(au, data);
}

int audio_alarm(audio_t* au, audio_msg* data){
    audio_attach_header(data);
    audio_attach_seq(data, 1);
    audio_attach_tailer(data);
    for(int i = 0; i < data->index; ++i){
        printf("%2x ", data->buffer[i]);
    }
    printf("\r\n");
    return audio_send_msg(au, data);
}

