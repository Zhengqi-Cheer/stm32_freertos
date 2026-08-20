#include "partition_config.h"
#include "platform_config.h"

#include "stm32f1xx.h"

#include <stdint.h>

/*
 * Minimal Stage0: stay at the reset vector and jump to Bootloader_A.
 * No UART, no HAL, no A/B selection yet.
 */

typedef void (*stage0_entry_t)(void);

static int stage0_in_range(uint32_t value, uint32_t start, uint32_t end)
{
    return (value >= start) && (value < end);
}

static int stage0_bootloader_is_valid(uint32_t vector_addr)
{
    const uint32_t sp = *(volatile uint32_t *)vector_addr;
    const uint32_t reset = *(volatile uint32_t *)(vector_addr + 4u);
    const uint32_t thumb_pc = reset & ~1u;
    const uint32_t sram_end = PLATFORM_SRAM_BASE_ADDR + PLATFORM_SRAM_SIZE;

    if (!stage0_in_range(sp, PLATFORM_SRAM_BASE_ADDR, sram_end + 1u)) {
        return 0;
    }

    if ((reset & 1u) == 0u) {
        return 0;
    }

    if (!stage0_in_range(thumb_pc, PARTITION_BOOT_A_ADDR, PARTITION_BOOT_A_END_ADDR)) {
        return 0;
    }

    return 1;
}

static void stage0_jump(uint32_t vector_addr)
{
    const uint32_t sp = *(volatile uint32_t *)vector_addr;
    const uint32_t reset = *(volatile uint32_t *)(vector_addr + 4u);
    stage0_entry_t entry = (stage0_entry_t)reset;

    SCB->VTOR = vector_addr;
    __DSB();
    __ISB();
    __set_MSP(sp);
    __DSB();
    __ISB();
    entry();
}

int main(void)
{
    if (stage0_bootloader_is_valid(PARTITION_BOOT_A_ADDR)) {
        stage0_jump(PARTITION_BOOT_A_ADDR);
    }

    while (1) {
    }
}
