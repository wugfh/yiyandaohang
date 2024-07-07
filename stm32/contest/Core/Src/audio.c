#include <string.h>
#include <stdio.h>
#include "audio.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

static const osThreadAttr_t audio_task_attr = {
    .name = "maintask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

void audio_init(audio_t* au, USART_TypeDef* audio_usart){
    // LL_USART_EnableIT_RXNE(au->usart);
    // LL_USART_EnableIT_IDLE(au->usart);
    au->usart = audio_usart;
    au->recv_buf.index = 0;
    au->send_buf.index = 0;
    au->revent_queue=xQueueCreate(RECV_QUEUE_LEN, sizeof(au->recv_buf));
    au->audio_task_thread = osThreadNew(audio_task, au, &audio_task_attr);
    au->woken = pdFALSE;
}

void audio_callback(audio_t* au){
    if (LL_USART_IsActiveFlag_RXNE(au->usart)){
		uint8_t tmp = LL_USART_ReceiveData8(au->usart);
        au->recv_buf.buffer[au->recv_buf.index] = tmp;
        ++au->recv_buf.index;
	}
    else if(LL_USART_IsActiveFlag_IDLE(au->usart)){
        xQueueSendFromISR(au->revent_queue, &au->recv_buf, &au->woken);
        if(au->woken == pdTRUE) taskYIELD();
        au->recv_buf.index = 0;
    }
}

void audio_handle(audio_msg *msg){
    int len = msg->index;
    msg->buffer[len] = 0;
    if(msg->buffer[0] == 0xAA && msg->buffer[1] == 0x55){
        
    }
    else{

    }
}

void audio_task(void* args){
    audio_t* au = (audio_t*)args;
    audio_msg msg;
    while(1){
        while(xQueueReceive(au->revent_queue, &msg, 0) != pdTRUE)
            osDelay(100/portTICK_PERIOD_MS);
        audio_handle(&msg);
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

int audio_send_msg(audio_t* au){
    for(int i = 0; i < au->send_buf.index; ++i){
        while(LL_USART_IsActiveFlag_TC(au->usart) == RESET);
        LL_USART_TransmitData8(au->usart, au->send_buf.buffer[i]);
    }
    while(LL_USART_IsActiveFlag_TC(au->usart) == RESET);
    au->send_buf.index = 0;
    return 1;
}

int audio_send_pos(audio_t* au, double latitude, double longitude, double altitude){
    audio_attach_header(&au->send_buf);
    audio_attach_pos(&au->send_buf, latitude, longitude, altitude);
    audio_attach_tailer(&au->send_buf);
    return audio_send_msg(au);
}

int audio_send_how(audio_t* au, int dir, int dis, int dura, double latitude, double longitude, double altitude){
    audio_attach_header(&au->send_buf);
    audio_attach_seq(&au->send_buf, 2);
    audio_attach_how(&au->send_buf, dir, dis, dura);
    audio_attach_pos(&au->send_buf, latitude, longitude, altitude);
    audio_attach_tailer(&au->send_buf);

    return audio_send_msg(au);
}

