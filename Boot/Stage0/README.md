# Stage0

固定入口，链接在 `0x08000000`。当前最小实现只检查 Bootloader_A 的向量表，然后跳转。

普通升级不要覆盖这一区。

```sh
./build.sh stage0
./build.sh boot
```
