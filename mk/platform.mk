#######################################
# platform selection
#######################################
PLATFORM ?= stm32f103xe
PLATFORM_DIR := platform/$(PLATFORM)

include $(PLATFORM_DIR)/platform.mk
