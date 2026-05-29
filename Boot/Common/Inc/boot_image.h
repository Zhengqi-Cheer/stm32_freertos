#ifndef BOOT_IMAGE_H
#define BOOT_IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "boot_partition.h"

/*
 * 每个可启动镜像都需要镜像头。
 *
 * 注意：
 * STM32F103 的固件通常不是位置无关代码，所以镜像的链接地址必须
 * 和分区表中的运行地址一致。修改分区后，需要重新生成 linker script
 * 并重新编译对应 slot 的镜像。
 */

#define BOOT_IMAGE_MAGIC        0x494D4748u /* 'IMGH' */

typedef enum {
    BOOT_IMAGE_TYPE_INVALID = 0,
    BOOT_IMAGE_TYPE_STAGE0,
    BOOT_IMAGE_TYPE_BOOTLOADER,
    BOOT_IMAGE_TYPE_APP,
} boot_image_type_t;

typedef struct {
    uint32_t magic;
    uint32_t image_type;
    uint32_t slot_id;
    uint32_t version;
    uint32_t image_size;
    uint32_t image_crc;
    uint32_t vector_addr;
    uint32_t reset_handler;
    uint32_t flags;
    uint32_t header_crc;
} boot_image_header_t;

bool boot_image_header_is_valid(const boot_image_header_t *header,
                                boot_image_type_t expected_type,
                                const boot_partition_entry_t *partition);
bool boot_image_vector_is_valid(uint32_t vector_addr,
                                const boot_partition_entry_t *partition);
void boot_image_jump(uint32_t vector_addr);

#endif /* BOOT_IMAGE_H */
