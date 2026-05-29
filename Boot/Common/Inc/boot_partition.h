#ifndef BOOT_PARTITION_H
#define BOOT_PARTITION_H

#include <stdbool.h>
#include <stdint.h>

/*
 * 分区表是启动链的“地图”。
 *
 * 设计原则：
 * 1. Stage0 和 PartitionTable_0/1 的地址固定，保证系统永远有入口。
 * 2. Bootloader_A/B、App_A/B、BootControl、Config 等地址由分区表描述。
 * 3. 后续修改 App 或 Bootloader 分区时，只更新分区表和重新链接对应镜像。
 * 4. 分区表使用双备份 + seq + crc，避免写入过程中掉电导致无法启动。
 */

#define BOOT_PARTITION_MAGIC      0x5054424Cu /* 'PTBL' */
#define BOOT_PARTITION_MAX_ENTRY  16u

typedef enum {
    BOOT_PART_ID_INVALID = 0,
    BOOT_PART_ID_STAGE0,
    BOOT_PART_ID_TABLE_0,
    BOOT_PART_ID_TABLE_1,
    BOOT_PART_ID_BOOT_A,
    BOOT_PART_ID_BOOT_B,
    BOOT_PART_ID_APP_A,
    BOOT_PART_ID_APP_B,
    BOOT_PART_ID_BOOTCTRL_0,
    BOOT_PART_ID_BOOTCTRL_1,
    BOOT_PART_ID_CONFIG,
    BOOT_PART_ID_FACTORY,
    BOOT_PART_ID_RESERVED,
} boot_partition_id_t;

typedef enum {
    BOOT_PART_TYPE_INVALID = 0,
    BOOT_PART_TYPE_STAGE0,
    BOOT_PART_TYPE_PARTITION_TABLE,
    BOOT_PART_TYPE_BOOTLOADER,
    BOOT_PART_TYPE_APP,
    BOOT_PART_TYPE_BOOT_CONTROL,
    BOOT_PART_TYPE_CONFIG,
    BOOT_PART_TYPE_RESERVED,
} boot_partition_type_t;

typedef struct {
    uint32_t id;            /* boot_partition_id_t */
    uint32_t type;          /* boot_partition_type_t */
    uint32_t flags;         /* 预留：只读、可升级、加密、压缩等 */
    uint32_t start_addr;    /* Flash 绝对地址，例如 0x0801E000 */
    uint32_t size;          /* 分区大小，单位 byte */
    uint32_t image_offset;  /* 镜像头相对分区起始偏移 */
    uint32_t vector_offset; /* 向量表相对分区起始偏移 */
    uint32_t reserved;
} boot_partition_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t seq;          /* 双备份分区表选择 seq 最大且 crc 正确的一份 */
    uint32_t table_size;
    uint32_t entry_count;
    uint32_t header_crc;
    uint32_t table_crc;
    uint32_t reserved;
    boot_partition_entry_t entries[BOOT_PARTITION_MAX_ENTRY];
} boot_partition_table_t;

bool boot_partition_table_is_valid(const boot_partition_table_t *table);
const boot_partition_entry_t *boot_partition_find(const boot_partition_table_t *table,
                                                  boot_partition_id_t id);
const boot_partition_table_t *boot_partition_select_active(const boot_partition_table_t *table0,
                                                           const boot_partition_table_t *table1);

#endif /* BOOT_PARTITION_H */
