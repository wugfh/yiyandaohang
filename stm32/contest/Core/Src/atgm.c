#include "atgm.h"
#include "FreeRTOS.h"
#include "main.h"

nmea_t atgm;
osThreadId_t atgm_task;
const osThreadAttr_t atgmtask_attributes = {
  .name = "atgmTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};

static void atgm_task_entry(void* args){
    nmea_available_reset(&atgm);
    nmea_msg nmsg;
    while(1){
        while(xQueueReceive(atgm.qrecv, &nmsg, 0) != pdTRUE)
            osDelay(1);
        nmea_loop(&nmsg, &atgm);
        memset(nmsg.buf, 0, nmsg.len);
        xQueueSend(atgm.qaddr, &nmsg.buf, 0);
        osDelay(1);
    }
}

void atgm_init(USART_TypeDef *usart){
    nmea_init(&atgm, usart, 5);
    atgm_task = osThreadNew(atgm_task_entry, NULL, &atgmtask_attributes);
    printf("init atgm uart:ok\r\n");
}

