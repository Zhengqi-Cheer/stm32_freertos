#include "bootloader_uart.h"

#include "stm32f1xx_hal.h"

#include <string.h>

static UART_HandleTypeDef g_uart;

int bootloader_uart_init(uint32_t baud)
{
    g_uart.Instance = USART1;
    g_uart.Init.BaudRate = baud;
    g_uart.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart.Init.StopBits = UART_STOPBITS_1;
    g_uart.Init.Parity = UART_PARITY_NONE;
    g_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart.Init.Mode = UART_MODE_TX_RX;

    if (HAL_UART_Init(&g_uart) != HAL_OK) {
        return -1;
    }

    return 0;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    if (huart->Instance != USART1) {
        return;
    }

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void bootloader_uart_putc(char c)
{
    (void)HAL_UART_Transmit(&g_uart, (uint8_t *)&c, 1u, 100u);
}

void bootloader_uart_write(const void *data, size_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;

    if ((bytes == NULL) || (len == 0u)) {
        return;
    }

    (void)HAL_UART_Transmit(&g_uart, (uint8_t *)bytes, (uint16_t)len, 1000u);
}

void bootloader_uart_puts(const char *s)
{
    if (s == NULL) {
        return;
    }

    while (*s != '\0') {
        if (*s == '\n') {
            bootloader_uart_putc('\r');
        }
        bootloader_uart_putc(*s);
        s++;
    }
}

int bootloader_uart_getc_timeout(uint32_t timeout_ms)
{
    uint8_t ch = 0;

    if (HAL_UART_Receive(&g_uart, &ch, 1u, timeout_ms) != HAL_OK) {
        return -1;
    }

    return (int)ch;
}

int bootloader_uart_kbhit(void)
{
    return (__HAL_UART_GET_FLAG(&g_uart, UART_FLAG_RXNE) != RESET) ? 1 : 0;
}
