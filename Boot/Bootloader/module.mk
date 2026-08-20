######################################
# Serial bootloader
######################################
ifeq ($(ENABLE_BOOTLOADER),y)
C_SOURCES += \
Boot/Bootloader/Src/bootloader_entry.c \
Boot/Bootloader/Src/bootloader_uart.c \
Boot/Bootloader/Src/bootloader_cli.c \
Boot/Bootloader/Src/bootloader_xmodem.c \
Core/Src/system_stm32f1xx.c

ASM_SOURCES += \
startup_stm32f103xe.s

C_INCLUDES += \
-IBoot/Bootloader/Inc \
-IBoot/Common/Inc \
-ICore/Inc
endif
