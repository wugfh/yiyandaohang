#pragma once


void stm32_task_entry(void);
int sendtostm32(char* data, int len);
void stm32_gpio_init(void);
int stm_uart_config(void);
char* get_start_pos(void);
char* get_target_pos(void);
void set_target_pos(char* lal, char* lng);