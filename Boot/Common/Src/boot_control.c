#include "boot_control.h"

#include <stddef.h>

bool boot_control_is_valid(const boot_control_t *control)
{
    if (control == NULL) {
        return false;
    }

    if (control->magic != BOOT_CONTROL_MAGIC) {
        return false;
    }

    /*
     * TODO:
     * 1. 校验 crc。
     * 2. 校验 active/pending/confirmed slot 是否为 A/B/NONE。
     * 3. 校验 try_count 不超过 BOOT_MAX_TRY_COUNT。
     */
    return true;
}

const boot_control_t *boot_control_select_active(const boot_control_t *control0,
                                                 const boot_control_t *control1)
{
    const bool control0_valid = boot_control_is_valid(control0);
    const bool control1_valid = boot_control_is_valid(control1);

    if (control0_valid && control1_valid) {
        return (control1->seq > control0->seq) ? control1 : control0;
    }

    if (control0_valid) {
        return control0;
    }

    if (control1_valid) {
        return control1;
    }

    return NULL;
}

uint32_t boot_control_select_slot(boot_slot_control_t *control)
{
    if (control == NULL) {
        return BOOT_SLOT_NONE;
    }

    if (control->pending_slot != BOOT_SLOT_NONE) {
        if (control->try_count < BOOT_MAX_TRY_COUNT) {
            control->try_count++;
            return control->pending_slot;
        }

        boot_control_mark_bad(control, control->pending_slot);
        control->pending_slot = BOOT_SLOT_NONE;
        control->try_count = 0;
    }

    if (control->active_slot != BOOT_SLOT_NONE) {
        return control->active_slot;
    }

    return control->confirmed_slot;
}

void boot_control_mark_confirmed(boot_slot_control_t *control, uint32_t slot)
{
    if (control == NULL) {
        return;
    }

    control->active_slot = slot;
    control->confirmed_slot = slot;
    control->pending_slot = BOOT_SLOT_NONE;
    control->try_count = 0;

    if (slot == BOOT_SLOT_A) {
        control->slot_a_state = BOOT_SLOT_STATE_CONFIRMED;
    } else if (slot == BOOT_SLOT_B) {
        control->slot_b_state = BOOT_SLOT_STATE_CONFIRMED;
    }
}

void boot_control_mark_bad(boot_slot_control_t *control, uint32_t slot)
{
    if (control == NULL) {
        return;
    }

    if (slot == BOOT_SLOT_A) {
        control->slot_a_state = BOOT_SLOT_STATE_BAD;
    } else if (slot == BOOT_SLOT_B) {
        control->slot_b_state = BOOT_SLOT_STATE_BAD;
    }
}
