# Stage0

Stage0 是系统的固定入口，放在 `0x08000000`，普通升级流程不覆盖它。

它只负责最小启动决策：

- 读取 `PartitionTable_0/1`
- 选择 seq 最新且 CRC 正确的分区表
- 读取 `BootControl_0/1`
- 对 Bootloader_A/B 做三次失败回退
- 校验 Bootloader 镜像
- 跳转到选中的 Bootloader

Stage0 不负责：

- 下载固件
- 升级 App
- 复杂 CLI
- 业务外设初始化

这样做的目的，是保证 Bootloader 自升级失败时仍然有恢复入口。
