######################################
# Stage0
######################################
ifeq ($(ENABLE_STAGE0),y)
C_SOURCES += \
Boot/Stage0/Src/stage0_main.c \
Core/Src/system_stm32f1xx.c

ASM_SOURCES += \
startup_stm32f103xe.s

C_INCLUDES += \
-IBoot/Common/Inc \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
-IDrivers/CMSIS/Include

AS_INCLUDES += \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
-IDrivers/CMSIS/Include
endif
