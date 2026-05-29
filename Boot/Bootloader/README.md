# Bootloader

Bootloader 由 Stage0 启动，支持 A/B 主备分区。

主要职责：

- 根据分区表定位 App_A/B
- 根据 BootControl 对 App_A/B 做三次失败回退
- 支持升级 App 分区
- 支持升级 Bootloader 备用分区
- 支持升级 Partition Table
- 提供最小 CLI，例如 `show`、`upgrade`、`confirm`、`rollback`

Bootloader 自己启动成功后，需要确认：

```c
boot_confirm_bootloader();
```

App 启动成功后，也需要确认：

```c
boot_confirm_app();
```

未确认的 pending 镜像连续启动三次失败后，会自动回退到另一个可用分区。
