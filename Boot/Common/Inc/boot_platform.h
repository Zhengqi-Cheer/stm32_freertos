#ifndef BOOT_PLATFORM_H
#define BOOT_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Platform abstraction for portable boot code.
 *
 * Boot/Common, Stage0, and Bootloader should call these functions instead of
 * including chip HAL headers directly. A new MCU should provide a new
 * platform/<platform_name>/src/boot_platform.c implementation.
 */

typedef struct {
    uint32_t base_addr;
    uint32_t size;
    uint32_t erase_size;
    uint32_t write_align;
} boot_flash_info_t;

typedef enum {
    BOOT_PLATFORM_OK = 0,
    BOOT_PLATFORM_ERR = -1,
    BOOT_PLATFORM_ERR_ALIGN = -2,
    BOOT_PLATFORM_ERR_RANGE = -3,
} boot_platform_status_t;

const boot_flash_info_t *boot_platform_flash_info(void);

bool boot_platform_flash_is_valid_range(uint32_t addr, uint32_t size);
boot_platform_status_t boot_platform_flash_read(uint32_t addr, void *data, size_t size);
boot_platform_status_t boot_platform_flash_erase(uint32_t addr, uint32_t size);
boot_platform_status_t boot_platform_flash_write(uint32_t addr, const void *data, size_t size);

uint32_t boot_platform_crc32(const void *data, size_t size, uint32_t seed);

void boot_platform_deinit_before_jump(void);
void boot_platform_jump_to_image(uint32_t vector_addr);
void boot_platform_reset(void);

#endif /* BOOT_PLATFORM_H */
