# Partition Layout

平台目录下的 `inc/partition_config.h` 是 Flash 地图的唯一编译期源。

当前默认平台配置位于：

```text
platform/stm32f103xe/inc/partition_config.h
```

当前配置方式：

- `platform_config.h` 描述 Flash 起始地址、总大小、擦除粒度和写入对齐。
- `partition_config.h` 只手写 Stage0 和各分区 SIZE。
- 后续分区地址由上一个分区的 `END` 宏顺延。
- `_Static_assert` 检查擦除对齐、连续布局、不超出 Flash。

分区调整原则：

- `Stage0` 和 `PartitionTable_0/1` 固定，不随普通升级改变。
- Bootloader 和 App 分区允许调整，但调整后对应镜像必须按新地址重新链接。
- 分区表自身也走 A/B + seq + crc，升级失败可以回退。
- 链接脚本中的 ORIGIN/LENGTH 必须与这些宏一致。
