/*
 * Copyright (C) 2021 HiHope Open Source Organization .
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_uart.h"
#include "hi_io.h"
#include "iot_pwm.h"
#include "hi_pwm.h"
#include "ohos_init.h"
#include "iot_errno.h"
#include "hi_isr.h"
#include "hi3861.h"


#define STM32_UART_TX_PIN   11
#define STM32_UART_RX_PIN   12
#define STM32_UART_IDX      2
#define STM32_DATA_LEN      256
#define STM32_HEADER_LEN    3
#define STM32_LEN_OFFSET    2       

#define UART_MIS_OFFSET     0x040
#define UART_RIS_OFFSET     0x03c
#define UART_IMSC_OFFSET    0x038
#define UART_IFLS_OFFSET    0x034
#define UART_ICR_OFFSET     0x044

#define UART2_BASEADDR      0x4000A000

#define IMSC_VALUE          0x1

static char origin[32] = "32.073838,118.671309";
static char destination[32] = "32.073838,118.671309";
static uint8_t data_buf[STM32_DATA_LEN];

static void uart_handler(){
    int len = IoTUartRead(STM32_UART_IDX, data_buf, 100);
    int begin = 0, end = 0;
    for(int i = 0; i < len; ++i){
        if(data_buf[i] == 's'){
            int j = i;
            while(data_buf[j] != 'e') ++j;
            int validlen = j-i-1;
            memcpy(origin, &data_buf[i+1], validlen);
            origin[validlen] = '\0';
            i = j;
        }
        if(data_buf[i] == 't'){
            int j = i;
            while(data_buf[j] != 'e') ++j;
            int validlen = j-i-1;
            memcpy(destination, &data_buf[i+1], validlen);
            destination[validlen] = '\0';
            i = j;
        }
    }
}

/*
 * 初始化uart2接口配置
 * Initialize uart2 interface configuration
 */
int stm_uart_config(void)
{
    IotUartAttribute g_uart_cfg = {
        .baudRate = 115200, 
        .dataBits = 8, 
        .stopBits = 1, 
        .parity = IOT_UART_PARITY_NONE, 
        .rxBlock = IOT_UART_BLOCK_STATE_NONE_BLOCK, 
        .txBlock = IOT_UART_BLOCK_STATE_NONE_BLOCK, 
        .pad = 0
    };
    int ret = IoTUartInit(STM32_UART_IDX, &g_uart_cfg);

    while (ret != IOT_SUCCESS){
        printf("stm32 uart init fail ret=%d\r\n", ret);
        sleep(2);
        ret = IoTUartInit(STM32_UART_IDX, &g_uart_cfg);
    }

    return ret;
}

void stm32_gpio_init(void)
{
    IoTGpioInit(STM32_UART_TX_PIN);  
    hi_io_set_func(STM32_UART_TX_PIN, HI_IO_FUNC_GPIO_11_UART2_TXD);
    IoTGpioInit(STM32_UART_RX_PIN);  
    hi_io_set_func(STM32_UART_RX_PIN, HI_IO_FUNC_GPIO_12_UART2_RXD);
}

void parse_pos(char* data, uint32_t* lal, uint32_t* lon, uint32_t* alt){
    if(data == NULL)
        return;
    memcpy(lal, data, sizeof(uint32_t));
    int offset = sizeof(uint32_t);
    memcpy(lon, data+offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(alt,data+offset, sizeof(uint32_t));
}

static void* stm32_task(void){
    int len = 0;
    uint32_t lal_tmp, lon_tmp, alt_tmp;
    while(1){
        uart_handler();
        sleep(1);
    }
    return NULL;
}

int sendtostm32(char* data, int len){
    for(int i = 0; i < len; ++i){
        putchar(data[i]);
    }
    putchar('\n');
    return IoTUartWrite(STM32_UART_IDX, data, len);
}

char* get_start_pos(){
    return origin;
}

char* get_target_pos(){
    return destination;
}

void set_target_pos(char* lal, char* lng){
    snprintf(destination, 32, "%s,%s", lal, lng);
}

/*
 * 任务入口函数
 * Task Entry Function
 */
void stm32_task_entry(void)
{ 
    osThreadAttr_t attr = {0};

    attr.name = "stm32_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;      
    attr.priority = osPriorityNormal;  

    if (osThreadNew((osThreadFunc_t)stm32_task, NULL, &attr) == NULL) 
    {
        printf("Falied to create stm32_task!\n");
    }
}
