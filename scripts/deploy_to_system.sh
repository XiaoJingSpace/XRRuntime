#!/bin/bash
# XR Runtime 系统部署脚本
# 用途: 将编译好的 libxrruntime.so 推送到 Android 系统分区

set -e

# 配置
SOURCE_FILE="app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so"
TARGET_BASE="/system/lib64"
TARGET_FILE="$TARGET_BASE/libxrruntime.so"

# 检查参数
if [ "$1" = "--vendor" ] || [ "$1" = "-v" ]; then
    TARGET_BASE="/vendor/lib64"
    TARGET_FILE="$TARGET_BASE/libxrruntime.so"
fi

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "XR Runtime 系统部署脚本"
    echo ""
    echo "用法:"
    echo "  $0              # 推送到 /system/lib64"
    echo "  $0 --vendor     # 推送到 /vendor/lib64"
    echo ""
    echo "前提条件:"
    echo "  - 设备已连接并启用 USB 调试"
    echo "  - 设备已 root"
    echo "  - 已编译生成 libxrruntime.so"
    exit 0
fi

echo "========================================"
echo "XR Runtime 系统部署工具"
echo "========================================"
echo ""

# 1. 检查源文件
echo "[1/6] 检查源文件..."
if [ ! -f "$SOURCE_FILE" ]; then
    echo "错误: 找不到源文件 $SOURCE_FILE"
    echo "请先编译项目: ./gradlew assembleDebug"
    exit 1
fi

FILE_SIZE=$(du -h "$SOURCE_FILE" | cut -f1)
echo "✓ 找到源文件: $SOURCE_FILE ($FILE_SIZE)"
echo ""

# 2. 检查 ADB 连接
echo "[2/6] 检查设备连接..."
DEVICES=$(adb devices | grep -w "device$" | wc -l)
if [ "$DEVICES" -eq 0 ]; then
    echo "错误: 未找到已连接的设备"
    echo "请确保设备已连接并启用 USB 调试"
    exit 1
fi
echo "✓ 设备已连接"
echo ""

# 3. 获取 root 权限
echo "[3/6] 获取 root 权限..."
if ! adb root > /dev/null 2>&1; then
    echo "错误: 无法获取 root 权限"
    exit 1
fi
sleep 2  # 等待 root 生效
echo "✓ Root 权限已获取"
echo ""

# 4. 重新挂载分区
echo "[4/6] 重新挂载分区为可写..."
MOUNT_POINT=$(echo "$TARGET_BASE" | cut -d'/' -f2)
if ! adb remount > /dev/null 2>&1; then
    echo "警告: remount 可能失败，尝试手动挂载..."
    adb shell "mount -o remount,rw /$MOUNT_POINT" > /dev/null 2>&1 || true
fi
echo "✓ 分区已挂载为可写"
echo ""

# 5. 推送文件
echo "[5/6] 推送库文件到设备..."
echo "  源: $SOURCE_FILE"
echo "  目标: $TARGET_FILE"

if ! adb push "$SOURCE_FILE" "$TARGET_FILE" > /dev/null 2>&1; then
    echo "错误: 推送失败"
    exit 1
fi
echo "✓ 文件推送成功"
echo ""

# 6. 设置权限和所有权
echo "[6/6] 设置文件权限..."
adb shell "chmod 644 $TARGET_FILE" > /dev/null 2>&1
adb shell "chown root:root $TARGET_FILE" > /dev/null 2>&1

# 设置 SELinux 上下文（如果支持）
SELINUX_MODE=$(adb shell "getenforce" 2>/dev/null || echo "Permissive")
if [ "$SELINUX_MODE" = "Enforcing" ]; then
    echo "  检测到 SELinux Enforcing 模式，设置上下文..."
    if [ "$TARGET_BASE" = "/vendor/lib64" ]; then
        CONTEXT="u:object_r:vendor_lib_file:s0"
    else
        CONTEXT="u:object_r:system_lib_file:s0"
    fi
    adb shell "chcon $CONTEXT $TARGET_FILE" > /dev/null 2>&1 || true
fi

echo "✓ 权限设置完成"
echo ""

# 7. 验证部署
echo "验证部署..."
VERIFY=$(adb shell "ls -la $TARGET_FILE" 2>&1)
if echo "$VERIFY" | grep -q "libxrruntime.so"; then
    echo "✓ 文件验证成功"
    echo ""
    echo "文件信息:"
    echo "$VERIFY"
else
    echo "警告: 文件验证失败"
    echo "输出: $VERIFY"
fi

# 8. 重新挂载为只读
echo ""
echo "重新挂载分区为只读..."
adb shell "mount -o remount,ro /$MOUNT_POINT" > /dev/null 2>&1 || true
echo "✓ 分区已重新挂载为只读"
echo ""

# 完成
echo "========================================"
echo "部署完成！"
echo "========================================"
echo ""
echo "库文件已安装到: $TARGET_FILE"
echo ""
echo "下一步:"
echo "1. 重启设备使更改生效: adb reboot"
echo "2. 查看日志验证: adb logcat | grep -i xrruntime"
echo ""

