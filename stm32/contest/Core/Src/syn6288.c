#include "syn6288.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hi3861.h"
#include "audio.h"

enum syn_back_status{
    SYN_INIT_SUCC       = 0x4A,
    SYN_RECV_SUCC       = 0x41,
    SYN_RECV_FAIL       = 0x45,
    SYN_BROADCASTING    = 0x4E,                
    SYN_IDLE            = 0x4F,
    SYN_UNKNOW          = 0x00
};

enum syn_cmd{
    SYN_BROADCAST       = 0x01,
    SYN_SET_BAUDRATE    = 0x31,
    SYN_STOP            = 0x02,
    SYN_SUSPEND         = 0x03,
    SYN_RECOVER         = 0x04,
    SYN_CHECK           = 0x21,
    SYN_POWERDOWN       = 0x88
};

enum encode_param{
    SYN_GB2312          = 0,
    SYN_GBK             = 1,
    SYN_BIG5            = 2,
    SYN_UNICODE         = 3
};

enum{
    SYN_CMD_LEN         = 1,
    SYN_CMDPARAM_LEN    = 1,
    SYN_CALIBRATION_LEN = 1,
    SYN_LEN_SUM         = 3     
};

static const osThreadAttr_t syn_task_attr = {
    .name = "syn_task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

void syn_msg_handle(syn_msg_t* msg, syn_t* syn){
    switch (msg->buffer[0]){
    case SYN_RECV_SUCC:
    case SYN_BROADCASTING:
        syn->state = SYN_BROADCASTING;
        break;
    case SYN_INIT_SUCC:  
    case SYN_IDLE:
        syn->state = SYN_IDLE;
        break;
    case SYN_RECV_FAIL:
        printf("syn send msg failed\r\n");
        syn->state = SYN_IDLE;
        // syn_check(syn);
        break;
    default:
        break;
    }
}

static void syn_task(void* args){
    syn_t* syn = (syn_t*)args;
    syn_msg_t msg;
    while(1){
        while(xQueueReceive(syn->msg_queue, &msg, 0) != pdTRUE)
            osDelay(1);
        syn_msg_handle(&msg, syn);
        xQueueSend(syn->space_queue, &msg.buffer, 0);
        osDelay(1);
    }
}

void syn_init(syn_t* syn, USART_TypeDef* usart, int qmsg_len){
    syn->usart = usart;
    syn->msg_queue = xQueueCreate(qmsg_len, sizeof(syn->recv_msg));
    syn->recv_msg.len = 0;
    syn->recv_msg.buffer = NULL;
    syn->woken = pdFALSE;
    syn->space_queue = xQueueCreate(qmsg_len, sizeof(syn->recv_msg.buffer));
    for(int i = 0; i < qmsg_len; ++i){
        char* addr = pvPortMalloc(MSG_BUFFER_LEN);
        xQueueSend(syn->space_queue, &addr, 0);
    }
    xQueueReceive(syn->space_queue, &syn->recv_msg.buffer, 0);
    syn->state = SYN_IDLE;
    syn->syn_task = osThreadNew(syn_task, syn, &syn_task_attr);
    LL_USART_EnableIT_IDLE(syn->usart);
    LL_USART_EnableIT_RXNE(syn->usart);
    printf("init syn uart:ok\r\n");
}

void syn_callback(syn_t* syn){
    if(LL_USART_IsActiveFlag_RXNE(syn->usart) && syn->recv_msg.buffer != NULL){
        uint8_t tmp = LL_USART_ReceiveData8(syn->usart);
        syn->recv_msg.buffer[syn->recv_msg.len] = tmp;
        ++syn->recv_msg.len;
        LL_USART_ClearFlag_RXNE(syn->usart);
    }
    if(LL_USART_IsActiveFlag_IDLE(syn->usart)){
        LL_USART_ClearFlag_IDLE(syn->usart);
        syn->recv_msg.buffer[syn->recv_msg.len] = ',';
        syn->recv_msg.buffer[syn->recv_msg.len+1] = '\0';
        ++syn->recv_msg.len;
        if(xQueueSendFromISR(syn->msg_queue, &syn->recv_msg, &syn->woken) != pdTRUE){
            printf("send msg failed\r\n");
        }
        if(xQueueReceiveFromISR(syn->space_queue, &syn->recv_msg.buffer, &syn->woken) != pdTRUE){
            syn->recv_msg.buffer = NULL;
        }
        syn->recv_msg.len = 0;
        if(syn->woken == pdTRUE) taskYIELD();
        syn->woken = pdFALSE;
    }
}


int syn_send_msg(syn_t* syn, syn_msg_t* msg){
    if(msg->buffer == NULL){
        printf("error occur in syn send msg\r\n");
        return 0;
    }
    for(int i = 0; i < msg->len; ++i){
        while(LL_USART_IsActiveFlag_TC(syn->usart) == RESET);
        LL_USART_TransmitData8(syn->usart, msg->buffer[i]);
    }
    while(LL_USART_IsActiveFlag_TC(syn->usart) == RESET);
    return 1;
}

int syn_attach_header(syn_msg_t* msg){
    msg->buffer[msg->len] = 0xFD;
    ++msg->len;
    return 0;
}

int syn_attach_len(syn_msg_t* msg, int len){
    msg->buffer[msg->len] = (len>>8)&0xff;
    msg->buffer[msg->len+1] = len&0xff;
    msg->len += 2;
    return 0;
}

int syn_attach_cmd(syn_msg_t* msg, uint8_t cmd){
    msg->buffer[msg->len] = cmd;
    ++msg->len;
    return 0;
}

int syn_attach_cmdparam(syn_msg_t* msg, uint8_t upper, uint8_t lower){
    uint8_t param = (upper<<5)|lower;
    msg->buffer[msg->len] = param;
    ++msg->len;
    return 0;
}

int syn_attach_data(syn_msg_t* msg, char* data, uint16_t len){
    memcpy(msg->buffer+msg->len, data, len);
    msg->len += len;
    return 0;
}

int syn_attach_errcheck(syn_msg_t* msg){
    uint8_t errcode = 0;
    for(int i = 0; i < msg->len; ++i){
        errcode ^= msg->buffer[i];
    }
    msg->buffer[msg->len] = errcode;
    ++msg->len;
	return 0;
}

char syn_send_buffer[256];
int syn6288_send(syn_t* syn, char* data, uint16_t len){
    if(len > SYN6288_DATA_LEN){
        printf("syn6288 buffer overflow\r\n");
        return -1;
    }
    
    if(syn->state != SYN_IDLE){
        printf("syn is not ready to broadcast\r\n");
        return -1;
    }

    syn_msg_t msg;
    msg.buffer = syn_send_buffer;
    msg.len = 0;
    syn_attach_header(&msg);
    syn_attach_len(&msg, len+SYN_LEN_SUM);
    syn_attach_cmd(&msg, SYN_BROADCAST);
    syn_attach_cmdparam(&msg, 0, SYN_UNICODE);
    syn_attach_data(&msg, data, len);
    syn_attach_errcheck(&msg);
    syn_send_msg(syn, &msg);
    // vPortFree(syn_send_buffer);
    return 0;
}

int syn_check(syn_t* syn){
    char syn_send_buf[20];
    syn_msg_t msg;
    msg.buffer = syn_send_buf;
    msg.len = 0;
    syn_attach_header(&msg);
    syn_attach_len(&msg, 2);
    syn_attach_cmd(&msg, SYN_CHECK);
    syn_attach_errcheck(&msg);
    return syn_send_msg(syn, &msg);
}
