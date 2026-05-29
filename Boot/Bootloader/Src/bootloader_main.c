#include "boot_control.h"
#include "boot_image.h"
#include "boot_partition.h"
#include "boot_platform.h"
#include "partition_config.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Bootloader abstract boot flow.
 *
 * Responsibilities:
 * 1. Confirm the running Bootloader slot when it is a pending image.
 * 2. Load PartitionTable and BootControl.
 * 3. Select App_A/App_B with pending/confirmed/rollback semantics.
 * 4. Validate the selected App image.
 * 5. Persist BootControl updates before jumping.
 * 6. Jump to App or stay in recovery mode.
 *
 * This file is platform independent. It must not include STM32 HAL headers.
 */

#define BOOTLOADER_RECOVERY_INVALID_TABLE      1u
#define BOOTLOADER_RECOVERY_INVALID_CONTROL    2u
#define BOOTLOADER_RECOVERY_NO_APP             3u

typedef struct {
    boot_control_t control;
    uint32_t active_control_addr;
    bool control_loaded_from_flash;
} bootloader_context_t;

int bootloader_main(void);

static const boot_partition_table_t g_fallback_partition_table = {
    BOOT_PARTITION_MAGIC,
    1u,
    0u,
    sizeof(boot_partition_table_t),
    10u,
    0u,
    0u,
    0u,
    {
        {
            BOOT_PART_ID_STAGE0,
            BOOT_PART_TYPE_STAGE0,
            0u,
            PARTITION_STAGE0_ADDR,
            PARTITION_STAGE0_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_TABLE_0,
            BOOT_PART_TYPE_PARTITION_TABLE,
            0u,
            PARTITION_PARTITION_TABLE_0_ADDR,
            PARTITION_PARTITION_TABLE_0_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_TABLE_1,
            BOOT_PART_TYPE_PARTITION_TABLE,
            0u,
            PARTITION_PARTITION_TABLE_1_ADDR,
            PARTITION_PARTITION_TABLE_1_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_BOOTCTRL_0,
            BOOT_PART_TYPE_BOOT_CONTROL,
            0u,
            PARTITION_BOOTCTRL_0_ADDR,
            PARTITION_BOOTCTRL_0_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_BOOTCTRL_1,
            BOOT_PART_TYPE_BOOT_CONTROL,
            0u,
            PARTITION_BOOTCTRL_1_ADDR,
            PARTITION_BOOTCTRL_1_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_BOOT_A,
            BOOT_PART_TYPE_BOOTLOADER,
            0u,
            PARTITION_BOOT_A_ADDR,
            PARTITION_BOOT_A_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_BOOT_B,
            BOOT_PART_TYPE_BOOTLOADER,
            0u,
            PARTITION_BOOT_B_ADDR,
            PARTITION_BOOT_B_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_APP_A,
            BOOT_PART_TYPE_APP,
            0u,
            PARTITION_APP_A_ADDR,
            PARTITION_APP_A_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_APP_B,
            BOOT_PART_TYPE_APP,
            0u,
            PARTITION_APP_B_ADDR,
            PARTITION_APP_B_SIZE,
            0u,
            0u,
            0u,
        },
        {
            BOOT_PART_ID_CONFIG,
            BOOT_PART_TYPE_CONFIG,
            0u,
            PARTITION_CONFIG_ADDR,
            PARTITION_CONFIG_SIZE,
            0u,
            0u,
            0u,
        },
    },
};

static const boot_partition_table_t *bootloader_load_partition_table(void)
{
    const boot_partition_table_t *table0 =
        (const boot_partition_table_t *)PARTITION_PARTITION_TABLE_0_ADDR;
    const boot_partition_table_t *table1 =
        (const boot_partition_table_t *)PARTITION_PARTITION_TABLE_1_ADDR;
    const boot_partition_table_t *active = boot_partition_select_active(table0, table1);

    /*
     * Early development fallback:
     * The partition generator currently emits C macros, not a programmed
     * PartitionTable binary. Until table flashing is implemented, use a table
     * built from generated addresses.
     */
    if (active == NULL) {
        active = &g_fallback_partition_table;
    }

    return active;
}

static uint32_t bootloader_current_slot(void)
{
    const uint32_t self = (uint32_t)(uintptr_t)&bootloader_main;

    if ((self >= PARTITION_BOOT_A_ADDR) && (self < PARTITION_BOOT_A_END_ADDR)) {
        return BOOT_SLOT_A;
    }

    if ((self >= PARTITION_BOOT_B_ADDR) && (self < PARTITION_BOOT_B_END_ADDR)) {
        return BOOT_SLOT_B;
    }

    return BOOT_SLOT_NONE;
}

static void bootloader_default_control(boot_control_t *control)
{
    memset(control, 0, sizeof(*control));

    control->magic = BOOT_CONTROL_MAGIC;
    control->version = 1u;
    control->seq = 0u;

    control->bootloader.active_slot = BOOT_SLOT_A;
    control->bootloader.pending_slot = BOOT_SLOT_NONE;
    control->bootloader.confirmed_slot = BOOT_SLOT_A;
    control->bootloader.slot_a_state = BOOT_SLOT_STATE_CONFIRMED;
    control->bootloader.slot_b_state = BOOT_SLOT_STATE_EMPTY;

    control->app.active_slot = BOOT_SLOT_A;
    control->app.pending_slot = BOOT_SLOT_NONE;
    control->app.confirmed_slot = BOOT_SLOT_A;
    control->app.slot_a_state = BOOT_SLOT_STATE_CONFIRMED;
    control->app.slot_b_state = BOOT_SLOT_STATE_EMPTY;
}

static void bootloader_update_control_crc(boot_control_t *control)
{
    control->crc = 0u;
    control->crc = boot_platform_crc32(control, sizeof(*control) - sizeof(control->crc), 0u);
}

static bool bootloader_load_control(bootloader_context_t *context)
{
    const boot_control_t *control0 = (const boot_control_t *)PARTITION_BOOTCTRL_0_ADDR;
    const boot_control_t *control1 = (const boot_control_t *)PARTITION_BOOTCTRL_1_ADDR;
    const boot_control_t *active = boot_control_select_active(control0, control1);

    memset(context, 0, sizeof(*context));
    context->active_control_addr = PARTITION_BOOTCTRL_0_ADDR;

    if (active != NULL) {
        memcpy(&context->control, active, sizeof(context->control));
        context->active_control_addr =
            (active == control1) ? PARTITION_BOOTCTRL_1_ADDR : PARTITION_BOOTCTRL_0_ADDR;
        context->control_loaded_from_flash = true;
        return true;
    }

    /*
     * If both BootControl copies are empty on first boot, create a conservative
     * default. This lets initial factory images boot before BootControl flashing
     * tooling exists.
     */
    bootloader_default_control(&context->control);
    bootloader_update_control_crc(&context->control);
    context->control_loaded_from_flash = false;
    return true;
}

static bool bootloader_save_control(const bootloader_context_t *context)
{
    boot_control_t next = context->control;
    uint32_t target_addr = PARTITION_BOOTCTRL_0_ADDR;

    if (context->active_control_addr == PARTITION_BOOTCTRL_0_ADDR) {
        target_addr = PARTITION_BOOTCTRL_1_ADDR;
    }

    next.seq++;
    bootloader_update_control_crc(&next);

    /*
     * BootControl uses copy-on-write:
     * write the inactive page with seq+1, then the next boot selects it.
     * The old copy remains available if power is lost mid-write.
     */
    if (boot_platform_flash_erase(target_addr, PARTITION_BOOTCTRL_0_SIZE) != BOOT_PLATFORM_OK) {
        return false;
    }

    if (boot_platform_flash_write(target_addr, &next, sizeof(next)) != BOOT_PLATFORM_OK) {
        return false;
    }

    return true;
}

static void bootloader_confirm_running_bootloader(bootloader_context_t *context)
{
    const uint32_t current_slot = bootloader_current_slot();

    if (current_slot == BOOT_SLOT_NONE) {
        return;
    }

    if (context->control.bootloader.pending_slot == current_slot) {
        boot_control_mark_confirmed(&context->control.bootloader, current_slot);
    }
}

static const boot_partition_entry_t *bootloader_find_app_partition(
    const boot_partition_table_t *table,
    uint32_t slot)
{
    if (slot == BOOT_SLOT_A) {
        return boot_partition_find(table, BOOT_PART_ID_APP_A);
    }

    if (slot == BOOT_SLOT_B) {
        return boot_partition_find(table, BOOT_PART_ID_APP_B);
    }

    return NULL;
}

static uint32_t bootloader_other_slot(uint32_t slot)
{
    if (slot == BOOT_SLOT_A) {
        return BOOT_SLOT_B;
    }

    if (slot == BOOT_SLOT_B) {
        return BOOT_SLOT_A;
    }

    return BOOT_SLOT_NONE;
}

static bool bootloader_validate_app_partition(const boot_partition_entry_t *partition)
{
    const boot_image_header_t *header = NULL;

    if (partition == NULL) {
        return false;
    }

    header = (const boot_image_header_t *)(partition->start_addr + partition->image_offset);
    return boot_image_header_is_valid(header, BOOT_IMAGE_TYPE_APP, partition);
}

static const boot_partition_entry_t *bootloader_select_app(
    const boot_partition_table_t *table,
    bootloader_context_t *context)
{
    uint32_t selected_slot = boot_control_select_slot(&context->control.app);
    const boot_partition_entry_t *selected = bootloader_find_app_partition(table, selected_slot);

    if (bootloader_validate_app_partition(selected)) {
        return selected;
    }

    if (selected_slot != BOOT_SLOT_NONE) {
        boot_control_mark_bad(&context->control.app, selected_slot);
    }

    selected_slot = bootloader_other_slot(selected_slot);
    selected = bootloader_find_app_partition(table, selected_slot);

    if (bootloader_validate_app_partition(selected)) {
        context->control.app.active_slot = selected_slot;
        context->control.app.pending_slot = BOOT_SLOT_NONE;
        context->control.app.try_count = 0u;
        return selected;
    }

    if (selected_slot != BOOT_SLOT_NONE) {
        boot_control_mark_bad(&context->control.app, selected_slot);
    }

    return NULL;
}

static void bootloader_enter_recovery(uint32_t reason)
{
    /*
     * Recovery policy is product-specific. Keep the abstract layer minimal:
     * later this can start a CLI, serial downloader, or factory image path.
     */
    (void)reason;
    while (1) {
    }
}

int bootloader_main(void)
{
    const boot_partition_table_t *table = NULL;
    const boot_partition_entry_t *app_partition = NULL;
    const boot_image_header_t *app_header = NULL;
    bootloader_context_t context;

    table = bootloader_load_partition_table();
    if (table == NULL) {
        bootloader_enter_recovery(BOOTLOADER_RECOVERY_INVALID_TABLE);
    }

    if (!bootloader_load_control(&context)) {
        bootloader_enter_recovery(BOOTLOADER_RECOVERY_INVALID_CONTROL);
    }

    bootloader_confirm_running_bootloader(&context);

    /*
     * Partition-table migration and upgrade transport are intentionally not
     * implemented here yet. This function owns the boot decision. Upgrade code
     * should write images/control data before reset, then this path validates
     * and boots the selected image.
     */
    app_partition = bootloader_select_app(table, &context);
    if (app_partition == NULL) {
        (void)bootloader_save_control(&context);
        bootloader_enter_recovery(BOOTLOADER_RECOVERY_NO_APP);
    }

    if (!bootloader_save_control(&context)) {
        bootloader_enter_recovery(BOOTLOADER_RECOVERY_INVALID_CONTROL);
    }

    app_header = (const boot_image_header_t *)(app_partition->start_addr +
                                               app_partition->image_offset);

    boot_platform_deinit_before_jump();
    boot_platform_jump_to_image(app_header->vector_addr);

    bootloader_enter_recovery(BOOTLOADER_RECOVERY_NO_APP);
    return 0;
}
