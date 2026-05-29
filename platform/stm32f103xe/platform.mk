#######################################
# STM32F103xE platform
#######################################
CPU := -mcpu=cortex-m3
MCU := $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

C_DEFS += \
-DSTM32F103xE \
-DUSE_HAL_DRIVER

C_INCLUDES += \
-I$(GENERATED_DIR) \
-Iplatform/stm32f103xe/inc

AS_INCLUDES += \
-I$(GENERATED_DIR) \
-Iplatform/stm32f103xe/inc

PARTITION_INPUT := platform/stm32f103xe/partition.yaml
