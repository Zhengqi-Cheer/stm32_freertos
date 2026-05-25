######################################
# module switches
######################################
# These switches control whether a module contributes sources and include paths.
# If a module is disabled, code that calls into it must also be guarded.

ENABLE_CORE_APP      ?= y
ENABLE_CORE_LED      ?= y
ENABLE_CORE_UART     ?= y
ENABLE_CORE_SYSCMD   ?= y
ENABLE_EMBEDDED_CLI  ?= y
ENABLE_STM32_HAL     ?= y
ENABLE_FREERTOS      ?= y
