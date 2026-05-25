######################################
# Core application
######################################
ifeq ($(ENABLE_CORE_APP),y)
C_SOURCES += \
Core/Src/stm32f1xx_hal_msp.c \
Core/Src/main.c \
Core/Src/freertos.c \
Core/Src/stm32f1xx_it.c \
Core/Src/system_stm32f1xx.c

ASM_SOURCES += \
startup_stm32f103xe.s

C_INCLUDES += \
-ICore/Inc

AS_INCLUDES += \
-ICore/Inc
endif

ifeq ($(ENABLE_CORE_LED),y)
C_SOURCES += \
Core/led/led.c

C_INCLUDES += \
-ICore/led
endif

ifeq ($(ENABLE_CORE_UART),y)
C_SOURCES += \
Core/uart/uart.c

C_INCLUDES += \
-ICore/uart
endif

ifeq ($(ENABLE_CORE_SYSCMD),y)
C_SOURCES += \
Core/syscmd/cmd_cli.c

C_INCLUDES += \
-ICore/syscmd
endif

ifeq ($(ENABLE_EMBEDDED_CLI),y)
C_SOURCES += \
Core/cmd_lib/lib/embedded_cli.c

C_INCLUDES += \
-ICore/cmd_lib/lib
endif
