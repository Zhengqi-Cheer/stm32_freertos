# Partition Layout

| Name | Type | Address | Size | End |
| --- | --- | ---: | ---: | ---: |
| stage0 | stage0 | 0x08000000 | 16384 | 0x08004000 |
| partition_table_0 | partition_table | 0x08004000 | 4096 | 0x08005000 |
| partition_table_1 | partition_table | 0x08005000 | 4096 | 0x08006000 |
| bootctrl_0 | boot_control | 0x08006000 | 4096 | 0x08007000 |
| bootctrl_1 | boot_control | 0x08007000 | 4096 | 0x08008000 |
| boot_a | bootloader | 0x08008000 | 49152 | 0x08014000 |
| boot_b | bootloader | 0x08014000 | 49152 | 0x08020000 |
| app_a | app | 0x08020000 | 163840 | 0x08048000 |
| app_b | app | 0x08048000 | 163840 | 0x08070000 |
| config | config | 0x08070000 | 65536 | 0x08080000 |
