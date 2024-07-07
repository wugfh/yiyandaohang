#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdbool.h>
#include "main.h"
#include "cmsis_os.h"
#include <queue.h>

#define AUDIO_BUFFER_LEN        256
#define RECV_QUEUE_LEN          10

typedef struct{
    uint8_t buffer[AUDIO_BUFFER_LEN+1];
    uint8_t index;
}audio_msg;

typedef struct su03_audio{
    USART_TypeDef   *usart;
    audio_msg       recv_buf;
    audio_msg       send_buf;
    xQueueHandle    revent_queue;
    osThreadId_t    audio_task_thread;
    BaseType_t      woken;
}audio_t;

void audio_init(audio_t* au, USART_TypeDef* audio_usart);
void audio_callback(audio_t* au);
void audio_task(void* au);
void audio_attach_header(audio_msg* msg);
void audio_attach_tailer(audio_msg* msg);
void audio_attach_pos(audio_msg* msg, double latitude, double longitude, double altitude);
int audio_send_msg(audio_t* au);
int audio_send_pos(audio_t* au, double latitude, double longitude, double altitude);
int audio_send_how(audio_t* au, int dir, int dis, int dura, double latitude, double longitude, double altitude);


#endif

