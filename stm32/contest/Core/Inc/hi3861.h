#ifndef __HI3861_H__
#define __HI3861_H__

#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"

#define MSG_BUFFER_LEN      256

typedef struct hi3861_msg{
    char* buffer;
    int len;
}hi3861_msg_t;

typedef struct hi3861{
    USART_TypeDef*  usart;
    hi3861_msg_t    recv_msg;
    xQueueHandle    msg_queue;
    xQueueHandle    space_queue;
    osThreadId_t    hi_task;
    BaseType_t      woken;
}hi3861_t;

void hi3861_callback(hi3861_t* hi);
void hi3861_init(hi3861_t* hi, USART_TypeDef* usart, int qmsg_len);
int hi3861_send_pos(hi3861_t* hi, double latitude, double longitude, double altitude);

#endif

