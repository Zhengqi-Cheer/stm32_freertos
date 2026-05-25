#######################################
# flags
#######################################
ASFLAGS := $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS += -Werror

ifeq ($(DEBUG),1)
CFLAGS += -g -gdwarf-2
endif

CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

LDFLAGS := $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

#######################################
# build the application
#######################################
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASMM_SOURCES:.S=.o)))
vpath %.S $(sort $(dir $(ASMM_SOURCES)))

MAKEFILE_DEPS := $(MAKEFILE_LIST)

$(BUILD_DIR)/%.o: %.c $(MAKEFILE_DEPS) | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s $(MAKEFILE_DEPS) | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.S $(MAKEFILE_DEPS) | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(MAKEFILE_DEPS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# helpers
#######################################
.PHONY: all clean list-modules

clean:
	-rm -fR $(BUILD_DIR)

list-modules:
	@echo "ENABLE_CORE_APP=$(ENABLE_CORE_APP)"
	@echo "ENABLE_CORE_LED=$(ENABLE_CORE_LED)"
	@echo "ENABLE_CORE_UART=$(ENABLE_CORE_UART)"
	@echo "ENABLE_CORE_SYSCMD=$(ENABLE_CORE_SYSCMD)"
	@echo "ENABLE_EMBEDDED_CLI=$(ENABLE_EMBEDDED_CLI)"
	@echo "ENABLE_STM32_HAL=$(ENABLE_STM32_HAL)"
	@echo "ENABLE_FREERTOS=$(ENABLE_FREERTOS)"

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)
