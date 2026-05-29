#include "boot_partition.h"

#include <stddef.h>

/*
 * 这里先提供轻量框架，不直接绑定具体 Flash 地址。
 * 后续 Stage0 会把固定地址处的 PartitionTable_0/1 映射成
 * boot_partition_table_t，再调用这些接口做选择。
 */

bool boot_partition_table_is_valid(const boot_partition_table_t *table)
{
    if (table == NULL) {
        return false;
    }

    if (table->magic != BOOT_PARTITION_MAGIC) {
        return false;
    }

    if (table->entry_count > BOOT_PARTITION_MAX_ENTRY) {
        return false;
    }

    /*
     * TODO:
     * 1. 校验 header_crc。
     * 2. 校验 table_crc。
     * 3. 校验每个 entry 的地址和大小没有越界、没有重叠。
     */
    return true;
}

const boot_partition_entry_t *boot_partition_find(const boot_partition_table_t *table,
                                                  boot_partition_id_t id)
{
    if (!boot_partition_table_is_valid(table)) {
        return NULL;
    }

    for (uint32_t i = 0; i < table->entry_count; i++) {
        if (table->entries[i].id == (uint32_t)id) {
            return &table->entries[i];
        }
    }

    return NULL;
}

const boot_partition_table_t *boot_partition_select_active(const boot_partition_table_t *table0,
                                                           const boot_partition_table_t *table1)
{
    const bool table0_valid = boot_partition_table_is_valid(table0);
    const bool table1_valid = boot_partition_table_is_valid(table1);

    if (table0_valid && table1_valid) {
        return (table1->seq > table0->seq) ? table1 : table0;
    }

    if (table0_valid) {
        return table0;
    }

    if (table1_valid) {
        return table1;
    }

    return NULL;
}
