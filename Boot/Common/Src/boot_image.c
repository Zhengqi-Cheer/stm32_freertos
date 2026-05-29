#include "boot_image.h"

#include <stddef.h>

bool boot_image_vector_is_valid(uint32_t vector_addr,
                                const boot_partition_entry_t *partition)
{
    if (partition == NULL) {
        return false;
    }

    if (vector_addr < partition->start_addr) {
        return false;
    }

    if (vector_addr >= (partition->start_addr + partition->size)) {
        return false;
    }

    /*
     * TODO:
     * 1. 检查初始 MSP 是否落在 SRAM 范围。
     * 2. 检查 Reset_Handler 是否落在当前镜像分区。
     * 3. 检查 vector_addr 是否按 Cortex-M 向量表要求对齐。
     */
    return true;
}

bool boot_image_header_is_valid(const boot_image_header_t *header,
                                boot_image_type_t expected_type,
                                const boot_partition_entry_t *partition)
{
    if ((header == NULL) || (partition == NULL)) {
        return false;
    }

    if (header->magic != BOOT_IMAGE_MAGIC) {
        return false;
    }

    if (header->image_type != (uint32_t)expected_type) {
        return false;
    }

    if (header->image_size > partition->size) {
        return false;
    }

    if (!boot_image_vector_is_valid(header->vector_addr, partition)) {
        return false;
    }

    /*
     * TODO:
     * 1. 校验 header_crc。
     * 2. 校验 image_crc。
     * 3. 校验版本策略，例如禁止回滚到过旧版本。
     */
    return true;
}

void boot_image_jump(uint32_t vector_addr)
{
    /*
     * TODO:
     * 这里后续放 Cortex-M 跳转流程：
     * 1. 关闭中断。
     * 2. 停止 SysTick 和外设中断。
     * 3. 设置 SCB->VTOR = vector_addr。
     * 4. 设置 MSP = *(uint32_t *)vector_addr。
     * 5. 跳转 Reset_Handler = *(uint32_t *)(vector_addr + 4)。
     *
     * 该文件暂时不 include CMSIS，避免框架阶段影响现有 App 编译。
     */
    (void)vector_addr;
}
