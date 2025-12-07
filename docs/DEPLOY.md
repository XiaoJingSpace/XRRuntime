# 部署指南

本文档描述如何将编译好的 XR Runtime 部署到 Android 设备上。

## 前置要求

1. 已编译的 APK 或 SO 库
2. Android 设备（高通 XR2 平台）
3. USB 调试已启用
4. ADB 工具已安装

## 部署方法

### 方法 1: 通过 APK 安装（推荐）

#### 步骤 1: 编译 APK

```bash
./gradlew assembleDebug
# 或
./gradlew assembleRelease
```

#### 步骤 2: 安装 APK

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

#### 步骤 3: 配置 OpenXR Loader

创建 OpenXR Loader 配置文件：

```bash
PACKAGE_NAME="com.xrruntime"
CONFIG_PATH="/sdcard/Android/data/$PACKAGE_NAME/files/openxr_loader.json"

cat > /tmp/openxr_loader.json << EOF
{
  "runtime": {
    "library_path": "/data/data/$PACKAGE_NAME/lib/arm64/libxrruntime.so",
    "name": "Custom XR2 Runtime",
    "api_version": "1.1.0"
  }
}
EOF

adb push /tmp/openxr_loader.json "$CONFIG_PATH"
```

#### 步骤 4: 验证安装

```bash
adb shell pm list packages | grep xrruntime
adb shell ls -l /data/data/com.xrruntime/lib/arm64/libxrruntime.so
```

### 方法 2: 使用部署脚本

```bash
chmod +x scripts/deploy.sh
./scripts/deploy.sh [device_serial]
```

脚本会自动：
1. 检查设备连接
2. 编译 APK（如果不存在）
3. 安装 APK
4. 推送 OpenXR Loader 配置

### 方法 3: 手动部署 SO 库（需要 Root）

如果设备已 Root，可以直接复制 SO 库：

```bash
# 推送到设备
adb push app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so /sdcard/

# 复制到系统目录（需要 root）
adb shell su -c "cp /sdcard/libxrruntime.so /system/lib64/"
adb shell su -c "chmod 644 /system/lib64/libxrruntime.so"
adb shell su -c "chown root:root /system/lib64/libxrruntime.so"
```

## OpenXR Loader 配置

OpenXR Loader 需要知道 Runtime 的位置。有几种配置方法：

### 方法 1: 应用数据目录（推荐）

配置文件位置：
```
/sdcard/Android/data/<package_name>/files/openxr_loader.json
```

### 方法 2: 系统配置（需要 Root）

配置文件位置：
```
/system/etc/openxr_loader.json
```

### 方法 3: 环境变量

设置环境变量：
```bash
adb shell setprop openxr.runtime.library_path /path/to/libxrruntime.so
```

## 启动 Runtime

### 方法 1: 通过 Android Service

```bash
adb shell am startservice com.xrruntime/.XRRuntimeService
```

### 方法 2: 通过 OpenXR 应用

当 OpenXR 应用调用 `xrCreateInstance` 时，Loader 会自动加载 Runtime。

### 方法 3: 测试应用

创建一个简单的测试应用：

```java
import org.khronos.openxr.*;

public class XRTest {
    static {
        System.loadLibrary("openxr_loader");
    }
    
    public static void main(String[] args) {
        XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
        // ... 配置 createInfo
        XrInstance instance = new XrInstance();
        int result = XR.xrCreateInstance(createInfo, instance);
        // ...
    }
}
```

## 验证部署

### 1. 检查 SO 库

```bash
adb shell ls -l /data/data/com.xrruntime/lib/arm64/libxrruntime.so
```

应该显示文件存在且有执行权限。

### 2. 检查配置文件

```bash
adb shell cat /sdcard/Android/data/com.xrruntime/files/openxr_loader.json
```

### 3. 检查日志

```bash
adb logcat | grep XRRuntime
```

应该看到 Runtime 初始化的日志。

### 4. 检查进程

```bash
adb shell ps | grep xrruntime
```

## 卸载

### 卸载 APK

```bash
adb uninstall com.xrruntime
```

### 清理配置文件

```bash
adb shell rm /sdcard/Android/data/com.xrruntime/files/openxr_loader.json
```

### 清理系统库（如果手动安装）

```bash
adb shell su -c "rm /system/lib64/libxrruntime.so"
```

## 多设备部署

如果有多个设备，可以指定设备序列号：

```bash
adb -s <device_serial> install -r app-debug.apk
```

查看设备列表：
```bash
adb devices
```

## 常见问题

### 1. 权限被拒绝

**错误**: `Permission denied`

**解决**: 
- 确保 USB 调试已启用
- 检查设备是否授权了此计算机
- 对于系统目录操作，需要 Root 权限

### 2. SO 库未找到

**错误**: `cannot locate symbol` 或 `library not found`

**解决**:
- 检查 SO 库路径是否正确
- 检查 ABI 是否匹配（arm64-v8a）
- 检查 OpenXR Loader 配置是否正确

### 3. Runtime 未加载

**错误**: OpenXR 应用无法找到 Runtime

**解决**:
- 检查 OpenXR Loader 配置文件路径和内容
- 检查 SO 库权限
- 查看 logcat 日志获取详细错误信息

### 4. 版本不匹配

**错误**: API 版本不匹配

**解决**:
- 确保 Runtime 和 Loader 的 OpenXR 版本兼容
- 检查 `api_version` 配置

## 生产部署

对于生产环境部署：

1. **使用 Release 构建**
   ```bash
   ./gradlew assembleRelease
   ```

2. **签名 APK**
   ```bash
   jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
     -keystore my-release-key.jks app-release-unsigned.apk alias_name
   ```

3. **对齐 APK**
   ```bash
   zipalign -v 4 app-release-unsigned.apk app-release.apk
   ```

4. **分发配置**
   - 通过应用商店分发
   - 或通过 OTA 更新分发

## 下一步

部署完成后，参考 [DEBUG.md](DEBUG.md) 进行调试和测试。

