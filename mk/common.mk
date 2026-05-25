######################################
# building variables
######################################
DEBUG ?= 1
OPT ?= -Og

#######################################
# binaries
#######################################
PREFIX ?= arm-none-eabi-
GCC_PATH ?= $(ROOT)/toolchain/gcc-arm-none-eabi-5_4-2016q3/bin

CC := $(GCC_PATH)/$(PREFIX)gcc
AS := $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP := $(GCC_PATH)/$(PREFIX)objcopy
SZ := $(GCC_PATH)/$(PREFIX)size

HEX := $(CP) -O ihex
BIN := $(CP) -O binary -S

#######################################
# target cpu
#######################################
CPU := -mcpu=cortex-m3
MCU := $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

#######################################
# project-wide flags
#######################################
C_DEFS += \
-DSTM32F103xE \
-DUSE_HAL_DRIVER

AS_DEFS +=

LDSCRIPT ?= STM32F103XX_FLASH.ld
LIBS ?= -lc -lm -lnosys
LIBDIR ?=
