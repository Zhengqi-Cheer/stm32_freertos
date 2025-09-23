#!/bin/bash

# 配置
TOOLCHAIN_DIR="`pwd`"
TOOLCHAIN_NAME="gcc-arm-none-eabi-5_4-2016q3"
DOWNLOAD_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2"

# 检查是否已安装
if [ -f "$TOOLCHAIN_DIR/$TOOLCHAIN_NAME/bin/arm-none-eabi-gcc" ]; then
    echo "工具链已存在，跳过安装"
    exit 0
fi

# 创建目录
#mkdir -p "$TOOLCHAIN_DIR"/{tools,projects}

# 下载
echo "正在下载工具链..."
cd "$TOOLCHAIN_DIR/tools"
wget "$DOWNLOAD_URL" || curl -O "$DOWNLOAD_URL"

# 解压
echo "正在解压..."
tar -xjf gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2
rm gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2

# 创建环境脚本
#cat > "$TOOLCHAIN_DIR/setup_env.sh" << 'EOF'
#export PATH="$HOME/stm32_toolchain/tools/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH"
#echo "STM32工具链环境已设置"
#arm-none-eabi-gcc --version
#EOF

#chmod +x "$TOOLCHAIN_DIR/setup_env.sh"

echo "安装完成！"
#echo "使用命令: source $TOOLCHAIN_DIR/setup_env.sh"
