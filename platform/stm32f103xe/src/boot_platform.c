#include "boot_platform.h"

#include <string.h>

#include "platform_config.h"
#include "stm32f1xx_hal.h"

/*
 * STM32F103xE platform adapter.
 *
 * This file contains chip-specific operations used by Stage0 and Bootloader.
 * Portable boot code must call boot_platform.h instead of touching STM32 HAL or
 * Cortex-M registers directly.
 *
 * Flash programming notes for STM32F103xE:
 * - Page erase granularity is 2KB for high-density F103 parts.
 * - Program granularity is half-word, so writes must be 2-byte aligned.
 * - STM32 flash programming can only change bits from 1 to 0. Call erase before
 *   writing a page that may already contain programmed data.
 */

typedef void (*boot_entry_t)(void);

static const boot_flash_info_t g_flash_info = {
    PLATFORM_FLASH_BASE_ADDR,
    PLATFORM_FLASH_SIZE,
    PLATFORM_FLASH_ERASE_SIZE,
    PLATFORM_FLASH_WRITE_ALIGN,
};

static bool is_aligned(uint32_t value, uint32_t align)
{
    return (align != 0u) && ((value % align) == 0u);
}

const boot_flash_info_t *boot_platform_flash_info(void)
{
    return &g_flash_info;
}

bool boot_platform_flash_is_valid_range(uint32_t addr, uint32_t size)
{
    const uint32_t flash_end = g_flash_info.base_addr + g_flash_info.size;

    if (size == 0u) {
        return false;
    }

    if (addr < g_flash_info.base_addr) {
        return false;
    }

    if (addr > flash_end) {
        return false;
    }

    if (size > (flash_end - addr)) {
        return false;
    }

    return true;
}

boot_platform_status_t boot_platform_flash_read(uint32_t addr, void *data, size_t size)
{
    if ((data == NULL) || !boot_platform_flash_is_valid_range(addr, (uint32_t)size)) {
        return BOOT_PLATFORM_ERR_RANGE;
    }

    memcpy(data, (const void *)addr, size);
    return BOOT_PLATFORM_OK;
}

boot_platform_status_t boot_platform_flash_erase(uint32_t addr, uint32_t size)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0u;
    uint32_t page_count = 0u;
    HAL_StatusTypeDef status;

    if (!boot_platform_flash_is_valid_range(addr, size)) {
        return BOOT_PLATFORM_ERR_RANGE;
    }

    if (!is_aligned(addr - g_flash_info.base_addr, g_flash_info.erase_size) ||
        !is_aligned(size, g_flash_info.erase_size)) {
        return BOOT_PLATFORM_ERR_ALIGN;
    }

    page_count = size / g_flash_info.erase_size;

    memset(&erase_init, 0, sizeof(erase_init));
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = addr;
    erase_init.NbPages = page_count;

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return BOOT_PLATFORM_ERR;
    }

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    (void)HAL_FLASH_Lock();

    return (status == HAL_OK) ? BOOT_PLATFORM_OK : BOOT_PLATFORM_ERR;
}

boot_platform_status_t boot_platform_flash_write(uint32_t addr, const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t write_addr = addr;
    HAL_StatusTypeDef status = HAL_OK;

    if ((data == NULL) || !boot_platform_flash_is_valid_range(addr, (uint32_t)size)) {
        return BOOT_PLATFORM_ERR_RANGE;
    }

    if (!is_aligned(addr - g_flash_info.base_addr, g_flash_info.write_align) ||
        !is_aligned((uint32_t)size, g_flash_info.write_align)) {
        return BOOT_PLATFORM_ERR_ALIGN;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return BOOT_PLATFORM_ERR;
    }

    for (size_t i = 0u; i < size; i += 2u) {
        uint16_t half_word = (uint16_t)bytes[i] | ((uint16_t)bytes[i + 1u] << 8);

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, write_addr, half_word);
        if (status != HAL_OK) {
            break;
        }

        if (*(volatile uint16_t *)write_addr != half_word) {
            status = HAL_ERROR;
            break;
        }

        write_addr += 2u;
    }

    (void)HAL_FLASH_Lock();
    return (status == HAL_OK) ? BOOT_PLATFORM_OK : BOOT_PLATFORM_ERR;
}

uint32_t boot_platform_crc32(const void *data, size_t size, uint32_t seed)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = ~seed;

    if (data == NULL) {
        return seed;
    }

    /*
     * Standard reflected CRC32, polynomial 0x04C11DB7 represented as
     * 0xEDB88320. The seed parameter lets callers chain CRC over multiple
     * memory ranges by passing the previous result back in.
     */
    for (size_t i = 0; i < size; i++) {
        crc ^= bytes[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            if ((crc & 1u) != 0u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

void boot_platform_deinit_before_jump(void)
{
    /*
     * A boot jump must leave the target image with a clean interrupt state.
     * HAL_DeInit() resets HAL-owned peripherals; explicit NVIC cleanup handles
     * pending/enabled IRQs that may otherwise fire in the next image.
     */
    __disable_irq();

    SysTick->CTRL = 0u;
    SysTick->LOAD = 0u;
    SysTick->VAL = 0u;

    HAL_DeInit();

    NVIC->ICER[0] = 0xFFFFFFFFu;
    NVIC->ICER[1] = 0xFFFFFFFFu;
    NVIC->ICPR[0] = 0xFFFFFFFFu;
    NVIC->ICPR[1] = 0xFFFFFFFFu;
}

void boot_platform_jump_to_image(uint32_t vector_addr)
{
    const uint32_t initial_msp = *(volatile uint32_t *)vector_addr;
    const uint32_t reset_handler = *(volatile uint32_t *)(vector_addr + 4u);
    boot_entry_t entry = (boot_entry_t)reset_handler;

    /*
     * Cortex-M vector table layout:
     * [0] initial MSP
     * [1] Reset_Handler
     *
     * VTOR must point to the image vector table before MSP is changed. After
     * MSP is replaced, this function must not use stack-based state again.
     */
    SCB->VTOR = vector_addr;
    __DSB();
    __ISB();

    __set_MSP(initial_msp);
    __DSB();
    __ISB();

    entry();

    /*
     * A valid Reset_Handler never returns. If it does, reset into Stage0 so the
     * boot state machine can choose a fallback on the next pass.
     */
    boot_platform_reset();
}

void boot_platform_reset(void)
{
    NVIC_SystemReset();
}
