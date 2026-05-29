#ifndef BOOT_CONTROL_H
#define BOOT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/*
 * BootControl 保存“当前启动谁、谁是待确认镜像、失败次数是多少”。
 *
 * Stage0 负责 Bootloader_A/B 的三次失败回退。
 * Bootloader 负责 App_A/B 的三次失败回退。
 *
 * 任何 pending 镜像启动成功后，都必须由运行中的镜像主动 confirm。
 * 如果连续 3 次未 confirm，则启动链回退到上一个 confirmed/valid slot。
 */

#define BOOT_CONTROL_MAGIC       0x4243544Cu /* 'BCTL' */
#define BOOT_MAX_TRY_COUNT       3u
#define BOOT_SLOT_NONE           0xffffffffu

typedef enum {
    BOOT_SLOT_A = 0,
    BOOT_SLOT_B = 1,
} boot_slot_t;

typedef enum {
    BOOT_SLOT_STATE_EMPTY = 0,
    BOOT_SLOT_STATE_VALID,
    BOOT_SLOT_STATE_PENDING,
    BOOT_SLOT_STATE_CONFIRMED,
    BOOT_SLOT_STATE_BAD,
} boot_slot_state_t;

typedef struct {
    uint32_t active_slot;
    uint32_t pending_slot;
    uint32_t confirmed_slot;
    uint32_t try_count;
    uint32_t slot_a_state;
    uint32_t slot_b_state;
    uint32_t slot_a_version;
    uint32_t slot_b_version;
} boot_slot_control_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t seq;

    boot_slot_control_t bootloader;
    boot_slot_control_t app;

    uint32_t active_partition_seq;
    uint32_t pending_partition_seq;
    uint32_t partition_try_count;

    uint32_t crc;
} boot_control_t;

bool boot_control_is_valid(const boot_control_t *control);
const boot_control_t *boot_control_select_active(const boot_control_t *control0,
                                                 const boot_control_t *control1);
uint32_t boot_control_select_slot(boot_slot_control_t *control);
void boot_control_mark_confirmed(boot_slot_control_t *control, uint32_t slot);
void boot_control_mark_bad(boot_slot_control_t *control, uint32_t slot);

#endif /* BOOT_CONTROL_H */
