# 系统集成指南

本文档详细说明如何将 `libxrruntime.so` 集成到 Android 系统分区。

## 目录

1. [快速部署（开发测试）](#快速部署开发测试)
2. [系统镜像集成](#系统镜像集成)
3. [SELinux 配置](#selinux-配置)
4. [OpenXR Loader 配置](#openxr-loader-配置)

---

## 快速部署（开发测试）

### 使用 ADB 推送（需要 Root）

```bash
#!/bin/bash
# deploy_to_system.sh

# 1. 检查 root 权限
adb root
if [ $? -ne 0 ]; then
    echo "错误: 需要 root 权限"
    exit 1
fi

# 2. 重新挂载系统分区
adb remount

# 3. 推送库文件
echo "推送 libxrruntime.so 到系统..."
adb push app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so /system/lib64/libxrruntime.so

# 4. 设置权限
adb shell chmod 644 /system/lib64/libxrruntime.so
adb shell chown root:root /system/lib64/libxrruntime.so

# 5. 设置 SELinux 上下文（如果需要）
adb shell chcon u:object_r:system_lib_file:s0 /system/lib64/libxrruntime.so

# 6. 重新挂载为只读
adb shell mount -o remount,ro /system

echo "部署完成！请重启设备。"
```

### Windows PowerShell 脚本

```powershell
# deploy_to_system.ps1

# 1. 检查 root 权限
Write-Host "检查 root 权限..."
$rootCheck = adb root
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 需要 root 权限" -ForegroundColor Red
    exit 1
}

# 2. 重新挂载系统分区
Write-Host "重新挂载系统分区..."
adb remount

# 3. 推送库文件
$soFile = "app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so"
if (Test-Path $soFile) {
    Write-Host "推送 libxrruntime.so 到系统..."
    adb push $soFile /system/lib64/libxrruntime.so
    
    # 4. 设置权限
    adb shell chmod 644 /system/lib64/libxrruntime.so
    adb shell chown root:root /system/lib64/libxrruntime.so
    
    # 5. 设置 SELinux 上下文
    adb shell chcon u:object_r:system_lib_file:s0 /system/lib64/libxrruntime.so
    
    Write-Host "部署完成！" -ForegroundColor Green
} else {
    Write-Host "错误: 找不到 $soFile" -ForegroundColor Red
    exit 1
}

# 6. 重新挂载为只读
adb shell mount -o remount,ro /system
```

---

## 系统镜像集成

### 方式 1: 使用 Android.mk（AOSP 构建）

创建 `device/your-vendor/your-device/libxrruntime/Android.mk`:

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

### 方式 2: 使用 Android.bp（AOSP 构建）

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

### 添加到设备配置

在 `device/your-vendor/your-device/device.mk` 中添加：

```makefile
# XR Runtime
PRODUCT_PACKAGES += \
    libxrruntime
```

---

## SELinux 配置

### 创建 SELinux 策略文件

创建 `device/your-vendor/sepolicy/libxrruntime.te`:

```te
# libxrruntime - XR Runtime library
type xrruntime, domain;
type xrruntime_exec, exec_type, file_type, system_file_type;

# Allow runtime to access system properties
get_prop(xrruntime, system_prop)
get_prop(xrruntime, default_prop)

# Allow runtime to access graphics
allow xrruntime graphics_device:chr_file rw_file_perms;
allow xrruntime gpu_device:chr_file rw_file_perms;

# Allow runtime to access sensors
allow xrruntime sensors_device:chr_file rw_file_perms;

# Allow runtime to access camera
allow xrruntime camera_device:chr_file rw_file_perms;

# Allow runtime to use binder
binder_use(xrruntime)
binder_call(xrruntime, system_server)
binder_call(xrruntime, surfaceflinger)

# Allow runtime to access files
allow xrruntime system_file:file execute;
allow xrruntime system_lib_file:file execute;
```

### 文件上下文配置

在 `device/your-vendor/sepolicy/file_contexts` 中添加：

```
/system/lib64/libxrruntime\.so u:object_r:system_lib_file:s0
/vendor/lib64/libxrruntime\.so u:object_r:vendor_lib_file:s0
```

---

## OpenXR Loader 配置

### 创建 OpenXR Loader 配置文件

创建 `device/your-vendor/your-device/etc/openxr/openxr_loader.json`:

```json
{
    "file_format_version": "1.0.0",
    "runtime": [
        {
            "library_path": "/system/lib64/libxrruntime.so",
            "name": "Qualcomm XR2 Runtime",
            "api_version": "1.1.0",
            "extensions": [
                "XR_KHR_android_create_instance",
                "XR_KHR_opengl_es_enable",
                "XR_KHR_vulkan_enable"
            ]
        }
    ]
}
```

### 添加到设备配置

在 `device/your-vendor/your-device/device.mk` 中添加：

```makefile
# OpenXR Loader configuration
PRODUCT_COPY_FILES += \
    device/your-vendor/your-device/etc/openxr/openxr_loader.json:vendor/etc/openxr/openxr_loader.json
```

---

## 验证部署

### 检查库文件

```bash
# 检查文件是否存在
adb shell ls -la /system/lib64/libxrruntime.so

# 检查文件权限
adb shell stat /system/lib64/libxrruntime.so

# 检查库依赖
adb shell ldd /system/lib64/libxrruntime.so
```

### 检查 SELinux 上下文

```bash
# 检查文件上下文
adb shell ls -Z /system/lib64/libxrruntime.so

# 应该显示: u:object_r:system_lib_file:s0
```

### 检查 OpenXR 配置

```bash
# 检查配置文件
adb shell cat /vendor/etc/openxr/openxr_loader.json

# 检查配置权限
adb shell ls -la /vendor/etc/openxr/
```

### 运行时测试

```bash
# 查看 logcat
adb logcat | grep -i "xrruntime"

# 应该看到初始化日志
# I/XRRuntime: XRRuntime JNI_OnLoad called
# I/XRRuntime: Initializing XR Runtime for Qualcomm XR2
# I/XRRuntime: XR Runtime initialized successfully
```

---

## 故障排除

### 问题 1: 库无法加载

**症状**: `dlopen failed: library not found`

**解决方案**:
1. 检查文件路径是否正确
2. 检查文件权限（应该是 644）
3. 检查 SELinux 上下文
4. 检查库依赖是否满足

### 问题 2: SELinux 拒绝访问

**症状**: `avc: denied` 在 logcat 中

**解决方案**:
1. 检查 SELinux 策略文件
2. 添加必要的权限规则
3. 重新编译 SELinux 策略

### 问题 3: OpenXR Runtime 未被发现

**症状**: `xrCreateInstance` 返回 `XR_ERROR_RUNTIME_UNAVAILABLE`

**解决方案**:
1. 检查 OpenXR Loader 配置文件路径
2. 检查 JSON 格式是否正确
3. 检查库路径是否可访问
4. 检查 OpenXR Loader 版本

---

## 相关文件

- `app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so` - 编译好的库文件
- `docs/USAGE_GUIDE.md` - 使用指南
- `AndroidManifest.xml` - Android 清单文件

