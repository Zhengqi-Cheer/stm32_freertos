#ifndef BOOTLOADER_UART_H
#define BOOTLOADER_UART_H

#include <stddef.h>
#include <stdint.h>

int bootloader_uart_init(uint32_t baud);
void bootloader_uart_putc(char c);
void bootloader_uart_write(const void *data, size_t len);
void bootloader_uart_puts(const char *s);
int bootloader_uart_getc_timeout(uint32_t timeout_ms);
int bootloader_uart_kbhit(void);

#endif /* BOOTLOADER_UART_H */
