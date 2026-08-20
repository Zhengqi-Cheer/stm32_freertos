#include "bootloader_cli.h"
#include "bootloader_uart.h"
#include "bootloader_xmodem.h"

#include "boot_platform.h"
#include "partition_config.h"
#include "platform_config.h"
#include "stm32f1xx_hal.h"

#include <stdint.h>
#include <string.h>

#define BOOTLOADER_LINE_MAX          64u
#define BOOTLOADER_WAIT_MS           3000u

static int vector_in_range(uint32_t value, uint32_t start, uint32_t end)
{
    return (value >= start) && (value < end);
}

static int app_vector_is_valid(uint32_t vector_addr, uint32_t image_end)
{
    const uint32_t sp = *(volatile uint32_t *)vector_addr;
    const uint32_t reset = *(volatile uint32_t *)(vector_addr + 4u);
    const uint32_t thumb_pc = reset & ~1u;
    const uint32_t sram_end = PLATFORM_SRAM_BASE_ADDR + PLATFORM_SRAM_SIZE;

    if (!vector_in_range(sp, PLATFORM_SRAM_BASE_ADDR, sram_end + 1u)) {
        return 0;
    }

    if ((reset & 1u) == 0u) {
        return 0;
    }

    if (!vector_in_range(thumb_pc, vector_addr, image_end)) {
        return 0;
    }

    return 1;
}

static void print_hex(uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char buf[11];
    int i;

    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 8; i++) {
        buf[9 - i] = digits[value & 0xFu];
        value >>= 4;
    }
    buf[10] = '\0';
    bootloader_uart_puts(buf);
}

static void print_u32(uint32_t value)
{
    char buf[11];
    int pos = 10;

    buf[pos] = '\0';
    do {
        pos--;
        buf[pos] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);

    bootloader_uart_puts(&buf[pos]);
}

static void cmd_help(void)
{
    bootloader_uart_puts("commands:\n");
    bootloader_uart_puts("  help   show this list\n");
    bootloader_uart_puts("  info   show flash map\n");
    bootloader_uart_puts("  jump   jump to App_A\n");
    bootloader_uart_puts("  erase  erase App_A\n");
    bootloader_uart_puts("  load   XMODEM-CRC receive into App_A\n");
    bootloader_uart_puts("  reboot reset MCU\n");
}

static void cmd_info(void)
{
    bootloader_uart_puts("stage0    ");
    print_hex(PARTITION_STAGE0_ADDR);
    bootloader_uart_puts(" ");
    print_u32(PARTITION_STAGE0_SIZE);
    bootloader_uart_puts("\nboot_a    ");
    print_hex(PARTITION_BOOT_A_ADDR);
    bootloader_uart_puts(" ");
    print_u32(PARTITION_BOOT_A_SIZE);
    bootloader_uart_puts("\napp_a     ");
    print_hex(PARTITION_APP_A_ADDR);
    bootloader_uart_puts(" ");
    print_u32(PARTITION_APP_A_SIZE);
    bootloader_uart_puts("\napp valid ");
    bootloader_uart_puts(app_vector_is_valid(PARTITION_APP_A_ADDR, PARTITION_APP_A_END_ADDR) ? "yes\n" : "no\n");
}

static void cmd_jump(void)
{
    if (!app_vector_is_valid(PARTITION_APP_A_ADDR, PARTITION_APP_A_END_ADDR)) {
        bootloader_uart_puts("no valid App_A\n");
        return;
    }

    bootloader_uart_puts("jump App_A\n");
    boot_platform_deinit_before_jump();
    boot_platform_jump_to_image(PARTITION_APP_A_ADDR);
}

static int cmd_erase(void)
{
    bootloader_uart_puts("erase App_A...\n");
    if (boot_platform_flash_erase(PARTITION_APP_A_ADDR, PARTITION_APP_A_SIZE) != BOOT_PLATFORM_OK) {
        bootloader_uart_puts("erase fail\n");
        return -1;
    }

    bootloader_uart_puts("erase ok\n");
    return 0;
}

static int cmd_load(void)
{
    uint32_t received = 0u;
    int status;

    if (cmd_erase() != 0) {
        return -1;
    }

    bootloader_uart_puts("send App_A .bin with XMODEM-CRC\n");
    status = bootloader_xmodem_receive(PARTITION_APP_A_ADDR, PARTITION_APP_A_SIZE, &received);

    if (status == BOOTLOADER_XMODEM_OK) {
        bootloader_uart_puts("load ok ");
        print_u32(received);
        bootloader_uart_puts(" bytes\n");
        return 0;
    }

    if (status == BOOTLOADER_XMODEM_TIMEOUT) {
        bootloader_uart_puts("load timeout\n");
    } else if (status == BOOTLOADER_XMODEM_CANCELED) {
        bootloader_uart_puts("load canceled\n");
    } else if (status == BOOTLOADER_XMODEM_TOO_LARGE) {
        bootloader_uart_puts("image too large\n");
    } else {
        bootloader_uart_puts("write fail\n");
    }

    return -1;
}

static void handle_line(char *line)
{
    if ((strcmp(line, "help") == 0) || (strcmp(line, "?") == 0)) {
        cmd_help();
    } else if (strcmp(line, "info") == 0) {
        cmd_info();
    } else if (strcmp(line, "jump") == 0) {
        cmd_jump();
    } else if (strcmp(line, "erase") == 0) {
        (void)cmd_erase();
    } else if (strcmp(line, "load") == 0) {
        (void)cmd_load();
    } else if (strcmp(line, "reboot") == 0) {
        boot_platform_reset();
    } else if (line[0] != '\0') {
        bootloader_uart_puts("unknown command, try help\n");
    }
}

static void wait_for_key_or_jump(void)
{
    const uint32_t start = HAL_GetTick();

    bootloader_uart_puts("press any key to stay, else jump App_A\n");

    while ((HAL_GetTick() - start) < BOOTLOADER_WAIT_MS) {
        if (bootloader_uart_kbhit()) {
            (void)bootloader_uart_getc_timeout(10u);
            return;
        }
    }

    if (app_vector_is_valid(PARTITION_APP_A_ADDR, PARTITION_APP_A_END_ADDR)) {
        cmd_jump();
    }
}

void bootloader_cli_run(void)
{
    char line[BOOTLOADER_LINE_MAX];
    uint32_t len = 0u;

    bootloader_uart_puts("\nSTM32 serial bootloader\n");
    cmd_info();
    wait_for_key_or_jump();
    cmd_help();
    bootloader_uart_puts("boot> ");

    while (1) {
        int ch = bootloader_uart_getc_timeout(1000u);

        if (ch < 0) {
            continue;
        }

        if ((ch == '\r') || (ch == '\n')) {
            bootloader_uart_puts("\n");
            line[len] = '\0';
            handle_line(line);
            len = 0u;
            bootloader_uart_puts("boot> ");
            continue;
        }

        if ((ch == 0x08) || (ch == 0x7F)) {
            if (len > 0u) {
                len--;
                bootloader_uart_puts("\b \b");
            }
            continue;
        }

        if ((ch >= 32) && (ch < 127) && ((len + 1u) < BOOTLOADER_LINE_MAX)) {
            line[len++] = (char)ch;
            bootloader_uart_putc((char)ch);
        }
    }
}
