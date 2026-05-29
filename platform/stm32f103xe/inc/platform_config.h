#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

/*
 * STM32F103xE platform constants.
 *
 * Keep hardware facts here. Portable boot logic should use boot_platform.h
 * rather than including STM32 HAL or register headers directly.
 */

#define PLATFORM_NAME               "stm32f103xe"
#define PLATFORM_FLASH_BASE_ADDR    0x08000000u
#define PLATFORM_FLASH_SIZE         (512u * 1024u)
#define PLATFORM_FLASH_ERASE_SIZE   (2u * 1024u)
#define PLATFORM_FLASH_WRITE_ALIGN  2u

#endif /* PLATFORM_CONFIG_H */
