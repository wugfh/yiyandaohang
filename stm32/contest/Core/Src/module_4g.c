#include "module_4g.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hi3861.h"
#include "audio.h"

extern audio_t su03_audio;
extern float latitude, longitute, altitude;

static const osThreadAttr_t m4g_task_attr = {
    .name = "m4g_task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

void m4g_msg_handle(hi3861_msg_t* msg){
    int dist = 0, dir = 0, dura = 0;
    char* token = strtok(msg->buffer, ":");
    char* token = strtok(NULL, ",");
    latitude = atof(token);
    token = strtok(NULL, ",");
    longitute = atof(token);
}

static void m4g_task(void* args){
   m4g_t* hi = (m4g_t*)args;
   m4g_msg_t msg;
    while(1){
        while(xQueueReceive(hi->msg_queue, &msg, 0) != pdTRUE)
            osDelay(1);
        m4g_msg_handle(&msg);
        xQueueSend(hi->space_queue, &msg.buffer, 0);
        osDelay(1);
    }
}

void m4g_init(m4g_t* hi, USART_TypeDef* usart, int qmsg_len){
    hi->usart = usart;
    hi->msg_queue = xQueueCreate(qmsg_len, sizeof(hi->recv_msg));
    hi->recv_msg.len = 0;
    hi->recv_msg.buffer = NULL;
    hi->woken = pdFALSE;
    LL_USART_EnableIT_IDLE(hi->usart);
    LL_USART_EnableIT_RXNE(hi->usart);
    hi->m4g_task = osThreadNew(m4g_task, hi, &m4g_task_attr);
    hi->space_queue = xQueueCreate(qmsg_len, sizeof(hi->recv_msg.buffer));
    for(int i = 0; i < qmsg_len; ++i){
        char* addr = pvPortMalloc(MSG_BUFFER_LEN);
        xQueueSend(hi->space_queue, &addr, 0);
    }
    xQueueReceive(hi->space_queue, &hi->recv_msg.buffer, 0);
}

void m4g_callback(m4g_t* hi){
    if(LL_USART_IsActiveFlag_RXNE(hi->usart) && hi->recv_msg.buffer != NULL){
        uint8_t tmp = LL_USART_ReceiveData8(hi->usart);
        hi->recv_msg.buffer[hi->recv_msg.len] = tmp;
        ++hi->recv_msg.len;
    }
    if(LL_USART_IsActiveFlag_IDLE(hi->usart)){
        uint8_t clear = hi->usart->SR;
        clear = hi->usart->DR;
        hi->recv_msg.buffer[hi->recv_msg.len] = ',';
        hi->recv_msg.buffer[hi->recv_msg.len+1] = '\0';
        ++hi->recv_msg.len;
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


int m4g_send_msg(m4g_t* hi, uint8_t* data, int len){
    if(data == NULL){
        printf("error occur inm4g send msg\r\n");
        return 0;
    }
    for(int i = 0; i < len; ++i){
        while(LL_USART_IsActiveFlag_TC(hi->usart) == RESET);
        LL_USART_TransmitData8(hi->usart, data[i]);
    }
    while(LL_USART_IsActiveFlag_TC(hi->usart) == RESET);
    return 1;
}


int m4g_msg_gps(m4g_t* hi){
    char data[50];
    int len = snprintf(data, 50, "AT+GPSINFO");
    return m4g_send_msg(hi, data, len);
}

int m4g_send_pos(m4g_t* hi, double latitude, double longitude, double altitude){
    uint8_t data[50];
    memset(data, 0, sizeof(data));
    int len = 50, index = 0;
    index =m4g_msg_pos(data, index, len, latitude, longitude, altitude);
    return m4g_send_msg(hi, data, len);
}

