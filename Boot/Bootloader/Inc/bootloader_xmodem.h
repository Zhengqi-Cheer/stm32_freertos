#ifndef BOOTLOADER_XMODEM_H
#define BOOTLOADER_XMODEM_H

#include <stdint.h>

#define BOOTLOADER_XMODEM_OK           0
#define BOOTLOADER_XMODEM_TIMEOUT     -1
#define BOOTLOADER_XMODEM_CANCELED    -2
#define BOOTLOADER_XMODEM_TOO_LARGE   -3
#define BOOTLOADER_XMODEM_WRITE_FAIL  -4

int bootloader_xmodem_receive(uint32_t dest_addr, uint32_t max_size, uint32_t *received);

#endif /* BOOTLOADER_XMODEM_H */
