/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdio.h>

#include "led.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
*/


#define STM32_LED_ON GPIO_PIN_RESET //0
#define STM32_LED_OFF GPIO_PIN_SET //1

#define LED_TASK_NAME "led_task"

typedef struct
{
  stm32_led_type_e led_id;
  GPIO_TypeDef *GPIOx; //GPIOD
  unsigned int pin;
}stm32_led_map_t;

static stm32_led_map_t* __stm32_get_led_map(void);
static int __stm32_get_led_num(void);


static stm32_led_map_t stm32_led_map[] = {
	  {STM32_LED_LED1,      GPIOA,      GPIO_PIN_8},
    {STM32_LED_LED0,      GPIOD,      GPIO_PIN_2},
};


static stm32_led_map_t* __stm32_get_led_map(void)
{
  return stm32_led_map;
}

static int __stm32_get_led_num(void)
{
  return sizeof(stm32_led_map)/ sizeof(stm32_led_map_t);
}


int stm32_led_on(stm32_led_type_e led_id)
{
  const stm32_led_map_t *led_map_ptr = NULL; 
  int led_num = 0;

  led_map_ptr = __stm32_get_led_map();
  led_num = __stm32_get_led_num();

  for (int i = 0; i < led_num; i++)
  {
    if (led_map_ptr[i].led_id == led_id)
    {
        HAL_GPIO_WritePin(led_map_ptr[i].GPIOx, led_map_ptr[i].pin, GPIO_PIN_RESET);
        break;
    }
  }

  return 0;
}


int stm32_led_off(stm32_led_type_e led_id)
{
  const stm32_led_map_t *led_map_ptr = NULL; 
  int led_num = 0;

  led_map_ptr = __stm32_get_led_map();
  led_num = __stm32_get_led_num();

  for (int i = 0; i < led_num; i++)
  {
    if (led_map_ptr[i].led_id == led_id)
    {
        HAL_GPIO_WritePin(led_map_ptr[i].GPIOx, led_map_ptr[i].pin, GPIO_PIN_SET);
        break;
    }
  }

  return 0;
}


static void __stm32_led_bsp_init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  const stm32_led_map_t *led_map_ptr = NULL; 
  int led_num = 0;

  led_map_ptr = __stm32_get_led_map();
  led_num = __stm32_get_led_num();

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();  //这里是否可以加到系统初始化中
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  // HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED1_Pin */
  memset(&GPIO_InitStruct, 0x00, sizeof(GPIO_InitTypeDef));
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  
  for(int i = 0; i < led_num; i++)
  {
    HAL_GPIO_WritePin(led_map_ptr[i].GPIOx, led_map_ptr[i].pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = led_map_ptr[i].pin;
    HAL_GPIO_Init(led_map_ptr[i].GPIOx, &GPIO_InitStruct);
    // stm32_led_off(led_map_ptr[i].led_id);
  }

  return;
}



static void __led_main(void* arg)
{
  while(1)
  {
    stm32_led_on(STM32_LED_LED0);
    stm32_led_off(STM32_LED_LED1);
    vTaskDelay(500); //500ms
    stm32_led_off(STM32_LED_LED0);
    stm32_led_on(STM32_LED_LED1);
    vTaskDelay(500);
    printf("led_ok\n");
  }

  return;
}


static TaskHandle_t stm32_led_TaskHandle;
int led_process_init(void)
{
  __stm32_led_bsp_init();
  xTaskCreate(__led_main, LED_TASK_NAME, 256, (void*)NULL, 3, &stm32_led_TaskHandle);

  // taskEXIT_CRITICAL();
  return 0;
}


/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
