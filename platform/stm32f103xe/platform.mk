#######################################
# STM32F103xE platform
#######################################
CPU := -mcpu=cortex-m3
MCU := $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

C_DEFS += \
-DSTM32F103xE

ifeq ($(ENABLE_STM32_HAL),y)
C_DEFS += \
-DUSE_HAL_DRIVER
endif

C_INCLUDES += \
-Iplatform/stm32f103xe/inc

AS_INCLUDES += \
-Iplatform/stm32f103xe/inc

C_SOURCES += \
platform/stm32f103xe/src/partition_config.c

ifeq ($(ENABLE_BOOT_PLATFORM),y)
C_SOURCES += \
platform/stm32f103xe/src/boot_platform.c
endif
