#######################################
# firmware image selection
#######################################
# TARGET_IMAGE=app      current single App at 0x08000000
# TARGET_IMAGE=app_a    App linked into App_A
# TARGET_IMAGE=stage0   reset vector, jumps to Boot_A
# TARGET_IMAGE=boot_a   serial bootloader in Boot_A

TARGET_IMAGE ?= app

ifeq ($(TARGET_IMAGE),boot_a)
TARGET := boot_a
BUILD_DIR := build/boot_a
LDSCRIPT := platform/stm32f103xe/linker/boot_a.ld
ENABLE_CORE_APP := n
ENABLE_CORE_LED := n
ENABLE_CORE_UART := n
ENABLE_CORE_SYSCMD := n
ENABLE_EMBEDDED_CLI := n
ENABLE_FREERTOS := n
ENABLE_STM32_HAL := y
ENABLE_BOOT_PLATFORM := y
ENABLE_BOOTLOADER := y
ENABLE_STAGE0 := n
else ifeq ($(TARGET_IMAGE),stage0)
TARGET := stage0
BUILD_DIR := build/stage0
LDSCRIPT := platform/stm32f103xe/linker/stage0.ld
ENABLE_CORE_APP := n
ENABLE_CORE_LED := n
ENABLE_CORE_UART := n
ENABLE_CORE_SYSCMD := n
ENABLE_EMBEDDED_CLI := n
ENABLE_FREERTOS := n
ENABLE_STM32_HAL := n
ENABLE_BOOT_PLATFORM := n
ENABLE_BOOTLOADER := n
ENABLE_STAGE0 := y
else ifeq ($(TARGET_IMAGE),app_a)
TARGET := app_a
BUILD_DIR := build/app_a
LDSCRIPT := platform/stm32f103xe/linker/app_a.ld
ENABLE_BOOT_PLATFORM := n
ENABLE_BOOTLOADER := n
ENABLE_STAGE0 := n
else ifeq ($(TARGET_IMAGE),app)
TARGET := STM32-freertos
BUILD_DIR := build
LDSCRIPT := STM32F103XX_FLASH.ld
ENABLE_BOOT_PLATFORM := n
ENABLE_BOOTLOADER := n
ENABLE_STAGE0 := n
else
$(error Unknown TARGET_IMAGE=$(TARGET_IMAGE). Use app, app_a, stage0, or boot_a)
endif
