# Bootloader

最小串口 Bootloader，放在 Bootloader_A（`0x08008000`）。复位后由 Stage0 跳过来。

## 构建

```sh
./build.sh boot      # Stage0 + boot_a + build/boot_combined.bin
./build.sh app_a     # App 链到 App_A，供 load/jump 使用
```

烧录 `build/boot_combined.bin` 到 `0x08000000`。

USART1（PA9 TX / PA10 RX），115200 8N1。

## 命令

上电打印地图，等待 3 秒：有按键则留下，否则若 App_A 向量有效就跳转。

| 命令 | 作用 |
| --- | --- |
| `help` | 命令列表 |
| `info` | 分区地址和 App 是否有效 |
| `erase` | 擦除 App_A |
| `load` | XMODEM-CRC 接收 `.bin` 写到 App_A |

XMODEM 实现：`Boot/Bootloader/Src/bootloader_xmodem.c`。
| `jump` | 跳到 App_A |
| `reboot` | 复位 |

发送固件示例：

```sh
sx --xmodem build/app_a/app_a.bin < /dev/ttyUSB0 > /dev/ttyUSB0
```

必须使用 `./build.sh app_a` 编出来的镜像。当前默认 App 链在 `0x08000000`，不能直接 `load` 后跳转。
