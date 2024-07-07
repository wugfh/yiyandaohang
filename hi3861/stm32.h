#pragma once


void stm32_task_entry(void);
int sendtostm32(unsigned char* data, int len);
void stm32_gpio_init();
int stm_uart_config();
char* get_start_pos();