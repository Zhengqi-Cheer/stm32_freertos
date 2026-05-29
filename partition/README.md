# Partition Layout

平台目录下的 `partition.yaml` 是后续分区布局的唯一源配置。

当前默认平台配置位于：

```text
platform/stm32f103xe/partition.yaml
```

当前配置方式：

- `flash` 描述 Flash 起始地址、总大小、擦除粒度。
- `fixed` 只保存必须固定的入口区，例如 `stage0`。
- `layout` 只描述后续分区的顺序、类型、大小。
- 地址由 `tools/gen_partition.py` 自动计算。

后续建议增加生成脚本，把它转换成：

- `generated/partition_table.bin`
- `generated/partition_config.h`
- `generated/stage0.ld`
- `generated/boot_a.ld`
- `generated/boot_b.ld`
- `generated/app_a.ld`
- `generated/app_b.ld`

分区调整原则：

- `Stage0` 和 `PartitionTable_0/1` 固定，不随普通升级改变。
- Bootloader 和 App 分区允许调整，但调整后对应镜像必须重新链接。
- 分区表自身也走 A/B + seq + crc，升级失败可以回退。
- 新分区表必须通过重叠检查，禁止覆盖 Stage0 和当前正在执行的关键区域。

生成命令：

```sh
bash build.sh partition
```
