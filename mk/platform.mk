#######################################
# platform selection
#######################################
PLATFORM ?= stm32f103xe
PLATFORM_DIR := platform/$(PLATFORM)
GENERATED_DIR := generated/$(PLATFORM)

include $(PLATFORM_DIR)/platform.mk
