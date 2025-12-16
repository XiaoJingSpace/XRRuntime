# 第15章：运行时库文件部署

## 15.1 应用级部署

### jniLibs 目录配置

#### 目录结构

将编译好的 `libxrruntime.so` 文件复制到应用的 `jniLibs` 目录：

```
your-app/
└── app/
    └── src/
        └── main/
            └── jniLibs/
                └── arm64-v8a/
                    └── libxrruntime.so
```

> **截图位置**: 此处应添加 Android Studio 项目结构截图，显示 jniLibs 目录结构

#### 创建目录

**Windows:**
```powershell
New-Item -ItemType Directory -Force -Path "app\src\main\jniLibs\arm64-v8a"
Copy-Item "path\to\libxrruntime.so" "app\src\main\jniLibs\arm64-v8a\"
```

**Linux/macOS:**
```bash
mkdir -p app/src/main/jniLibs/arm64-v8a
cp path/to/libxrruntime.so app/src/main/jniLibs/arm64-v8a/
```

### APK 打包集成

#### build.gradle 配置

确保在 `app/build.gradle` 中配置了正确的 ABI：

```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a'
        }
    }
}
```

#### 打包验证

构建 APK 后，可以验证 `.so` 文件是否包含：

**Windows:**
```powershell
# 解压 APK（APK 是 ZIP 格式）
Expand-Archive -Path app-debug.apk -DestinationPath apk_extracted -Force

# 检查 .so 文件
Test-Path apk_extracted\lib\arm64-v8a\libxrruntime.so
```

**Linux/macOS:**
```bash
# 解压 APK
unzip -q app-debug.apk -d apk_extracted

# 检查 .so 文件
test -f apk_extracted/lib/arm64-v8a/libxrruntime.so && echo "文件存在"
```

### 库加载机制

#### Java 代码加载

在 Java/Kotlin 代码中加载库：

```java
public class MainActivity extends Activity {
    static {
        try {
            // 加载 XR Runtime 库
            System.loadLibrary("xrruntime");
            // 注意：库名是 "xrruntime"，不需要 "lib" 前缀和 ".so" 后缀
            Log.i("XR", "XR Runtime library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e("XR", "Failed to load XR Runtime library", e);
        }
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 库已加载，可以使用 OpenXR API
    }
}
```

#### Kotlin 代码加载

```kotlin
class MainActivity : Activity() {
    companion object {
        init {
            try {
                System.loadLibrary("xrruntime")
                Log.i("XR", "XR Runtime library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e("XR", "Failed to load XR Runtime library", e)
            }
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }
}
```

#### 加载时机

- **推荐**: 在静态初始化块中加载（如上例）
- **备选**: 在 `Application.onCreate()` 中加载
- **避免**: 在需要时才加载（可能导致延迟）

## 15.2 系统级部署

### 推送到 /system/lib64

#### 前提条件

- Root 权限
- 设备已解锁 Bootloader（某些设备需要）
- ADB 已连接

#### 部署步骤

**Windows PowerShell:**
```powershell
# 1. 获取 root 权限
adb root

# 2. 重新挂载系统分区为可写
adb remount

# 3. 推送 .so 文件
$soFile = "app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so"
adb push $soFile /system/lib64/libxrruntime.so

# 4. 设置权限
adb shell chmod 644 /system/lib64/libxrruntime.so
adb shell chown root:root /system/lib64/libxrruntime.so

# 5. 设置 SELinux 上下文
adb shell chcon u:object_r:system_lib_file:s0 /system/lib64/libxrruntime.so

# 6. 重新挂载为只读
adb shell mount -o remount,ro /system

# 7. 重启设备
adb reboot
```

**Linux/macOS:**
```bash
# 1. 获取 root 权限
adb root

# 2. 重新挂载系统分区
adb remount

# 3. 推送 .so 文件
adb push app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so /system/lib64/libxrruntime.so

# 4. 设置权限
adb shell chmod 644 /system/lib64/libxrruntime.so
adb shell chown root:root /system/lib64/libxrruntime.so

# 5. 设置 SELinux 上下文
adb shell chcon u:object_r:system_lib_file:s0 /system/lib64/libxrruntime.so

# 6. 重新挂载为只读
adb shell mount -o remount,ro /system

# 7. 重启设备
adb reboot
```

### 推送到 /vendor/lib64

#### 适用场景

- 厂商分区部署
- 需要系统签名
- 更严格的权限控制

#### 部署步骤

```bash
# 1. 获取 root 权限
adb root

# 2. 重新挂载 vendor 分区
adb remount

# 3. 推送文件
adb push libxrruntime.so /vendor/lib64/libxrruntime.so

# 4. 设置权限
adb shell chmod 644 /vendor/lib64/libxrruntime.so
adb shell chown root:root /vendor/lib64/libxrruntime.so

# 5. 设置 SELinux 上下文
adb shell chcon u:object_r:vendor_lib_file:s0 /vendor/lib64/libxrruntime.so

# 6. 重新挂载为只读
adb shell mount -o remount,ro /vendor

# 7. 重启设备
adb reboot
```

### 权限与 SELinux 配置

#### 文件权限

- **644**: `rw-r--r--` - 所有者可读写，其他用户只读
- **所有者**: `root:root` - 系统用户和组

#### SELinux 上下文

- **系统库**: `u:object_r:system_lib_file:s0`
- **厂商库**: `u:object_r:vendor_lib_file:s0`

#### 验证权限

```bash
adb shell ls -laZ /system/lib64/libxrruntime.so
```

应该显示类似：
```
-rw-r--r-- 1 root root u:object_r:system_lib_file:s0 libxrruntime.so
```

## 15.3 系统镜像集成

### Android.mk 配置

#### 创建 Android.mk

在 AOSP 源码树中创建 `device/your-vendor/your-device/libxrruntime/Android.mk`:

```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libxrruntime
LOCAL_SRC_FILES := libxrruntime.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := false
LOCAL_MODULE_OWNER := your-vendor
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
```

#### 添加到设备配置

在 `device/your-vendor/your-device/device.mk` 中添加：

```makefile
# XR Runtime
PRODUCT_PACKAGES += \
    libxrruntime
```

### Android.bp 配置

#### 创建 Android.bp

创建 `device/your-vendor/your-device/libxrruntime/Android.bp`:

```bp
cc_prebuilt_library_shared {
    name: "libxrruntime",
    vendor: true,
    owner: "your-vendor",
    srcs: ["libxrruntime.so"],
    compile_multilib: "64",
    strip: {
        none: true,
    },
}
```

#### 添加到设备配置

在 `device/your-vendor/your-device/device.mk` 中添加：

```makefile
PRODUCT_PACKAGES += libxrruntime
```

### AOSP 集成方法

#### 完整集成步骤

1. **准备库文件**
   ```bash
   # 将编译好的 .so 文件复制到设备配置目录
   cp libxrruntime.so device/your-vendor/your-device/libxrruntime/
   ```

2. **创建构建文件**
   - 创建 `Android.mk` 或 `Android.bp`
   - 添加到设备配置

3. **编译系统镜像**
   ```bash
   source build/envsetup.sh
   lunch your_device-userdebug
   m libxrruntime
   ```

4. **刷入设备**
   ```bash
   fastboot flash system system.img
   ```

## 15.4 部署脚本使用

### Windows 部署脚本

#### 系统部署脚本 (deploy_to_system.ps1)

项目提供了完整的 PowerShell 部署脚本：

```powershell
# 使用方法
.\scripts\deploy_to_system.ps1

# 推送到厂商分区
.\scripts\deploy_to_system.ps1 -Vendor

# 查看帮助
.\scripts\deploy_to_system.ps1 -Help
```

脚本功能：
- ✅ 检查设备连接
- ✅ 获取 root 权限
- ✅ 推送文件到系统分区
- ✅ 设置正确的权限和 SELinux 上下文
- ✅ 验证部署

### Linux/macOS 部署脚本

#### 系统部署脚本 (deploy_to_system.sh)

```bash
# 添加执行权限
chmod +x scripts/deploy_to_system.sh

# 推送到系统分区
./scripts/deploy_to_system.sh

# 推送到厂商分区
./scripts/deploy_to_system.sh --vendor
```

### 自动化部署流程

#### CI/CD 集成

可以在 CI/CD 流程中自动部署：

```yaml
# GitHub Actions 示例
- name: Deploy to device
  run: |
    adb root
    adb remount
    adb push app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so /system/lib64/
    adb shell chmod 644 /system/lib64/libxrruntime.so
    adb shell chown root:root /system/lib64/libxrruntime.so
    adb reboot
```

## 部署验证

### 检查文件存在

```bash
adb shell ls -la /system/lib64/libxrruntime.so
```

### 检查权限

```bash
adb shell ls -laZ /system/lib64/libxrruntime.so
```

### 检查库依赖

```bash
adb shell "cd /data/local/tmp && ldd /system/lib64/libxrruntime.so"
```

### 运行时验证

查看日志确认库已加载：

```bash
adb logcat | grep -i "xrruntime"
```

应该看到：
```
I/XRRuntime: XRRuntime JNI_OnLoad called
I/XRRuntime: Initializing XR Runtime for Qualcomm XR2
I/XRRuntime: XR Runtime initialized successfully
```

## 常见问题

### 问题1: 权限不足

**症状**: `adb remount` 失败

**解决方案**:
1. 确保设备已 root
2. 检查设备是否支持 remount
3. 某些设备需要解锁 Bootloader

### 问题2: SELinux 阻止加载

**症状**: 库文件存在但无法加载

**解决方案**:
1. 检查 SELinux 上下文是否正确
2. 查看 SELinux 日志: `adb shell dmesg | grep avc`
3. 可能需要修改 SELinux 策略

### 问题3: 库版本不匹配

**症状**: 应用无法找到库或版本错误

**解决方案**:
1. 确认库文件架构匹配（arm64-v8a）
2. 检查库文件完整性
3. 重新编译并部署

## 本章小结

本章详细介绍了运行时库文件的三种部署方式：应用级部署、系统级部署和系统镜像集成，以及相应的部署脚本使用方法。选择合适的部署方式取决于具体的使用场景。

## 下一步

- [第16章：OpenXR Loader 配置](chapter16.md)

