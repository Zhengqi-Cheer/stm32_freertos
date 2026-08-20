######################################
# Portable boot common
######################################
ifeq ($(ENABLE_BOOTLOADER),y)
C_INCLUDES += \
-IBoot/Common/Inc
endif
