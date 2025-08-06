#ifndef __STM32_F1xx_UART_H__
#define __STM32_F1xx_UART_H__

#include "stm32f1xx_hal_uart.h"

int uart_init(unsigned int bound);
void HAL_UART_MspInit(UART_HandleTypeDef *huart);


#endif