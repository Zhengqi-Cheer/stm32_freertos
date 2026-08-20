#include "bootloader_xmodem.h"
#include "bootloader_uart.h"

#include "boot_platform.h"

#include <stdint.h>

#define XMODEM_SOH          0x01u
#define XMODEM_STX          0x02u
#define XMODEM_EOT          0x04u
#define XMODEM_ACK          0x06u
#define XMODEM_NAK          0x15u
#define XMODEM_CAN          0x18u
#define XMODEM_CRC_CHAR     'C'
#define XMODEM_MAX_PAYLOAD  1024u
#define XMODEM_START_TRIES  20u
#define XMODEM_IO_TIMEOUT_MS 1000u

static uint8_t g_xmodem_buf[XMODEM_MAX_PAYLOAD];

static uint16_t xmodem_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0u;
    uint32_t i;
    uint32_t bit;

    for (i = 0u; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (bit = 0u; bit < 8u; bit++) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static int xmodem_read_exact(uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
    uint32_t i;

    for (i = 0u; i < len; i++) {
        int ch = bootloader_uart_getc_timeout(timeout_ms);
        if (ch < 0) {
            return -1;
        }
        buf[i] = (uint8_t)ch;
    }

    return 0;
}

static int xmodem_write_payload(uint32_t dest_addr, uint32_t offset, uint32_t max_size,
                                uint32_t payload_len)
{
    if ((offset + payload_len) > max_size) {
        bootloader_uart_putc(XMODEM_CAN);
        bootloader_uart_putc(XMODEM_CAN);
        return BOOTLOADER_XMODEM_TOO_LARGE;
    }

    if (boot_platform_flash_write(dest_addr + offset, g_xmodem_buf, payload_len) != BOOT_PLATFORM_OK) {
        bootloader_uart_putc(XMODEM_CAN);
        bootloader_uart_putc(XMODEM_CAN);
        return BOOTLOADER_XMODEM_WRITE_FAIL;
    }

    return BOOTLOADER_XMODEM_OK;
}

int bootloader_xmodem_receive(uint32_t dest_addr, uint32_t max_size, uint32_t *received)
{
    uint32_t offset = 0u;
    uint8_t expected_block = 1u;
    uint32_t start_tries = 0u;
    uint8_t header[2];
    uint8_t crc_bytes[2];

    if (received != NULL) {
        *received = 0u;
    }

    while (1) {
        int ch;

        if (offset == 0u) {
            bootloader_uart_putc(XMODEM_CRC_CHAR);
            start_tries++;
            if (start_tries > XMODEM_START_TRIES) {
                return BOOTLOADER_XMODEM_TIMEOUT;
            }
        }

        ch = bootloader_uart_getc_timeout(XMODEM_IO_TIMEOUT_MS);
        if (ch < 0) {
            continue;
        }

        if (ch == (int)XMODEM_CAN) {
            return BOOTLOADER_XMODEM_CANCELED;
        }

        if (ch == (int)XMODEM_EOT) {
            bootloader_uart_putc(XMODEM_ACK);
            if (received != NULL) {
                *received = offset;
            }
            return BOOTLOADER_XMODEM_OK;
        }

        if ((ch != (int)XMODEM_SOH) && (ch != (int)XMODEM_STX)) {
            continue;
        }

        {
            const uint32_t payload_len = (ch == (int)XMODEM_STX) ? 1024u : 128u;
            uint16_t crc;
            uint16_t got_crc;
            int write_status;

            if (xmodem_read_exact(header, 2u, XMODEM_IO_TIMEOUT_MS) != 0) {
                bootloader_uart_putc(XMODEM_NAK);
                continue;
            }

            if (xmodem_read_exact(g_xmodem_buf, payload_len, XMODEM_IO_TIMEOUT_MS) != 0) {
                bootloader_uart_putc(XMODEM_NAK);
                continue;
            }

            if (xmodem_read_exact(crc_bytes, 2u, XMODEM_IO_TIMEOUT_MS) != 0) {
                bootloader_uart_putc(XMODEM_NAK);
                continue;
            }

            if (header[0] != (uint8_t)(~header[1])) {
                bootloader_uart_putc(XMODEM_NAK);
                continue;
            }

            crc = xmodem_crc16(g_xmodem_buf, payload_len);
            got_crc = ((uint16_t)crc_bytes[0] << 8) | crc_bytes[1];
            if (crc != got_crc) {
                bootloader_uart_putc(XMODEM_NAK);
                continue;
            }

            if (header[0] == (uint8_t)(expected_block - 1u)) {
                bootloader_uart_putc(XMODEM_ACK);
                continue;
            }

            if (header[0] != expected_block) {
                bootloader_uart_putc(XMODEM_NAK);
                continue;
            }

            write_status = xmodem_write_payload(dest_addr, offset, max_size, payload_len);
            if (write_status != BOOTLOADER_XMODEM_OK) {
                return write_status;
            }

            offset += payload_len;
            expected_block++;
            bootloader_uart_putc(XMODEM_ACK);
        }
    }
}
