#include "atgm.h"
#include "FreeRTOS.h"
#include "main.h"

nmea_t atgm;
osThreadId_t atgm_task;
const osThreadAttr_t atgmtask_attributes = {
  .name = "atgmTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void atgm_task_entry(void* args){
    nmea_available_reset(&atgm);
    while(1){
        nmea_loop(&atgm);
        osDelay(1);
    }
}

void atgm_init(USART_TypeDef *usart){
    nmea_init(&atgm, usart, 1024);
    atgm_task = osThreadNew(atgm_task_entry, NULL, &atgmtask_attributes);
}

