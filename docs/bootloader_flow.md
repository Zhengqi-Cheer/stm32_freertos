# Bootloader Flow

本文档描述 `Boot/Bootloader/Src/bootloader_main.c` 的抽象启动流程。

Bootloader 的职责是选择并启动 App_A/App_B，同时维护 App 的 pending、confirmed、rollback 状态。它不直接依赖 STM32 HAL，所有硬件操作通过 `boot_platform.h` 抽象层完成。

## 1. Inputs

Bootloader 启动时依赖以下输入：

```text
PartitionTable_0
PartitionTable_1
BootControl_0
BootControl_1
App_A image
App_B image
```

当前实现中，PartitionTable 真实二进制还未生成和烧录，所以代码提供了 fallback：

```text
generated/<platform>/partition_config.h
        ↓
g_fallback_partition_table
```

这保证 Bootloader 主流程可以先落地，后续再接真实 PartitionTable。

## 2. Main Flow

入口函数：

```c
int bootloader_main(void)
```

主流程：

```text
bootloader_main
  │
  ├─ load active PartitionTable
  │    ├─ try PartitionTable_0
  │    ├─ try PartitionTable_1
  │    └─ fallback to generated partition_config.h table
  │
  ├─ load active BootControl
  │    ├─ try BootControl_0
  │    ├─ try BootControl_1
  │    └─ create default BootControl if both invalid
  │
  ├─ confirm running Bootloader if it was pending
  │
  ├─ select App slot
  │    ├─ pending App has priority
  │    ├─ pending try_count++
  │    ├─ invalid pending -> mark BAD
  │    └─ try fallback slot
  │
  ├─ save BootControl
  │    └─ copy-on-write to inactive BootControl page
  │
  ├─ deinit platform before jump
  │
  └─ jump to selected App vector table
```

## 3. PartitionTable Loading

Function:

```c
static const boot_partition_table_t *bootloader_load_partition_table(void)
```

Logic:

```text
table0 = PartitionTable_0 address
table1 = PartitionTable_1 address
active = boot_partition_select_active(table0, table1)

if active == NULL:
    active = g_fallback_partition_table
```

Selection rule is implemented in:

```c
boot_partition_select_active()
```

Expected final behavior:

```text
choose table with:
  valid magic
  valid header_crc
  valid table_crc
  highest seq
```

Current limitation:

```text
boot_partition_table_is_valid() does not yet verify CRC.
```

## 4. BootControl Loading

Function:

```c
static bool bootloader_load_control(bootloader_context_t *context)
```

Logic:

```text
control0 = BootControl_0 address
control1 = BootControl_1 address
active = boot_control_select_active(control0, control1)

if active exists:
    copy active into RAM context
    remember active copy address
else:
    create default control
```

Default control:

```text
Bootloader active    = A
Bootloader confirmed = A
App active           = A
App confirmed        = A
pending slots        = NONE
```

Current limitation:

```text
boot_control_is_valid() does not yet verify CRC.
```

## 5. Bootloader Confirmation

Function:

```c
static void bootloader_confirm_running_bootloader(bootloader_context_t *context)
```

Bootloader confirms itself when:

```text
current bootloader slot == control.bootloader.pending_slot
```

Then:

```text
active_slot    = current slot
confirmed_slot = current slot
pending_slot   = NONE
try_count      = 0
slot state     = CONFIRMED
```

Current slot is detected by checking the address of `bootloader_main()`:

```text
bootloader_main address in Boot_A range -> BOOT_SLOT_A
bootloader_main address in Boot_B range -> BOOT_SLOT_B
```

This requires Bootloader_A and Bootloader_B to be linked to their real slot addresses.

## 6. App Slot Selection

Function:

```c
static const boot_partition_entry_t *bootloader_select_app(
    const boot_partition_table_t *table,
    bootloader_context_t *context)
```

Selection starts with:

```c
selected_slot = boot_control_select_slot(&context->control.app);
```

`boot_control_select_slot()` implements:

```text
if pending_slot exists:
    if try_count < BOOT_MAX_TRY_COUNT:
        try_count++
        return pending_slot
    else:
        mark pending BAD
        clear pending
        try_count = 0

if active_slot exists:
    return active_slot

return confirmed_slot
```

After a slot is selected:

```text
find App partition
validate App image
if valid -> boot it
if invalid -> mark BAD and try other slot
```

Fallback behavior:

```text
selected App invalid
  ↓
mark selected BAD
  ↓
try other App slot
  ↓
valid -> boot fallback
invalid -> recovery
```

## 7. App Image Validation

Function:

```c
static bool bootloader_validate_app_partition(const boot_partition_entry_t *partition)
```

It reads the App image header from:

```text
partition->start_addr + partition->image_offset
```

Then calls:

```c
boot_image_header_is_valid(header, BOOT_IMAGE_TYPE_APP, partition)
```

Expected final validation:

```text
header magic valid
image_type == APP
image_size inside partition
vector table inside partition
initial MSP inside SRAM
Reset_Handler inside image range
header CRC valid
image CRC valid
version policy valid
```

Current limitation:

```text
boot_image_header_is_valid() currently checks only basic fields.
CRC and vector-depth checks are still TODO.
```

## 8. BootControl Save

Function:

```c
static bool bootloader_save_control(const bootloader_context_t *context)
```

Save strategy:

```text
active copy is BootControl_0 -> write BootControl_1
active copy is BootControl_1 -> write BootControl_0
```

Write sequence:

```text
next = context->control
next.seq++
next.crc = crc(next without crc field)
erase inactive BootControl page
write new BootControl
```

This is copy-on-write. If power fails during write, the old copy is still available.

Important constraints:

```text
BootControl_0 and BootControl_1 must be independent erase pages.
Flash write must be range checked.
Never erase PartitionTable while updating BootControl.
```

## 9. Jump To App

After App is selected and BootControl is saved:

```c
boot_platform_deinit_before_jump();
boot_platform_jump_to_image(app_header->vector_addr);
```

Platform layer handles:

```text
disable interrupts
stop SysTick
deinit peripherals
clear NVIC pending/enabled IRQs
set SCB->VTOR
set MSP
jump Reset_Handler
```

For STM32F103xE this is implemented in:

```text
platform/stm32f103xe/src/boot_platform.c
```

## 10. Recovery Path

Function:

```c
static void bootloader_enter_recovery(uint32_t reason)
```

Current behavior:

```text
while (1) {}
```

Expected future behavior:

```text
start bootloader CLI
allow serial upgrade
show partition and image status
allow manual rollback
allow erase/write inactive slot
```

Recovery reasons currently defined:

```c
#define BOOTLOADER_RECOVERY_INVALID_TABLE      1u
#define BOOTLOADER_RECOVERY_INVALID_CONTROL    2u
#define BOOTLOADER_RECOVERY_NO_APP             3u
```

## 11. State Transition Summary

Pending App success:

```text
pending App boots
App calls boot_api_confirm_app()
pending_slot = NONE
active_slot = running slot
confirmed_slot = running slot
try_count = 0
```

Pending App fails three times:

```text
Bootloader tries pending App
try_count reaches BOOT_MAX_TRY_COUNT
pending App marked BAD
pending_slot = NONE
rollback to previous active/confirmed App
```

Confirmed App normal boot:

```text
No pending slot
Bootloader selects active/confirmed App
No unnecessary BootControl update should be needed
```

## 12. Current Gaps

The Bootloader flow is now structured, but these pieces are still required for a complete product bootloader:

```text
1. Generate and flash real PartitionTable binary.
2. Implement boot_partition_table_is_valid() CRC checks.
3. Implement boot_control_is_valid() CRC and slot checks.
4. Implement full boot_image_header_is_valid() image CRC checks.
5. Add App-side boot_api_confirm_app().
6. Add Bootloader CLI or upgrade transport.
7. Add Bootloader_A / Bootloader_B linker scripts.
8. Add boot_a / boot_b build targets.
9. Add package manifest validation.
10. Add write protection policy for Stage0 and PartitionTable pages.
```

## 13. Files Involved

```text
Boot/Bootloader/Src/bootloader_main.c
Boot/Common/Inc/boot_control.h
Boot/Common/Src/boot_control.c
Boot/Common/Inc/boot_partition.h
Boot/Common/Src/boot_partition.c
Boot/Common/Inc/boot_image.h
Boot/Common/Src/boot_image.c
Boot/Common/Inc/boot_platform.h
platform/stm32f103xe/src/boot_platform.c
generated/stm32f103xe/partition_config.h
```
