# OpenXR Runtime for Qualcomm XR2

一个完全符合 OpenXR 1.1 规范的自定义 XR Runtime，运行在 Android 12 系统上，硬件平台为高通 XR2，支持 VR 和 AR 应用。

---

## 📋 目录

- [项目概述](#项目概述)
- [前置要求](#前置要求)
- [环境配置](#环境配置)
- [编译项目](#编译项目)
- [使用指南](#使用指南)
- [系统集成](#系统集成)
- [SDK 调用示例](#sdk-调用示例)
- [部署方法](#部署方法)
- [故障排除](#故障排除)
- [项目结构](#项目结构)
- [功能特性](#功能特性)
- [相关文档](#相关文档)

---

## 📖 项目概述

本项目实现了一个完整的 OpenXR Runtime，包括：

- ✅ OpenXR 1.1 核心 API 实现
- ✅ Android 平台集成
- ✅ 高通 XR2 平台集成
- ✅ **QVR API 集成**（Snapdragon XR SDK 4.0.5）
- ✅ 显示管理
- ✅ 追踪系统
- ✅ 输入系统
- ✅ 事件处理

### 编译状态

✅ **项目已成功编译** - 所有编译错误已修复，可以正常构建生成 `libxrruntime.so`。

---

## 🔧 前置要求

### 1. 开发环境

- **Android SDK Platform 31+** (Android 12)
- **Android NDK r25+** (推荐 r25c 或更高版本)
- **CMake 3.22.1+**
- **Gradle 7.6** (已配置在项目中)
- **Android Gradle Plugin 7.2.2** (已配置)
- **JDK 8+**

### 2. 第三方库

#### OpenXR SDK Source（必需）⭐

**重要**: 本项目需要 **OpenXR SDK Source**（源代码版本），而不是预编译版本。

- **下载地址**: https://github.com/KhronosGroup/OpenXR-SDK-Source
- **版本**: 1.1.x（推荐）
- **设置方法**: 参考 [OpenXR SDK 设置指南](docs/OPENXR_SDK_SETUP.md)

**快速设置**:
```bash
# 克隆 OpenXR SDK Source
git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
cd OpenXR-SDK-Source
git checkout release-1.1

# 复制头文件到项目
cd ..
mkdir -p include/openxr
cp -r OpenXR-SDK-Source/include/openxr/* include/openxr/
```

#### Snapdragon XR SDK（已包含）

- Snapdragon XR SDK 4.0.5（已包含在项目中）
- QVR API 头文件位于 `SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc`

### 3. 硬件要求

- 高通 XR2 硬件设备
- Android 12+ 系统

---

## ⚙️ 环境配置

### 1. 设置环境变量

**Linux/macOS:**
```bash
export ANDROID_HOME=/path/to/android/sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/23.1.7779620
export PATH=$PATH:$ANDROID_HOME/platform-tools
```

**Windows (PowerShell):**
```powershell
$env:ANDROID_HOME = "D:\AndroidSDK"
$env:ANDROID_NDK_HOME = "$env:ANDROID_HOME\ndk\23.1.7779620"
$env:PATH += ";$env:ANDROID_HOME\platform-tools"
```

### 2. 配置 local.properties

创建或编辑 `local.properties`:

```properties
sdk.dir=D:\\AndroidSDK
# 注意：ndk.dir 已弃用，NDK 版本在 build.gradle 中配置
```

### 3. 验证 OpenXR SDK 配置

检查头文件是否存在：

```bash
# Windows
Test-Path include\openxr\openxr.h

# Linux/macOS
test -f include/openxr/openxr.h
```

如果文件不存在，请参考 [OpenXR SDK 设置指南](docs/OPENXR_SDK_SETUP.md)。

---

## 🔨 编译项目

### 方法 1: 使用 Android Studio（推荐）⭐

1. **打开项目**
   - File → Open → 选择项目目录
   - 等待 Gradle 同步完成

2. **编译项目**
   - Build → Make Project
   - 或点击工具栏的 Build 按钮

3. **查看输出**
   - `.so` 文件位置: `app/build/intermediates/cxx/Debug/[hash]/obj/arm64-v8a/libxrruntime.so`
   - 自动复制到: `app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so` ✅

### 方法 2: 使用命令行

**Windows:**
```powershell
# Debug 版本
.\gradlew.bat assembleDebug

# Release 版本
.\gradlew.bat assembleRelease

# 或使用脚本
.\scripts\build.bat debug
```

**Linux/macOS:**
```bash
# Debug 版本
./gradlew assembleDebug

# Release 版本
./gradlew assembleRelease

# 或使用脚本
./scripts/build.sh debug
```

### 方法 3: 在 Cursor/VS Code 中编译

1. **快捷键编译**
   - Windows/Linux: `Ctrl+Shift+B`
   - macOS: `Cmd+Shift+B`

2. **任务菜单**
   - `Ctrl+Shift+P` → 输入 "Run Task"
   - 选择 `Gradle: Build Debug`

详细说明参考 [Cursor 编译指南](docs/CURSOR_COMPILE_GUIDE.md)。

### 编译输出

编译成功后，`.so` 文件会自动复制到：

```
app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
```

这是通过 `app/build.gradle` 中配置的自动复制任务完成的。

---

## 📱 使用指南

### 方式 1: 应用中使用（推荐用于开发测试）

#### 步骤 1: 复制 .so 文件

将编译好的 `.so` 文件复制到你的 Android 项目：

```
your-app/
└── app/
    └── src/
        └── main/
            └── jniLibs/
                └── arm64-v8a/
                    └── libxrruntime.so
```

#### 步骤 2: 配置 build.gradle

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

#### 步骤 3: 加载库

在 Java/Kotlin 代码中加载库：

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

**Kotlin 版本:**
```kotlin
class YourActivity : Activity() {
    companion object {
        init {
            System.loadLibrary("xrruntime")
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }
}
```

---

## 🚀 系统集成

### 方式 1: 使用部署脚本（推荐）

#### Windows PowerShell

```powershell
# 推送到系统分区
.\scripts\deploy_to_system.ps1

# 推送到厂商分区
.\scripts\deploy_to_system.ps1 -Vendor

# 查看帮助
.\scripts\deploy_to_system.ps1 -Help
```

#### Linux/macOS Bash

```bash
# 添加执行权限
chmod +x scripts/deploy_to_system.sh

# 推送到系统分区
./scripts/deploy_to_system.sh

# 推送到厂商分区
./scripts/deploy_to_system.sh --vendor
```

### 方式 2: 手动推送（需要 Root）

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

# 5. 设置 SELinux 上下文（如果需要）
adb shell chcon u:object_r:system_lib_file:s0 /system/lib64/libxrruntime.so

# 6. 重新挂载为只读
adb shell mount -o remount,ro /system

# 7. 重启设备
adb reboot
```

### 方式 3: 集成到系统镜像（AOSP 构建）

参考 [系统集成指南](docs/SYSTEM_INTEGRATION.md) 了解详细的系统镜像集成方法。

---

## 💻 SDK 调用示例

### 简化示例：仅加载库

```java
package com.example.xrapp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class SimpleXRActivity extends Activity {
    private static final String TAG = "XRApp";
    
    static {
        try {
            // 加载 XR Runtime 库
            System.loadLibrary("xrruntime");
            Log.i(TAG, "XR Runtime library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load XR Runtime library", e);
        }
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 库已经初始化，可以通过 OpenXR API 使用
        Log.i(TAG, "Activity created, XR Runtime is ready");
    }
}
```

### 完整示例：OpenXR 应用

```java
package com.example.xrapp;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import org.khronos.openxr.*;

public class XRMainActivity extends Activity {
    private static final String TAG = "XRApp";
    
    static {
        System.loadLibrary("xrruntime");
    }
    
    private XrInstance instance;
    private XrSession session;
    private XrSystemId systemId;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        if (initializeOpenXR()) {
            Log.i(TAG, "OpenXR initialized successfully");
        } else {
            Log.e(TAG, "Failed to initialize OpenXR");
            finish();
        }
    }
    
    private boolean initializeOpenXR() {
        try {
            // 1. 创建 OpenXR Instance
            XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
            createInfo.setApplicationInfo(
                "My XR App",           // applicationName
                1,                     // applicationVersion
                "My Engine",           // engineName
                1,                     // engineVersion
                XrVersion.XR_CURRENT_API_VERSION
            );
            
            // 添加必需的扩展
            createInfo.addEnabledExtension("XR_KHR_android_create_instance");
            createInfo.addEnabledExtension("XR_KHR_opengl_es_enable");
            
            // Android 特定配置
            XrInstanceCreateInfoAndroidKHR androidInfo = 
                new XrInstanceCreateInfoAndroidKHR();
            androidInfo.setApplicationVM(getApplicationInfo().nativeLibraryDir);
            androidInfo.setApplicationActivity(this);
            createInfo.setNext(androidInfo);
            
            instance = new XrInstance();
            int result = XR.xrCreateInstance(createInfo, instance);
            
            if (result != XrResult.XR_SUCCESS) {
                Log.e(TAG, "xrCreateInstance failed: " + result);
                return false;
            }
            
            Log.i(TAG, "OpenXR instance created");
            
            // 2. 获取 System
            XrSystemGetInfo systemInfo = new XrSystemGetInfo();
            systemInfo.setFormFactor(XrFormFactor.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY);
            
            systemId = new XrSystemId();
            result = XR.xrGetSystem(instance, systemInfo, systemId);
            
            if (result == XrResult.XR_SUCCESS) {
                Log.i(TAG, "XR System found: " + systemId);
                return true;
            } else {
                Log.e(TAG, "xrGetSystem failed: " + result);
                return false;
            }
            
        } catch (Exception e) {
            Log.e(TAG, "Error initializing OpenXR", e);
            return false;
        }
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

更多示例代码请参考 [SDK 集成示例](docs/SDK_INTEGRATION_EXAMPLE.md)。

---

## 📦 部署方法

### 应用级部署

1. **复制 .so 文件到项目**
   ```
   your-app/app/src/main/jniLibs/arm64-v8a/libxrruntime.so
   ```

2. **在代码中加载**
   ```java
   System.loadLibrary("xrruntime");
   ```

3. **打包 APK**
   - 构建 APK 时，`.so` 文件会自动包含在 APK 中

### 系统级部署

使用提供的部署脚本：

```powershell
# Windows
.\scripts\deploy_to_system.ps1

# Linux/macOS
./scripts/deploy_to_system.sh
```

脚本会自动：
- ✅ 检查设备连接
- ✅ 获取 root 权限
- ✅ 推送文件到系统分区
- ✅ 设置正确的权限和 SELinux 上下文
- ✅ 验证部署

---

## 🔍 故障排除

### 编译问题

#### 问题 1: OpenXR SDK headers not found

**症状**: CMake 错误提示找不到 OpenXR 头文件

**解决方案**:
1. 确认已下载 OpenXR SDK Source
2. 检查头文件是否在 `include/openxr/` 目录
3. 参考 [OpenXR SDK 设置指南](docs/OPENXR_SDK_SETUP.md)

#### 问题 2: Gradle 版本不兼容

**症状**: `IncrementalTaskInputs` 错误

**解决方案**: 
- 项目已配置 Gradle 7.6，如果仍有问题，检查 `gradle/wrapper/gradle-wrapper.properties`

#### 问题 3: AGP 版本不兼容

**症状**: Android Studio 提示 AGP 版本不支持

**解决方案**:
- 项目已配置 AGP 7.2.2，这是 Android Studio 支持的最新版本

详细编译问题解决方案参考 [编译问题解决方案](docs/COMPILE_WORKAROUND.md)。

### 运行时问题

#### 问题 1: 库加载失败 "dlopen failed: library not found"

**解决方案**:
1. 检查 `.so` 文件是否在正确的 `jniLibs/arm64-v8a/` 目录
2. 检查 `build.gradle` 中是否配置了 `abiFilters 'arm64-v8a'`
3. 确认设备架构匹配（必须是 arm64-v8a）

#### 问题 2: 推送到系统后应用无法加载

**解决方案**:
1. 检查文件权限：`chmod 644` 和 `chown root:root`
2. 检查 SELinux 上下文
3. 重启设备使系统库生效

#### 问题 3: OpenXR Runtime 未被发现

**解决方案**:
1. 检查 OpenXR Loader 配置文件路径
2. 确认 Runtime 库路径正确
3. 检查 OpenXR Loader 版本兼容性

### 验证部署

```bash
# 查看日志
adb logcat | grep -i "xrruntime"

# 应该看到：
# I/XRRuntime: XRRuntime JNI_OnLoad called
# I/XRRuntime: Initializing XR Runtime for Qualcomm XR2
# I/XRRuntime: XR Runtime initialized successfully
```

---

## 📁 项目结构

```
XRRuntimeStudy/
├── CMakeLists.txt                 # CMake 构建配置
├── build.gradle                   # Android Gradle 配置
├── AndroidManifest.xml            # Android 清单文件
├── local.properties                # 本地配置（SDK 路径）
├── SnapdragonXR-SDK-source.rel.4.0.5/  # 高通 XR SDK
├── include/                       # OpenXR SDK 头文件
│   └── openxr/
│       ├── openxr.h
│       ├── openxr_platform.h
│       └── ...
├── src/
│   ├── main/
│   │   ├── cpp/                   # C++ 源代码
│   │   │   ├── openxr/            # OpenXR API 实现
│   │   │   │   ├── openxr_api.cpp
│   │   │   │   ├── instance.cpp
│   │   │   │   ├── session.cpp
│   │   │   │   ├── space.cpp
│   │   │   │   ├── swapchain.cpp
│   │   │   │   ├── input.cpp
│   │   │   │   ├── event.cpp
│   │   │   │   └── frame.cpp
│   │   │   ├── platform/          # 平台抽象层
│   │   │   │   ├── android_platform.cpp
│   │   │   │   ├── display_manager.cpp
│   │   │   │   ├── input_manager.cpp
│   │   │   │   └── frame_sync.cpp
│   │   │   ├── qualcomm/          # 高通 XR2 集成
│   │   │   │   ├── xr2_platform.cpp
│   │   │   │   ├── qvr_api_wrapper.cpp
│   │   │   │   └── spaces_sdk_wrapper.cpp
│   │   │   ├── utils/             # 工具类
│   │   │   │   ├── logger.cpp
│   │   │   │   ├── error_handler.cpp
│   │   │   │   └── memory_manager.cpp
│   │   │   └── jni/               # JNI 桥接
│   │   │       ├── jni_bridge.cpp
│   │   │       └── jni_bridge.h
│   │   ├── java/                  # Java 代码
│   │   │   └── com/xrruntime/
│   │   │       └── XRRuntimeService.java
│   │   └── assets/                # 资源文件
│   │       └── openxr_loader.json
├── app/
│   ├── build.gradle               # App 构建配置
│   └── build/
│       └── outputs/
│           ├── aar/               # AAR 输出
│           │   └── app-debug.aar
│           └── jniLibs/           # 自动分离的 .so 文件 ✅
│               └── arm64-v8a/
│                   └── libxrruntime.so
├── scripts/                       # 构建和部署脚本
│   ├── build.bat / build.sh       # 构建脚本
│   ├── deploy.bat / deploy.sh    # 部署脚本
│   └── deploy_to_system.ps1 / deploy_to_system.sh  # 系统部署脚本
└── docs/                          # 文档
    ├── USAGE_GUIDE.md             # 使用指南
    ├── SYSTEM_INTEGRATION.md       # 系统集成指南
    ├── SDK_INTEGRATION_EXAMPLE.md  # SDK 集成示例
    ├── OPENXR_SDK_SETUP.md        # OpenXR SDK 设置
    ├── COMPILE_WORKAROUND.md       # 编译问题解决方案
    └── ...
```

---

## ✨ 功能特性

### 已实现功能

- ✅ **Instance 管理** - OpenXR Instance 创建和销毁
- ✅ **Session 管理** - Session 生命周期管理
- ✅ **Frame 循环** - 完整的帧渲染循环
- ✅ **Space 追踪** - 使用 QVR API 的 6DOF 头部追踪（含姿态预测）
- ✅ **Swapchain 管理** - 交换链创建和管理
- ✅ **Input 系统** - 完整的输入系统（含 Action 映射和路径解析）
- ✅ **Event 系统** - OpenXR 事件处理
- ✅ **Android 平台集成** - JNI 桥接和平台抽象
- ✅ **XR2 平台集成** - QVR API 集成
- ✅ **时间扭曲和眼偏移计算** - 基于四元数的计算
- ✅ **性能级别管理** - CPU/GPU 动态调整

### QVR API 功能

- ✅ QVR Service Client 管理
- ✅ VR Mode 启动/停止
- ✅ 头部追踪（6DOF，含姿态预测）
- ✅ 显示中断配置（VSYNC）
- ✅ 时间戳转换（含 tracker-android offset）
- ✅ 姿态数据转换
- ✅ 性能级别管理（CPU/GPU）
- ✅ 控制器状态获取
- ✅ 眼动追踪 API 集成框架

### 编译状态

✅ **编译成功** - 所有编译错误已修复：
- ✅ CMake 配置问题已解决
- ✅ Gradle 版本兼容性问题已解决
- ✅ AGP 版本兼容性问题已解决
- ✅ NDK 配置警告已解决
- ✅ OpenXR API 版本不匹配问题已解决
- ✅ 代码错误已修复（变量名、函数名、结构体等）
- ✅ 链接错误已解决（重复符号、未定义符号）

### 待完善功能

- ⏳ 相机支持（深度、RGB）
- ⏳ 高级性能优化和调优
- ⏳ 完整的错误恢复机制测试

---

## 📚 相关文档

### 核心文档

- **[使用指南](docs/USAGE_GUIDE.md)** - 详细的使用说明和示例代码 ⭐
- **[系统集成指南](docs/SYSTEM_INTEGRATION.md)** - 系统分区集成方法 ⭐
- **[SDK 集成示例](docs/SDK_INTEGRATION_EXAMPLE.md)** - 完整的 SDK 调用示例 ⭐
- **[OpenXR SDK 设置指南](docs/OPENXR_SDK_SETUP.md)** - OpenXR SDK Source 设置说明 ⭐
- **[编译问题解决方案](docs/COMPILE_WORKAROUND.md)** - 编译相关问题解决 ⭐

### 技术文档

- [架构文档](docs/ARCHITECTURE.md) - 详细的架构设计说明
- [构建指南](docs/BUILD.md) - 编译步骤和配置
- [部署指南](docs/DEPLOY.md) - 部署和安装方法
- [调试指南](docs/DEBUG.md) - 调试技巧和故障排除
- [API 参考](docs/API_REFERENCE.md) - API 实现参考
- [QVR 集成指南](docs/QVR_INTEGRATION.md) - QVR API 集成详细说明
- [Cursor 编译指南](docs/CURSOR_COMPILE_GUIDE.md) - 在 Cursor/VS Code 中编译项目
- [已知限制](docs/KNOWN_LIMITATIONS.md) - 已知限制和待完善功能

---

## 🎯 快速开始流程

### 第一次使用

1. **环境准备**
   ```bash
   # 安装 Android Studio
   # 配置 Android SDK 和 NDK
   ```

2. **设置 OpenXR SDK**
   ```bash
   git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
   mkdir -p include/openxr
   cp -r OpenXR-SDK-Source/include/openxr/* include/openxr/
   ```

3. **编译项目**
   ```bash
   ./gradlew assembleDebug
   ```

4. **使用库**
   - 复制 `app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so` 到你的项目
   - 在代码中调用 `System.loadLibrary("xrruntime")`

### 日常开发

1. **修改代码**
2. **编译**: `./gradlew assembleDebug`
3. **自动分离**: `.so` 文件自动复制到 `app/build/outputs/jniLibs/arm64-v8a/`
4. **部署**: 使用部署脚本或手动复制

---

## ⚠️ 注意事项

1. **SDK 依赖**: 
   - 需要 **OpenXR SDK Source**（不是预编译版本）
   - 高通 XR2 SDK 已包含在项目中

2. **硬件要求**: 
   - 需要高通 XR2 硬件设备
   - Android 12+ 系统

3. **权限要求**: 
   - 需要相机、振动等权限（已在 AndroidManifest.xml 中配置）

4. **QVR 库**: 
   - 需要 `libqvrservice_client.qti.so` 库（设备上应已存在）

5. **系统集成**: 
   - 推送到系统分区需要 root 权限
   - 需要正确的 SELinux 配置

---

## 📝 版本信息

- **OpenXR 版本**: 1.1.54
- **Android SDK**: 33 (Android 12)
- **NDK 版本**: 23.1.7779620
- **Gradle 版本**: 7.6
- **AGP 版本**: 7.2.2
- **CMake 版本**: 3.22.1

---

## 🔄 更新日志

### 最新更新（编译完成）

- ✅ **编译成功** - 所有编译错误已修复
- ✅ **自动分离** - 编译后自动将 .so 文件复制到 outputs/jniLibs/
- ✅ **部署脚本** - 提供 Windows 和 Linux 部署脚本
- ✅ **完整文档** - 整理所有文档到 README.md

### 之前更新

- ✅ 修复所有编译错误（CMake、Gradle、AGP、NDK 配置）
- ✅ 实现完整的 Action 到输入路径映射系统
- ✅ 实现时间扭曲矩阵计算（基于四元数）
- ✅ 修正眼偏移旋转计算
- ✅ 实现 QVR 时间偏移自动获取
- ✅ 启用并优化性能级别管理
- ✅ 实现控制器状态同步

---

## 📄 许可证

本项目仅供学习和研究使用。

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request。

---

## 📧 联系方式

- 网站：https://XiaoJingSpace.com
- 邮箱：taoxiulun@gmail.com
- 如有问题，请提交 Issue。

---

## 📖 更多信息

- [OpenXR 官方文档](https://www.khronos.org/openxr/)
- [Android NDK 文档](https://developer.android.com/ndk)
- [Snapdragon XR SDK 文档](https://developer.qualcomm.com/software/snapdragon-xr-sdk)

---

**最后更新**: 2025年1月

**编译状态**: ✅ 成功编译，所有功能可用
