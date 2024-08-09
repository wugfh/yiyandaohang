#pragma once


#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"

#define SYN6288_DATA_LEN    200
#define SYN6288_HEADER_LEN  3
#define SYN6288_COMMAND_LEN 2
#define SYN6288_CHECK_LEN   1
#define SYN6288_BUFFER_LEN  210

typedef struct syn_msg{
    char* buffer;
    int len;
}syn_msg_t;

typedef struct syn6288{
    USART_TypeDef*  usart;
    syn_msg_t       recv_msg;
    xQueueHandle    msg_queue;
    xQueueHandle    space_queue;
    osThreadId_t    syn_task;
    BaseType_t      woken;
    uint8_t         state;
}syn_t;

void syn_init(syn_t* syn, USART_TypeDef* usart, int qmsg_len);
int syn6288_send(syn_t* syn, char* data, uint16_t len);
void syn_callback(syn_t* syn);
int syn_send_msg(syn_t* syn, syn_msg_t* msg);
int syn_check(syn_t* syn);
