# XR Runtime 使用指南

本文档说明如何使用编译好的 `libxrruntime.so` 文件。

## 目录

1. [应用中使用](#应用中使用)
2. [推送到系统分区](#推送到系统分区)
3. [OpenXR API 调用](#openxr-api-调用)
4. [示例代码](#示例代码)

---

## 应用中使用

### 方式 1: 作为应用库（推荐用于开发测试）

#### 步骤 1: 复制 .so 文件到项目

将 `libxrruntime.so` 复制到你的 Android 项目的 `jniLibs` 目录：

```
your-app/
└── app/
    └── src/
        └── main/
            └── jniLibs/
                └── arm64-v8a/
                    └── libxrruntime.so
```

#### 步骤 2: 在 Java/Kotlin 代码中加载库

```java
public class YourActivity extends Activity {
    static {
        // 加载原生库
        System.loadLibrary("xrruntime");
        // 注意：库名是 "xrruntime"，不需要 "lib" 前缀和 ".so" 后缀
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 库会在加载时自动初始化（通过 JNI_OnLoad）
    }
}
```

#### 步骤 3: 配置 build.gradle

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

---

## 推送到系统分区

### 前提条件

- **Root 权限**：需要 root 权限才能写入系统分区
- **系统签名**：如果要作为系统库，需要系统签名
- **只读分区**：系统分区通常是只读的，需要 remount

### 方式 1: 推送到 /system/lib64/（作为系统库）

```bash
# 1. 获取 root 权限
adb root

# 2. 重新挂载系统分区为可写
adb remount

# 3. 推送 .so 文件到系统库目录
adb push app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so /system/lib64/libxrruntime.so

# 4. 设置正确的权限
adb shell chmod 644 /system/lib64/libxrruntime.so
adb shell chown root:root /system/lib64/libxrruntime.so

# 5. 重新挂载为只读
adb shell mount -o remount,ro /system
```

### 方式 2: 推送到 /vendor/lib64/（作为厂商库）

```bash
# 1. 获取 root 权限
adb root

# 2. 重新挂载 vendor 分区为可写
adb remount

# 3. 推送 .so 文件
adb push app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so /vendor/lib64/libxrruntime.so

# 4. 设置权限
adb shell chmod 644 /vendor/lib64/libxrruntime.so
adb shell chown root:root /vendor/lib64/libxrruntime.so
```

### 方式 3: 集成到系统镜像（需要编译系统）

如果要集成到系统镜像中，需要：

1. **创建 Android.mk 或 Android.bp**

创建 `Android.mk`:
```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libxrruntime
LOCAL_SRC_FILES := libxrruntime.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)
```

2. **添加到系统构建**

在 `device.mk` 或 `product.mk` 中添加：
```makefile
PRODUCT_PACKAGES += libxrruntime
```

---

## OpenXR API 调用

### 库加载机制

`libxrruntime.so` 实现了 OpenXR Runtime。当库被加载时：

1. **自动初始化**：`JNI_OnLoad()` 会自动调用 `InitializeXRRuntime()`
2. **OpenXR Loader 发现**：系统会通过 OpenXR Loader 发现并加载这个 Runtime
3. **API 调用**：应用通过标准的 OpenXR API 调用 Runtime

### OpenXR Loader 配置

#### 方式 1: 使用 JSON 配置文件

创建 `assets/openxr_loader.json`:

```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "library_path": "libxrruntime.so",
        "name": "Qualcomm XR2 Runtime",
        "api_version": "1.1.0"
    }
}
```

#### 方式 2: 系统级配置（推送到系统后）

创建 `/vendor/etc/openxr/openxr_loader.json`:

```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "library_path": "/system/lib64/libxrruntime.so",
        "name": "Qualcomm XR2 Runtime",
        "api_version": "1.1.0"
    }
}
```

---

## 示例代码

### 完整的 Java 示例

```java
package com.example.xrapp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import org.khronos.openxr.*;

public class XRAppActivity extends Activity {
    private static final String TAG = "XRApp";
    
    static {
        // 加载 OpenXR Loader（如果使用）
        // System.loadLibrary("openxr_loader");
        
        // 加载我们的 Runtime
        System.loadLibrary("xrruntime");
    }
    
    private XrInstance instance;
    private XrSession session;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 初始化 OpenXR
        initializeOpenXR();
    }
    
    private void initializeOpenXR() {
        try {
            // 1. 创建 OpenXR Instance
            XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
            createInfo.setApplicationInfo(
                "My XR App",           // applicationName
                1,                     // applicationVersion
                "My Engine",           // engineName
                1,                     // engineVersion
                XrVersion.XR_CURRENT_API_VERSION // apiVersion
            );
            
            // 添加扩展
            createInfo.addEnabledExtension("XR_KHR_android_create_instance");
            createInfo.addEnabledExtension("XR_KHR_opengl_es_enable");
            
            // Android 特定信息
            XrInstanceCreateInfoAndroidKHR androidInfo = 
                new XrInstanceCreateInfoAndroidKHR();
            androidInfo.setApplicationVM(getApplicationInfo().nativeLibraryDir);
            androidInfo.setApplicationActivity(this);
            createInfo.setNext(androidInfo);
            
            // 创建实例
            instance = new XrInstance();
            int result = XR.xrCreateInstance(createInfo, instance);
            
            if (result != XrResult.XR_SUCCESS) {
                Log.e(TAG, "Failed to create XR instance: " + result);
                return;
            }
            
            Log.i(TAG, "OpenXR instance created successfully");
            
            // 2. 获取系统信息
            XrSystemGetInfo systemInfo = new XrSystemGetInfo();
            systemInfo.setFormFactor(XrFormFactor.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY);
            
            XrSystemId systemId = new XrSystemId();
            result = XR.xrGetSystem(instance, systemInfo, systemId);
            
            if (result == XR.xrResult.XR_SUCCESS) {
                Log.i(TAG, "XR System found: " + systemId);
                
                // 3. 创建 Session（需要 OpenGL ES 上下文）
                createSession();
            } else {
                Log.e(TAG, "Failed to get XR system: " + result);
            }
            
        } catch (Exception e) {
            Log.e(TAG, "Error initializing OpenXR", e);
        }
    }
    
    private void createSession() {
        // 创建 OpenGL ES 上下文和 Session
        // 这里需要设置 EGL 上下文和窗口
        // ...
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清理资源
        if (session != null) {
            XR.xrDestroySession(session);
        }
        if (instance != null) {
            XR.xrDestroyInstance(instance);
        }
    }
}
```

### Kotlin 示例

```kotlin
package com.example.xrapp

import android.app.Activity
import android.os.Bundle
import android.util.Log
import org.khronos.openxr.*

class XRAppActivity : Activity() {
    companion object {
        init {
            System.loadLibrary("xrruntime")
        }
        
        private const val TAG = "XRApp"
    }
    
    private var instance: XrInstance? = null
    private var session: XrSession? = null
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        initializeOpenXR()
    }
    
    private fun initializeOpenXR() {
        try {
            val createInfo = XrInstanceCreateInfo().apply {
                setApplicationInfo(
                    "My XR App",
                    1,
                    "My Engine",
                    1,
                    XrVersion.XR_CURRENT_API_VERSION
                )
                addEnabledExtension("XR_KHR_android_create_instance")
                addEnabledExtension("XR_KHR_opengl_es_enable")
            }
            
            instance = XrInstance()
            val result = XR.xrCreateInstance(createInfo, instance!!)
            
            if (result == XrResult.XR_SUCCESS) {
                Log.i(TAG, "OpenXR instance created")
                // 继续初始化...
            } else {
                Log.e(TAG, "Failed to create instance: $result")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error initializing OpenXR", e)
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        session?.let { XR.xrDestroySession(it) }
        instance?.let { XR.xrDestroyInstance(it) }
    }
}
```

---

## 验证安装

### 检查库是否加载成功

```bash
# 查看 logcat 输出
adb logcat | grep -i "xrruntime"

# 应该看到类似输出：
# I/XRRuntime: XRRuntime JNI_OnLoad called
# I/XRRuntime: Initializing XR Runtime for Qualcomm XR2
# I/XRRuntime: XR Runtime initialized successfully
```

### 检查系统库位置

```bash
# 检查系统库目录
adb shell ls -la /system/lib64/libxrruntime.so
adb shell ls -la /vendor/lib64/libxrruntime.so

# 检查库依赖
adb shell ldd /system/lib64/libxrruntime.so
```

### 测试 OpenXR Runtime 发现

使用 OpenXR 工具检查 Runtime 是否被发现：

```bash
# 使用 OpenXR SDK 的 hello_xr 示例
adb shell am start -n org.khronos.openxr.hello_xr/.MainActivity
```

---

## 常见问题

### Q: 库加载失败 "dlopen failed: library not found"

**A**: 检查以下几点：
1. `.so` 文件是否在正确的 `jniLibs/arm64-v8a/` 目录
2. `build.gradle` 中是否配置了 `abiFilters 'arm64-v8a'`
3. 设备架构是否匹配（必须是 arm64-v8a）

### Q: 推送到系统后应用无法加载

**A**: 
1. 检查权限：`chmod 644` 和 `chown root:root`
2. 检查 SELinux 上下文（可能需要设置）
3. 重启设备使系统库生效

### Q: OpenXR Runtime 未被发现

**A**:
1. 检查 OpenXR Loader 配置文件路径
2. 确认 Runtime 库路径正确
3. 检查 OpenXR Loader 版本兼容性

---

## 相关文档

- [OpenXR 官方文档](https://www.khronos.org/openxr/)
- [Android NDK 文档](https://developer.android.com/ndk)
- [系统库集成指南](docs/SYSTEM_INTEGRATION.md)

