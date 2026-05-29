# Portable Boot Framework Code Structure

本文档定义后续可移植启动框架的代码结构。目标是降低更换 MCU、调整 Flash 分区、扩展应用功能时的改动范围。

核心原则：

- 平台无关代码只处理格式、状态机和策略。
- 芯片相关代码集中在 `platform/<platform_name>/`。
- 分区地址由平台分区配置自动生成，不在业务代码里手写。
- App 通过服务接口访问配置、升级、Boot 状态，不直接操作硬件细节。

## 1. Target Directory Layout

建议最终目录结构如下：

```text
.
├── Makefile
├── build.sh
├── mk/
│   ├── config.mk
│   ├── common.mk
│   ├── platform.mk
│   └── rules.mk
│
├── Boot/
│   ├── Common/
│   │   ├── Inc/
│   │   │   ├── boot_control.h
│   │   │   ├── boot_image.h
│   │   │   ├── boot_partition.h
│   │   │   └── boot_platform.h
│   │   └── Src/
│   │       ├── boot_control.c
│   │       ├── boot_image.c
│   │       └── boot_partition.c
│   │
│   ├── Stage0/
│   │   ├── Inc/
│   │   ├── Src/
│   │   │   └── stage0_main.c
│   │   └── module.mk
│   │
│   └── Bootloader/
│       ├── Inc/
│       ├── Src/
│       │   └── bootloader_main.c
│       └── module.mk
│
├── AppServices/
│   ├── boot_api/
│   │   ├── boot_api.h
│   │   └── boot_api.c
│   ├── config_store/
│   │   ├── config_store.h
│   │   └── config_store.c
│   └── upgrade_client/
│       ├── upgrade_client.h
│       └── upgrade_client.c
│
├── platform/
│   ├── stm32f103xe/
│   │   ├── platform.mk
│   │   ├── partition.yaml
│   │   ├── inc/
│   │   │   └── platform_config.h
│   │   ├── src/
│   │   │   ├── boot_platform.c
│   │   │   ├── flash_driver.c
│   │   │   ├── reset_jump.c
│   │   │   └── platform_crc.c
│   │   └── linker/
│   │       ├── stage0.ld.in
│   │       ├── boot.ld.in
│   │       └── app.ld.in
│   │
│   └── <new_platform>/
│       ├── platform.mk
│       ├── partition.yaml
│       ├── inc/
│       ├── src/
│       └── linker/
│
├── generated/
│   └── <platform_name>/
│       ├── partition_config.h
│       ├── partition_layout.md
│       ├── stage0.ld
│       ├── boot_a.ld
│       ├── boot_b.ld
│       ├── app_a.ld
│       └── app_b.ld
│
└── tools/
    └── gen_partition.c
```

当前仓库还没有完全迁移到该结构。现阶段的 `Boot/`、`partition/`、`generated/` 和 `tools/gen_partition.c` 是第一步框架。

## 2. Layer Responsibilities

### 2.1 Boot/Common

`Boot/Common` 是平台无关层，不允许依赖具体芯片 HAL。

允许处理：

- 分区表结构和查询
- 镜像头格式
- BootControl A/B 状态机
- pending/confirmed/bad 状态转换
- 三次失败回退策略
- 镜像校验流程调度

不允许处理：

- Flash 解锁、擦写、上锁
- VTOR 设置
- MSP 设置
- 关闭具体外设
- 看门狗配置
- UART/CAN/USB 等通信外设
- 具体芯片寄存器

### 2.2 Boot/Stage0

Stage0 是固定入口，原则上不参与普通升级。

职责：

- 从固定地址读取分区表 A/B。
- 选择 seq 最大且 CRC 正确的分区表。
- 读取 BootControl A/B。
- 对 Bootloader_A/B 执行三次失败回退。
- 校验并跳转 Bootloader。
- Bootloader A/B 都不可用时进入最小恢复路径。

Stage0 不应包含复杂功能。代码越少，系统恢复能力越稳定。

### 2.3 Boot/Bootloader

Bootloader 由 Stage0 启动，也采用 A/B 主备。

职责：

- 确认当前 Bootloader 启动成功。
- 根据分区表定位 App_A/B。
- 对 App_A/B 执行三次失败回退。
- 支持升级 App。
- 支持升级备用 Bootloader。
- 支持升级分区表。
- 提供最小维护命令，例如 `show`、`upgrade`、`confirm`、`rollback`。

### 2.4 platform/<platform_name>

平台层封装所有芯片差异。

职责：

- Flash base/size/erase/write_align 定义
- Flash erase/write/read 驱动
- 系统复位
- 跳转镜像前的硬件清理
- VTOR/MSP/Reset_Handler 跳转
- CRC 实现
- 链接脚本模板
- 芯片相关编译参数和宏

新增芯片时，应新增一个 `platform/<new_platform>/`，而不是修改公共启动逻辑。

### 2.5 AppServices

AppServices 是应用功能和启动框架之间的稳定接口。

建议先拆三类：

- `boot_api`：确认 App 启动成功、查询当前 slot、请求回滚。
- `config_store`：配置读写，不让业务代码直接操作 Flash 地址。
- `upgrade_client`：下载升级包、写入目标分区、更新 BootControl。

业务代码只依赖 AppServices，避免直接依赖 `partition_config.h` 或平台 Flash 驱动。

## 3. Platform Interface

建议新增：

```text
Boot/Common/Inc/boot_platform.h
```

接口草案：

```c
#ifndef BOOT_PLATFORM_H
#define BOOT_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t base_addr;
    uint32_t size;
    uint32_t erase_size;
    uint32_t write_align;
} boot_flash_info_t;

typedef enum {
    BOOT_PLATFORM_OK = 0,
    BOOT_PLATFORM_ERR = -1,
    BOOT_PLATFORM_ERR_ALIGN = -2,
    BOOT_PLATFORM_ERR_RANGE = -3,
} boot_platform_status_t;

const boot_flash_info_t *boot_platform_flash_info(void);

bool boot_platform_flash_is_valid_range(uint32_t addr, uint32_t size);
boot_platform_status_t boot_platform_flash_erase(uint32_t addr, uint32_t size);
boot_platform_status_t boot_platform_flash_write(uint32_t addr,
                                                 const void *data,
                                                 size_t size);
boot_platform_status_t boot_platform_flash_read(uint32_t addr,
                                                void *data,
                                                size_t size);

uint32_t boot_platform_crc32(const void *data, size_t size, uint32_t seed);

void boot_platform_deinit_before_jump(void);
void boot_platform_jump_to_image(uint32_t vector_addr);
void boot_platform_reset(void);

#endif /* BOOT_PLATFORM_H */
```

`Boot/Common` 调用该接口，但不关心具体 MCU。

## 4. Platform Build Selection

建议新增：

```text
mk/platform.mk
```

内容示例：

```make
PLATFORM ?= stm32f103xe
PLATFORM_DIR := platform/$(PLATFORM)
GENERATED_DIR := generated/$(PLATFORM)

include $(PLATFORM_DIR)/platform.mk
```

根 `Makefile` include 顺序建议：

```make
include mk/config.mk
include mk/common.mk
include mk/platform.mk

include Core/module.mk
include Drivers/module.mk
include Middlewares/module.mk
include Boot/Common/module.mk

include mk/rules.mk
```

平台 Makefile 示例：

```make
# platform/stm32f103xe/platform.mk

CPU := -mcpu=cortex-m3
MCU := $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

C_DEFS += \
-DSTM32F103xE \
-DUSE_HAL_DRIVER

C_INCLUDES += \
-Iplatform/stm32f103xe/inc \
-I$(GENERATED_DIR) \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
-IDrivers/CMSIS/Include

PLATFORM_SOURCES += \
platform/stm32f103xe/src/boot_platform.c \
platform/stm32f103xe/src/flash_driver.c \
platform/stm32f103xe/src/reset_jump.c \
platform/stm32f103xe/src/platform_crc.c

PARTITION_INPUT := platform/stm32f103xe/partition.yaml
```

切换平台：

```sh
make PLATFORM=stm32f103xe
make PLATFORM=<new_platform>
```

或者：

```sh
bash build.sh PLATFORM=<new_platform>
```

## 5. Partition Generation

分区配置应该随平台走：

```text
platform/<platform_name>/partition.yaml
```

原因：

- Flash 起始地址依赖芯片。
- Flash 总大小依赖芯片。
- erase size 依赖芯片。
- 写入对齐依赖芯片。
- Bootloader/App 分区大小也常随芯片 Flash 容量调整。

`partition.yaml` 应只维护：

- Flash 基础信息
- 固定入口区
- 自动布局起点
- 分区顺序和大小

示例：

```yaml
flash:
  base: 0x08000000
  size: 512K
  erase_size: 2K

fixed:
  - name: stage0
    type: stage0
    address: 0x08000000
    size: 16K

auto_layout:
  start_after: stage0
  align: erase_size

layout:
  - name: partition_table_0
    type: partition_table
    size: 4K
  - name: partition_table_1
    type: partition_table
    size: 4K
  - name: bootctrl_0
    type: boot_control
    size: 4K
  - name: bootctrl_1
    type: boot_control
    size: 4K
  - name: boot_a
    type: bootloader
    size: 48K
  - name: boot_b
    type: bootloader
    size: 48K
  - name: app_a
    type: app
    size: 160K
  - name: app_b
    type: app
    size: 160K
  - name: config
    type: config
    size: 64K
```

生成输出：

```text
generated/<platform_name>/partition_config.h
generated/<platform_name>/partition_layout.md
generated/<platform_name>/stage0.ld
generated/<platform_name>/boot_a.ld
generated/<platform_name>/boot_b.ld
generated/<platform_name>/app_a.ld
generated/<platform_name>/app_b.ld
```

当前 `tools/gen_partition.c` 已经能生成 `partition_config.h` 和 `partition_layout.md`。后续应继续扩展 linker script 生成。

### 5.1 STM32F103xE Example Flash Map

当前 `platform/stm32f103xe/partition.yaml` 对应的 512KB Flash 分区示意图如下：

```text
Flash: 0x08000000 - 0x08080000, 512KB

0x08000000  ┌──────────────────────────────────────┐
            │ Stage0                               │ 16KB
0x08004000  ├──────────────────────────────────────┤
            │ PartitionTable_0                     │ 4KB
0x08005000  ├──────────────────────────────────────┤
            │ PartitionTable_1                     │ 4KB
0x08006000  ├──────────────────────────────────────┤
            │ BootControl_0                        │ 4KB
0x08007000  ├──────────────────────────────────────┤
            │ BootControl_1                        │ 4KB
0x08008000  ├──────────────────────────────────────┤
            │ Bootloader_A                         │ 48KB
0x08014000  ├──────────────────────────────────────┤
            │ Bootloader_B                         │ 48KB
0x08020000  ├──────────────────────────────────────┤
            │ App_A                                │ 160KB
0x08048000  ├──────────────────────────────────────┤
            │ App_B                                │ 160KB
0x08070000  ├──────────────────────────────────────┤
            │ Config                               │ 64KB
0x08080000  └──────────────────────────────────────┘
```

启动链关系：

```text
Reset
  │
  ▼
Stage0
  │
  ├─ read PartitionTable_0 / PartitionTable_1
  │
  ├─ read BootControl_0 / BootControl_1
  │
  ├─ select Bootloader_A or Bootloader_B
  │     pending slot has priority
  │     failed 3 times -> rollback
  │
  ▼
Bootloader
  │
  ├─ confirm current Bootloader if needed
  │
  ├─ read Partition Table
  │
  ├─ select App_A or App_B
  │     pending slot has priority
  │     failed 3 times -> rollback
  │
  ▼
App
  │
  └─ boot_api_confirm_app()
```

A/B 回退逻辑示意：

```text
Upgrade writes inactive slot
        │
        ▼
Mark inactive slot as PENDING
        │
        ▼
Reset
        │
        ▼
Boot pending slot, try_count++
        │
        ├─ confirm success
        │      │
        │      ▼
        │   pending -> CONFIRMED
        │   active_slot = pending_slot
        │   try_count = 0
        │
        └─ no confirm after 3 tries
               │
               ▼
            pending -> BAD
            rollback to previous confirmed slot
```

## 6. Image Link Rules

Cortex-M 固件通常不是位置无关代码。

因此：

- Bootloader_A 必须按 Bootloader_A 地址链接。
- Bootloader_B 必须按 Bootloader_B 地址链接。
- App_A 必须按 App_A 地址链接。
- App_B 必须按 App_B 地址链接。

分区地址变化后，必须重新生成 linker script 并重新编译对应镜像。

构建目标建议：

```sh
make TARGET_IMAGE=stage0
make TARGET_IMAGE=boot_a
make TARGET_IMAGE=boot_b
make TARGET_IMAGE=app_a
make TARGET_IMAGE=app_b
```

或封装：

```sh
bash build.sh stage0
bash build.sh boot-a
bash build.sh boot-b
bash build.sh app-a
bash build.sh app-b
```

## 7. Boot State Model

Bootloader 和 App 都使用同一套 slot 状态：

```c
typedef enum {
    BOOT_SLOT_STATE_EMPTY = 0,
    BOOT_SLOT_STATE_VALID,
    BOOT_SLOT_STATE_PENDING,
    BOOT_SLOT_STATE_CONFIRMED,
    BOOT_SLOT_STATE_BAD,
} boot_slot_state_t;
```

启动策略：

```text
if pending_slot exists:
    if try_count < 3:
        try_count++
        boot pending_slot
    else:
        mark pending_slot BAD
        clear pending_slot
        boot active/confirmed slot
else:
    boot active/confirmed slot
```

确认策略：

```text
running image calls confirm
active_slot = running slot
confirmed_slot = running slot
pending_slot = NONE
try_count = 0
running slot state = CONFIRMED
```

Stage0 管 Bootloader A/B。

Bootloader 管 App A/B。

## 8. App Configuration Access

业务代码不应直接使用：

```c
CONFIG_PART_ADDR
CONFIG_PART_SIZE
```

建议通过 `config_store`：

```c
int config_store_init(void);
int config_store_read(uint32_t key, void *data, uint32_t len);
int config_store_write(uint32_t key, const void *data, uint32_t len);
int config_store_commit(void);
```

`config_store` 内部再使用：

- `partition_config.h`
- `boot_platform_flash_read`
- `boot_platform_flash_erase`
- `boot_platform_flash_write`

这样换芯片时，业务配置逻辑基本不变。

## 9. Upgrade Package Direction

后续升级包建议包含 manifest：

```text
manifest:
  platform: stm32f103xe
  partition_seq: 12
  images:
    - type: app
      slot: b
      version: 1002
      size: ...
      crc: ...
    - type: bootloader
      slot: b
      version: 205
      size: ...
      crc: ...
```

Bootloader 升级流程：

```text
receive package
validate platform
validate partition compatibility
erase target inactive slot
write image
verify image crc
set pending slot
reset
```

如果涉及分区表升级，必须保证：

- 新分区表不覆盖 Stage0。
- 新分区表不覆盖当前正在执行的关键区域。
- 新分区表和新镜像一起验证。
- pending partition table 也要有三次失败回退。

## 10. Migration Plan From Current Repository

建议按以下顺序迁移，避免一次性改动过大。

### Step 1: Introduce Platform Directory

新增：

```text
platform/stm32f103xe/
```

移动：

```text
partition/partition.yaml
```

到：

```text
platform/stm32f103xe/partition.yaml
```

### Step 2: Add mk/platform.mk

新增平台选择变量：

```make
PLATFORM ?= stm32f103xe
PLATFORM_DIR := platform/$(PLATFORM)
GENERATED_DIR := generated/$(PLATFORM)
```

### Step 3: Move Partition Output

把生成输出从：

```text
generated/
```

改成：

```text
generated/$(PLATFORM)/
```

### Step 4: Add boot_platform.h

新增平台抽象接口，但先不强制所有代码使用。

### Step 5: Implement STM32 Platform Adapter

新增：

```text
platform/stm32f103xe/src/boot_platform.c
```

先实现：

- flash info
- reset
- jump stub
- crc stub

### Step 6: Generate Linker Scripts

扩展 `tools/gen_partition.c`：

- 生成 `stage0.ld`
- 生成 `boot_a.ld`
- 生成 `boot_b.ld`
- 生成 `app_a.ld`
- 生成 `app_b.ld`

### Step 7: Split Build Targets

新增目标：

```text
stage0
boot_a
boot_b
app_a
app_b
```

### Step 8: Move App Config Logic Behind config_store

业务代码不再直接操作 Flash 地址。

## 11. Rules For Adding A New Chip

新增芯片时只做这些事：

1. 新增 `platform/<chip>/partition.yaml`
2. 新增 `platform/<chip>/platform.mk`
3. 新增 `platform/<chip>/inc/platform_config.h`
4. 新增 `platform/<chip>/src/boot_platform.c`
5. 新增或复用 linker 模板
6. 调整 `Drivers/` 或芯片 SDK include/source
7. 执行：

```sh
make PLATFORM=<chip>
```

不应改动：

- `Boot/Common`
- BootControl 状态机
- 镜像头格式
- App 业务配置接口
- 升级状态机核心逻辑

如果新增芯片必须修改这些公共层，说明抽象边界不足，需要先评估是否应扩展 `boot_platform.h`。

## 12. Current Gaps

当前已有：

- Boot 框架目录
- 分区表结构
- BootControl 结构
- 镜像头结构
- C 语言分区生成器
- 自动生成 `partition_config.h`
- 默认编译自动刷新分区文件

当前缺失：

- `boot_platform.h`
- `platform/stm32f103xe/`
- 分平台生成目录
- linker script 自动生成
- Stage0 独立构建目标
- Bootloader 独立构建目标
- App A/B 独立链接
- Flash 写入平台适配
- 真正的 CRC 校验
- BootControl Flash 持久化
- App `config_store`

这些缺口建议按迁移计划逐步补齐。
