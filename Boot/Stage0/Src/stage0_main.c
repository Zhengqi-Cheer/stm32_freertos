#include "boot_control.h"
#include "boot_image.h"
#include "boot_partition.h"

/*
 * Stage0 框架入口。
 *
 * 该文件暂不接入当前 App 的 Makefile。后续应为 Stage0 单独准备：
 * 1. 独立 linker script，固定 ORIGIN = 0x08000000。
 * 2. 极小启动文件和 SystemInit。
 * 3. 只编译 Boot/Common 中必要文件。
 */

int stage0_main(void)
{
    /*
     * TODO:
     * 1. 从固定地址读取 PartitionTable_0/1。
     * 2. boot_partition_select_active() 选择有效分区表。
     * 3. 根据分区表找到 BootControl_0/1。
     * 4. boot_control_select_active() 选择有效 BootControl。
     * 5. boot_control_select_slot(&control->bootloader) 选择 Bootloader slot。
     * 6. 校验镜像头和 CRC。
     * 7. 写回 try_count。
     * 8. boot_image_jump() 跳转 Bootloader。
     * 9. 如果 A/B 都不可用，进入最小恢复模式。
     */
    return 0;
}
