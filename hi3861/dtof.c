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
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_uart.h"
#include "hi_io.h"
#include "iot_pwm.h"
#include "hi_pwm.h"
#include "ohos_init.h"
#include "stm32.h"
#include "iot_errno.h"

#define IOT_UART_IDX_1  (1)
#define STACK_SIZE   (1024)
#define DELAY_US     (20000)
#define IOT_GPIO_0  (0) //uart1
#define IOT_GPIO_1  (1)
#define IOT_GPIO_9   9  // Beep
#define IOT_GPIO_10  10  // Red
#define IOT_GPIO_11  11  // Green
#define IOT_GPIO_12  12  // Yellow
#define IOT_GPIO_5   (5)
#define IOT_GPIO_6   (6)

#define IOT_PWM_PORT_PWM0   0
#define PWM_DUTY_50        (50)
#define PWM_DUTY_99        (99)
#define PWM_FREQ_4K        (4000)
#define DTOF_DATA_LEN       137
#define CRC_LEN             133

unsigned char uartReadBuff[300] = {0};
unsigned char crcBuff[300] = {0};
static unsigned int crc32table[256];
char alarm_msg[] = "barrier";
/*
 * 初始化uart1接口配置
 * Initialize uart1 interface configuration
 */
int dtof_uart_config(void)
{
    IotUartAttribute g_uart_cfg = {
        .baudRate = 115200, 
        .dataBits = 8, 
        .stopBits = 1, 
        .parity = IOT_UART_PARITY_NONE, 
        .rxBlock = IOT_UART_BLOCK_STATE_BLOCK, 
        .txBlock = IOT_UART_BLOCK_STATE_BLOCK, 
        .pad = 0
    };
    int ret = IoTUartInit(IOT_UART_IDX_1, &g_uart_cfg);
    while (ret != IOT_SUCCESS) 
    {
        printf("dtof uart init fail\r\n");
        sleep(2);
        ret = IoTUartInit(IOT_UART_IDX_1, &g_uart_cfg);
    }

    return ret;
}


static void init_crc_table(void)
{
    unsigned int c;
    unsigned int i, j;
    for (i = 0; i < 256; i++) 
    {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++) 
        {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }                                                                                                                                            
        crc32table[i] = c;
    }
};

/*
 * crc校验
 * crc checking
 */

static unsigned int crc32(const unsigned char *buf, unsigned int size)
{
    unsigned int  i, crc = 0xFFFFFFFF;

    for (i = 0; i < size; i++) 
    {
        crc = crc32table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8); /* 8: 右移8bit */
    }
    return crc ^ 0xFFFFFFFF;
}

/*
 * 配置uart1、pwm0、红灯、黄灯、绿灯的gpio管脚
 * Configure GPIO pins for uart1, wm0, red light, yellow light, and green light
 */
void dtof_gpio_init(void)
{
    IoTGpioInit(IOT_GPIO_0);  
    hi_io_set_func(IOT_GPIO_0, HI_IO_FUNC_GPIO_0_UART1_TXD);
    IoTGpioInit(IOT_GPIO_1);  
    hi_io_set_func(IOT_GPIO_1, HI_IO_FUNC_GPIO_1_UART1_RXD);
}

/* 
 * 接收dtof数据并处理
 * Receive dtof data and process it
 */
static void* DTOF_Task(void)
{
    static int r = 0; //统计距离小于2米的点云数量，满足条件亮红灯
    static int d = 0; 
    unsigned short int *data = NULL;
    short int i = 0;

    unsigned int len = 0; //接收到的串口数据长度
    unsigned int crc_data = 0; //接收CRC校验后的值

    init_crc_table();
    int cnt = 0;
    while (1) 
    {
        len += IoTUartRead(IOT_UART_IDX_1, uartReadBuff+len, DTOF_DATA_LEN);
        if (len >= DTOF_DATA_LEN) 
        {
            if(uartReadBuff[0] == 0xAA && uartReadBuff[1] == 0x55 && uartReadBuff[132] == 0xFF) 
            {
                memcpy(crcBuff,uartReadBuff,CRC_LEN);
                crc_data = crc32(crcBuff,CRC_LEN);

                if(crc_data == *(unsigned int *)(&uartReadBuff[CRC_LEN])) 
                {
                    for(i = 0; i < 64; i++) 
                    {
                        data = (unsigned short int *)(&uartReadBuff[i*2+4]);
                        if((*data) < 1000 && (*data) > 10)
                        {
                            r += 1;
                        }

                    }
                    d = d + 1;
                }
                else
                {
                    printf("crc32 fail ！！！\r\n");
                }
            }
            len = 0;
            crc_data = 0;
            if(r > 56){
                ++cnt;
            }
            else{
                cnt = 0;
            }
            if(cnt > 5){
                sendtostm32(alarm_msg, sizeof(alarm_msg));
                cnt = 0;
            }
            r=0;
            data = NULL;
            memset(uartReadBuff,0,sizeof(uartReadBuff));
            memset(crcBuff,0,sizeof(crcBuff));
        }
        usleep(50000);
    }

    return NULL;
}

/*
 * 任务入口函数
 * Task Entry Function
 */
void dtof_init(void)
{
    osThreadAttr_t attr = {0};

    printf("[dtof_init] dtof_init()\n");

    attr.name = "dtof_init";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = STACK_SIZE;      
    attr.priority = osPriorityNormal;  

    if (osThreadNew((osThreadFunc_t)DTOF_Task, NULL, &attr) == NULL) 
    {
        printf("[dtof_init] Falied to create UartDemo_Task!\n");
    }
}
