#pragma once


#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"

typedef struct m4g_msg{
    char* buffer;
    int len;
}m4g_msg_t;

typedef struct module_4g{
    USART_TypeDef*  usart;
    m4g_msg_t       recv_msg;
    xQueueHandle    msg_queue;
    xQueueHandle    space_queue;
    osThreadId_t    m4g_task;
    BaseType_t      woken;
}m4g_t;

void m4g_init(m4g_t* hi, USART_TypeDef* usart, int qmsg_len);
int m4g_msg_gps(m4g_t* hi);