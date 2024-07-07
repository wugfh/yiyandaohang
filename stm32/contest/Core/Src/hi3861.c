#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hi3861.h"
#include "audio.h"

extern audio_t su03_audio;
extern float latitude, longitute, altitude;

static const osThreadAttr_t hi3861_task_attr = {
    .name = "hi3861_task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

void hi3861_msg_handle(hi3861_msg_t* msg){
    int dist = 0, dir = 0, dura = 0;
    char* token = strtok(msg->buffer, ",");
    dist = atoi(token);
    token = strtok(NULL, ",");
    dir = atoi(token);
    token = strtok(NULL, ",");
    dura = atoi(token);
    audio_send_how(&su03_audio, dir, dist, dura, latitude, longitute, altitude);
}

static void hi3861_task(void* args){
    hi3861_t* hi = (hi3861_t*)args;
    hi3861_msg_t msg;
    while(1){
        while(xQueueReceive(hi->msg_queue, &msg, 0) != pdTRUE)
            osDelay(1);
        hi3861_msg_handle(&msg);
        xQueueSend(hi->space_queue, &msg.buffer, 0);
        osDelay(1);
    }
}

void hi3861_init(hi3861_t* hi, USART_TypeDef* usart, int qmsg_len){
    hi->usart = usart;
    hi->msg_queue = xQueueCreate(qmsg_len, sizeof(hi->recv_msg));
    hi->recv_msg.len = 0;
    hi->recv_msg.buffer = NULL;
    hi->woken = pdFALSE;
    LL_USART_EnableIT_IDLE(hi->usart);
    LL_USART_EnableIT_RXNE(hi->usart);
    hi->hi_task = osThreadNew(hi3861_task, hi, &hi3861_task_attr);
    hi->space_queue = xQueueCreate(qmsg_len, sizeof(hi->recv_msg.buffer));
    for(int i = 0; i < qmsg_len; ++i){
        char* addr = pvPortMalloc(MSG_BUFFER_LEN);
        xQueueSend(hi->space_queue, &addr, 0);
    }
    xQueueReceive(hi->space_queue, &hi->recv_msg.buffer, 0);
}

void hi3861_callback(hi3861_t* hi){
    if(LL_USART_IsActiveFlag_RXNE(hi->usart) && hi->recv_msg.buffer != NULL){
        uint8_t tmp = LL_USART_ReceiveData8(hi->usart);
        hi->recv_msg.buffer[hi->recv_msg.len] = tmp;
        ++hi->recv_msg.len;
    }
    if(LL_USART_IsActiveFlag_IDLE(hi->usart)){
        uint8_t clear = hi->usart->SR;
        clear = hi->usart->DR;
        hi->recv_msg.buffer[hi->recv_msg.len] = '\0';
        if(xQueueSendFromISR(hi->msg_queue, &hi->recv_msg, &hi->woken) != pdTRUE){
            printf("send msg failed\r\n");
        }
        if(xQueueReceiveFromISR(hi->space_queue, &hi->recv_msg.buffer, &hi->woken) != pdTRUE){
            hi->recv_msg.buffer = NULL;
        }
        hi->recv_msg.len = 0;
        if(hi->woken == pdTRUE) taskYIELD();
        hi->woken = pdFALSE;
    }
}


int hi3861_send_msg(hi3861_t* hi, uint8_t* data, int len){
    if(data == NULL){
        printf("error occur in hi3861 send msg\r\n");
        return 0;
    }
    for(int i = 0; i < len; ++i){
        while(LL_USART_IsActiveFlag_TC(hi->usart) == RESET);
        LL_USART_TransmitData8(hi->usart, data[i]);
    }
    while(LL_USART_IsActiveFlag_TC(hi->usart) == RESET);
    return 1;
}

int hi3861_msg_header(uint8_t* data, int start, int len){
    if(data == NULL || start+2 >= len){
        return 0;
    }
    data[start] = 0x55;
    data[1+start] = 0xAA;
    data[start+2] = len-3;
    return start+3;
}

int hi3861_msg_tailer(uint8_t* data, int start, int len){
    if(data == NULL || start+2 >= len){
        return 0;
    }
    data[start] = 0xAA;
    data[start+1] = 0x55;
    return start+2;
}

int hi3861_msg_pos(uint8_t* data, int start, int len, double latitude, double longitude, double altitude){
    if(data == NULL || start+sizeof(double)*3>=len)
        return 0;
    start += snprintf((char*)data+start, len, "%.4f,%.4f", latitude, longitude);
    return start;
}

int hi3861_send_pos(hi3861_t* hi, double latitude, double longitude, double altitude){
    uint8_t data[50];
    memset(data, 0, sizeof(data));
    int len = 50, index = 0;
    index = hi3861_msg_pos(data, index, len, latitude, longitude, altitude);
    return hi3861_send_msg(hi, data, len);
}

