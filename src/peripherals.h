#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void MX_USART2_UART_Init(void);
void SysTick_1ms_Init(void);

#endif