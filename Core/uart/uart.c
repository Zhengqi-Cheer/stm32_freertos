#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "uart.h"
#include "stm32f1xx_hal_uart.h"

#include "cmd_cli.h"

#define STM32_UART_TX_PIN   GPIO_PIN_9
#define STM32_UART_RX_PIN   GPIO_PIN_10
#define STM32_UART_GPIOx    GPIOA

#define UART_TASK_NAME "uart_task"



extern HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
extern HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
extern HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
extern void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
extern HAL_UART_StateTypeDef HAL_UART_GetState(const UART_HandleTypeDef *huart);







static unsigned char stm32_uart_rx_buff[128] = {0};
static UART_HandleTypeDef UART1_Handler;

#if 0  //加入以下代码支持printf
#pragma import(__use_no_semihosting)             
             
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
   
void _sys_exit(int x) 
{ 
	x = x; 
} 

int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0); 
    USART1->DR = (unsigned char) ch;      
	return ch;
}
#endif 






// 串口接收中断回调函数
static uint8_t uart_rx_data = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* 中断写队列 */
        QueueHandle_t cmd_cli_queue_handle = cmd_cli_queue_get();
        if (cmd_cli_queue_handle) 
        {
            (void)xQueueSendFromISR( cmd_cli_queue_handle, &uart_rx_data, NULL );
        }

        HAL_UART_Receive_IT(huart, &uart_rx_data, 1); /* 继续接收 */
    }

    return;
}


/* 可以不写这个函数，其他地方使能GPIO也一样 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

	if (huart->Instance == USART1)
	{
	    __HAL_RCC_USART1_CLK_ENABLE();
	    __HAL_RCC_GPIOA_CLK_ENABLE();
	    
	    GPIO_InitStruct.Pin = STM32_UART_TX_PIN | STM32_UART_RX_PIN;
	    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	    GPIO_InitStruct.Pull = GPIO_PULLUP;
	    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	    HAL_GPIO_Init(STM32_UART_GPIOx, &GPIO_InitStruct);
    }

    return;
}


// 修改__uart_init_IT函数
static void __uart_init_IT(unsigned int bound)
{
    UART1_Handler.Instance = USART1;
    UART1_Handler.Init.BaudRate = bound;
    UART1_Handler.Init.WordLength = UART_WORDLENGTH_8B;
    UART1_Handler.Init.StopBits = UART_STOPBITS_1;
    UART1_Handler.Init.Parity = UART_PARITY_NONE;
    UART1_Handler.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UART1_Handler.Init.Mode = UART_MODE_TX_RX;
    
    if (HAL_UART_Init(&UART1_Handler) != HAL_OK)
    {
        ; // Error_Handler();
    }
    
    if (HAL_UART_Receive_IT(&UART1_Handler, stm32_uart_rx_buff, sizeof(stm32_uart_rx_buff)) != HAL_OK)
    {
        ;// Error_Handler();
    }
    
    const char *msg = "STM32 Uart init ok !\n";
    if (HAL_UART_Transmit_IT(&UART1_Handler, (uint8_t *)msg, strlen(msg)) != HAL_OK)
    {
        ;// Error_Handler();
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
      ;// 发送完成处理
    }
}


/* 
    这个函数是已经声明了的，名字固定的串口中断处理函数。
*/
void USART1_IRQHandler(void)
{
    unsigned int timeout=0;

    HAL_UART_IRQHandler(&UART1_Handler); //处理中断

    timeout=0;
    while (HAL_UART_GetState(&UART1_Handler) != HAL_UART_STATE_READY) // 等待就绪
    {
        timeout++;
        if(timeout>HAL_MAX_DELAY) break;
    }

    timeout=0;
    while(HAL_UART_Receive_IT(&UART1_Handler, (unsigned char *)stm32_uart_rx_buff, sizeof(stm32_uart_rx_buff)) != HAL_OK)
    {
        timeout++;
        if(timeout>HAL_MAX_DELAY) break;	
    }
}



static void __stm32_uart_main(void* arg)
{

    while(1)
    {
        ;
    }

    return;
}



static TaskHandle_t stm32_uart_TaskHandle;
int uart_init(unsigned int bound)
{
    __uart_init_IT(bound);
    stm32_uart_TaskHandle = NULL;
    xTaskCreate(__stm32_uart_main, UART_TASK_NAME, 256, (void*)NULL, 3, &stm32_uart_TaskHandle);

    return 0;
}


