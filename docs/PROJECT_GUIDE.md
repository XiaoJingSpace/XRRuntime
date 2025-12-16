# OpenXR Runtime for Qualcomm XR2 - 完整项目指南

## 目录

1. [项目概述](#项目概述)
2. [架构设计](#架构设计)
3. [技术栈](#技术栈)
4. [项目结构](#项目结构)
5. [核心功能](#核心功能)
6. [编译指南](#编译指南)
7. [部署指南](#部署指南)
8. [API 使用](#api-使用)
9. [开发指南](#开发指南)
10. [故障排除](#故障排除)

---

## 项目概述

### 项目简介

本项目是一个完全符合 **OpenXR 1.1** 规范的自定义 XR Runtime，专为 **高通 XR2** 平台设计，运行在 **Android 12** 系统上。Runtime 支持 VR 和 AR 应用，提供了完整的 OpenXR API 实现。

### 项目目标

- ✅ 实现完整的 OpenXR 1.1 API
- ✅ 集成高通 XR2 平台（QVR API）
- ✅ 支持 VR 和 AR 应用
- ✅ 提供高性能的追踪和渲染
- ✅ 支持双目显示和 6DOF 追踪

### 技术特点

- **标准兼容**: 完全符合 OpenXR 1.1 规范
- **平台集成**: 深度集成高通 XR2 和 QVR API
- **模块化设计**: 清晰的架构分层，易于扩展
- **性能优化**: 针对 XR2 硬件优化
- **完整文档**: 详细的开发和使用文档

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────┐
│      OpenXR Application Layer           │
│    (使用 OpenXR Loader)                 │
│    - Unity/Unreal Engine                │
│    - Native OpenXR Apps                 │
└──────────────────┬──────────────────────┘
                   │ OpenXR API Calls
┌──────────────────▼──────────────────────┐
│      Custom XR Runtime (SO库)          │
│  ┌──────────────────────────────────┐   │
│  │  OpenXR API Implementation      │   │
│  │  - Instance Management          │   │
│  │  - Session Management           │   │
│  │  - Frame Management             │   │
│  │  - Space Tracking               │   │
│  │  - Swapchain Management         │   │
│  │  - Input System                 │   │
│  │  - Event System                 │   │
│  └──────────────────────────────────┘   │
│  ┌──────────────────────────────────┐   │
│  │  Platform Abstraction Layer      │   │
│  │  - Android Platform              │   │
│  │  - Display Manager               │   │
│  │  - Input Manager                 │   │
│  │  - Frame Synchronization         │   │
│  └──────────────────────────────────┘   │
└──────────────────┬──────────────────────┘
                   │ Platform APIs
┌──────────────────▼──────────────────────┐
│      Qualcomm XR2 Platform Layer        │
│  - QVR API (Snapdragon XR SDK 4.0.5)   │
│  - Android NDK APIs                     │
│  - EGL/OpenGL ES                       │
└─────────────────────────────────────────┘
```

### 核心组件

#### 1. OpenXR API 层 (`src/main/cpp/openxr/`)

实现所有 OpenXR 1.1 核心 API：

- **Instance 管理** (`instance.cpp`)
  - `xrCreateInstance` - 创建 XR 实例
  - `xrDestroyInstance` - 销毁实例
  - `xrGetInstanceProperties` - 获取实例属性
  - `xrGetSystem` - 获取系统信息

- **Session 管理** (`session.cpp`)
  - `xrCreateSession` - 创建会话
  - `xrDestroySession` - 销毁会话
  - `xrBeginSession` - 开始会话
  - `xrEndSession` - 结束会话

- **Frame 管理** (`frame.cpp`)
  - `xrWaitFrame` - 等待下一帧
  - `xrBeginFrame` - 开始帧渲染
  - `xrEndFrame` - 提交帧

- **Space 追踪** (`space.cpp`)
  - `xrCreateReferenceSpace` - 创建参考空间
  - `xrLocateSpace` - 定位空间
  - `xrLocateViews` - 定位视图

- **Swapchain 管理** (`swapchain.cpp`)
  - `xrCreateSwapchain` - 创建交换链
  - `xrAcquireSwapchainImage` - 获取图像
  - `xrReleaseSwapchainImage` - 释放图像

- **Input 系统** (`input.cpp`)
  - `xrCreateActionSet` - 创建动作集
  - `xrSyncActions` - 同步动作
  - `xrGetActionState*` - 获取动作状态

- **Event 系统** (`event.cpp`)
  - `xrPollEvent` - 轮询事件

#### 2. 平台抽象层 (`src/main/cpp/platform/`)

- **Android Platform** (`android_platform.cpp`)
  - JNI 接口管理
  - EGL 上下文管理
  - Native Window 管理

- **Display Manager** (`display_manager.cpp`)
  - Swapchain 图像创建
  - 支持的格式枚举
  - 视图配置属性

- **Input Manager** (`input_manager.cpp`)
  - 交互配置文件绑定
  - 动作状态查询
  - 输入同步

- **Frame Sync** (`frame_sync.cpp`)
  - 帧等待和同步
  - 帧渲染开始/结束
  - 层提交

#### 3. 高通 XR2 平台层 (`src/main/cpp/qualcomm/`)

- **XR2 Platform** (`xr2_platform.cpp`)
  - 平台初始化和关闭
  - 显示管理
  - 追踪系统集成
  - 帧管理
  - 输入系统集成

- **QVR API Wrapper** (`qvr_api_wrapper.cpp`)
  - QVR Service Client 封装
  - VR Mode 管理
  - 追踪数据获取
  - 显示中断配置
  - 数据转换（QVR ↔ OpenXR）

---

## 技术栈

### 开发环境

- **操作系统**: Windows/Linux/macOS
- **Android SDK**: Platform 31+ (Android 12)
- **Android NDK**: r25+
- **CMake**: 3.22.1+
- **Gradle**: 8.0+
- **JDK**: 8+

### 核心库

- **OpenXR SDK**: 1.1.x
- **Snapdragon XR SDK**: 4.0.5
- **QVR API**: Snapdragon XR SDK 内置
- **Android NDK**: 原生开发工具
- **OpenGL ES**: 3.0+ (图形渲染)

### 编程语言

- **C++**: 17 标准（核心实现）
- **Java**: 8+（Android 集成）
- **CMake**: 构建系统
- **Gradle**: 项目管理

---

## 项目结构

```
XRRuntimeStudy/
├── CMakeLists.txt                 # 根 CMake 配置
├── build.gradle                   # 根 Gradle 配置
├── settings.gradle                # Gradle 设置
├── gradle.properties              # Gradle 属性
├── gradlew.bat                    # Windows Gradle wrapper
├── gradle/                        # Gradle wrapper 文件
│   └── wrapper/
│       └── gradle-wrapper.properties
├── AndroidManifest.xml            # Android 清单文件
├── proguard-rules.pro             # ProGuard 规则
├── local.properties.example       # 本地配置示例
│
├── SnapdragonXR-SDK-source.rel.4.0.5/  # 高通 XR SDK
│   └── 3rdparty/qvr/             # QVR API
│       └── inc/                  # QVR 头文件
│
├── app/                           # Android 应用模块
│   └── build.gradle              # 应用构建配置
│
├── src/
│   └── main/
│       ├── cpp/                  # C++ 源代码
│       │   ├── CMakeLists.txt    # CMake 构建配置
│       │   ├── main.cpp          # JNI 入口
│       │   │
│       │   ├── openxr/           # OpenXR API 实现
│       │   │   ├── openxr_api.h
│       │   │   ├── openxr_api.cpp
│       │   │   ├── instance.cpp
│       │   │   ├── session.cpp
│       │   │   ├── frame.cpp
│       │   │   ├── space.cpp
│       │   │   ├── swapchain.cpp
│       │   │   ├── input.cpp
│       │   │   └── event.cpp
│       │   │
│       │   ├── platform/         # 平台抽象层
│       │   │   ├── android_platform.h
│       │   │   ├── android_platform.cpp
│       │   │   ├── display_manager.h
│       │   │   ├── display_manager.cpp
│       │   │   ├── input_manager.h
│       │   │   ├── input_manager.cpp
│       │   │   ├── frame_sync.h
│       │   │   └── frame_sync.cpp
│       │   │
│       │   ├── qualcomm/         # 高通 XR2 集成
│       │   │   ├── xr2_platform.h
│       │   │   ├── xr2_platform.cpp
│       │   │   ├── qvr_api_wrapper.h
│       │   │   ├── qvr_api_wrapper.cpp
│       │   │   ├── spaces_sdk_wrapper.cpp
│       │   │   └── qvr_api_wrapper.cpp
│       │   │
│       │   ├── utils/            # 工具类
│       │   │   ├── logger.h
│       │   │   ├── logger.cpp
│       │   │   ├── error_handler.h
│       │   │   ├── error_handler.cpp
│       │   │   ├── memory_manager.h
│       │   │   └── memory_manager.cpp
│       │   │
│       │   └── jni/              # JNI 桥接
│       │       ├── jni_bridge.h
│       │       └── jni_bridge.cpp
│       │
│       ├── java/                 # Java 代码
│       │   └── com/xrruntime/
│       │       └── XRRuntimeService.java
│       │
│       ├── res/                  # Android 资源
│       │   └── values/
│       │       └── strings.xml
│       │
│       └── assets/               # 资源文件
│           └── openxr_loader.json
│
├── scripts/                      # 构建和部署脚本
│   ├── build.sh                  # 构建脚本
│   ├── deploy.sh                 # 部署脚本
│   └── debug.sh                  # 调试脚本
│
└── docs/                         # 文档
    ├── ARCHITECTURE.md           # 架构文档
    ├── BUILD.md                  # 构建指南
    ├── DEPLOY.md                 # 部署指南
    ├── DEBUG.md                  # 调试指南
    ├── API_REFERENCE.md          # API 参考
    ├── QVR_INTEGRATION.md        # QVR 集成指南
    └── PROJECT_GUIDE.md          # 本文件
```

---

## 核心功能

### 1. OpenXR API 实现

实现了 OpenXR 1.1 规范的所有核心 API：

- ✅ Instance 管理（创建、销毁、属性查询）
- ✅ Session 管理（创建、开始、结束）
- ✅ Frame 循环（等待、开始、提交）
- ✅ Space 追踪（参考空间、动作空间、定位）
- ✅ Swapchain 管理（创建、枚举、获取、释放）
- ✅ Input 系统（动作集、动作、状态查询）
- ✅ Event 系统（事件轮询）
- ✅ View 配置（枚举、属性查询）

### 2. QVR API 集成

深度集成高通 QVR API：

- ✅ QVR Service Client 管理
- ✅ VR Mode 启动/停止
- ✅ 头部追踪（6DOF）
- ✅ 显示中断配置（VSYNC）
- ✅ 时间戳转换
- ✅ 姿态数据转换

### 3. Android 平台集成

- ✅ JNI 接口实现
- ✅ EGL 上下文管理
- ✅ Native Window 管理
- ✅ 权限管理
- ✅ 生命周期管理

### 4. 显示管理

- ✅ Swapchain 图像创建（OpenGL ES）
- ✅ 支持的格式枚举
- ✅ 视图配置属性
- ✅ 双目渲染支持

### 5. 追踪系统

- ✅ 6DOF 头部追踪（使用 QVR API）
- ✅ 空间定位（VIEW、LOCAL、STAGE）
- ✅ 追踪状态查询
- ✅ 姿态质量评估

---

## 编译指南

### 前置要求

1. **Android SDK**
   - Android SDK Platform 31+
   - Android SDK Build Tools 33.0.0+
   - Android NDK r25+

2. **开发工具**
   - Android Studio Hedgehog (2023.1.1) 或更高版本
   - CMake 3.22.1+
   - Gradle 8.0+
   - JDK 8+

3. **第三方库**
   - OpenXR SDK 1.1.x（需要手动下载）
   - Snapdragon XR SDK 4.0.5（已包含在项目中）

### 环境配置

1. **设置环境变量**

```bash
export ANDROID_HOME=/path/to/android/sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/25.2.9519653
export PATH=$PATH:$ANDROID_HOME/platform-tools
```

2. **创建 local.properties**

创建 `local.properties` 文件：

```properties
sdk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk
ndk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk\\ndk\\25.2.9519653
```

3. **配置 OpenXR SDK**

将 OpenXR SDK 解压到 `libs/openxr/` 目录。

### 编译步骤

#### 方法 1: 使用 Android Studio

1. 打开 Android Studio
2. 选择 File → Open
3. 选择项目根目录
4. 等待 Gradle 同步完成
5. 选择 Build → Make Project

#### 方法 2: 使用命令行

```bash
# Debug 版本
./gradlew assembleDebug

# Release 版本
./gradlew assembleRelease
```

#### 方法 3: 使用构建脚本

```bash
# Windows
gradlew.bat assembleDebug

# Linux/macOS
chmod +x scripts/build.sh
./scripts/build.sh debug
```

### 输出位置

编译完成后，SO 库位置：

- **Debug**: `app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so`
- **Release**: `app/build/intermediates/cmake/release/obj/arm64-v8a/libxrruntime.so`

APK 位置：

- **Debug**: `app/build/outputs/apk/debug/app-debug.apk`
- **Release**: `app/build/outputs/apk/release/app-release.apk`

详细编译指南请参考 [BUILD.md](BUILD.md)。

---

## 部署指南

### 前置要求

- 已编译的 APK 或 SO 库
- Android 设备（高通 XR2 平台）
- USB 调试已启用
- ADB 工具已安装

### 部署方法

#### 方法 1: 通过 APK 安装（推荐）

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

#### 方法 2: 使用部署脚本

```bash
./scripts/deploy.sh
```

#### 方法 3: 手动部署 SO 库（需要 Root）

```bash
adb push libxrruntime.so /system/lib64/
adb shell chmod 644 /system/lib64/libxrruntime.so
```

### OpenXR Loader 配置

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

详细部署指南请参考 [DEPLOY.md](DEPLOY.md)。

---

## API 使用

### 基本使用流程

#### 1. 创建 Instance

```cpp
XrInstanceCreateInfo createInfo = {};
createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
strcpy(createInfo.applicationInfo.applicationName, "My App");
strcpy(createInfo.applicationInfo.engineName, "My Engine");

XrInstance instance;
XrResult result = xrCreateInstance(&createInfo, &instance);
```

#### 2. 获取 System

```cpp
XrSystemGetInfo systemInfo = {};
systemInfo.type = XR_TYPE_SYSTEM_GET_INFO;
systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

XrSystemId systemId;
result = xrGetSystem(instance, &systemInfo, &systemId);
```

#### 3. 创建 Session

```cpp
XrSessionCreateInfo sessionInfo = {};
sessionInfo.type = XR_TYPE_SESSION_CREATE_INFO;
sessionInfo.systemId = systemId;
// ... 配置图形绑定

XrSession session;
result = xrCreateSession(instance, &sessionInfo, &session);
```

#### 4. 开始 Session

```cpp
XrSessionBeginInfo beginInfo = {};
beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

result = xrBeginSession(session, &beginInfo);
```

#### 5. 渲染循环

```cpp
while (running) {
    // 等待下一帧
    XrFrameWaitInfo waitInfo = {};
    waitInfo.type = XR_TYPE_FRAME_WAIT_INFO;
    
    XrFrameState frameState = {};
    frameState.type = XR_TYPE_FRAME_STATE;
    
    xrWaitFrame(session, &waitInfo, &frameState);
    
    if (frameState.shouldRender) {
        // 开始帧
        XrFrameBeginInfo beginInfo = {};
        beginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;
        xrBeginFrame(session, &beginInfo);
        
        // 渲染...
        
        // 提交帧
        XrFrameEndInfo endInfo = {};
        endInfo.type = XR_TYPE_FRAME_END_INFO;
        endInfo.displayTime = frameState.predictedDisplayTime;
        // ... 配置层
        
        xrEndFrame(session, &endInfo);
    }
}
```

详细 API 参考请查看 [API_REFERENCE.md](API_REFERENCE.md)。

---

## 开发指南

### 添加新的 OpenXR API

1. 在 `openxr/openxr_api.h` 中声明函数
2. 在对应的实现文件中实现（如 `instance.cpp`）
3. 添加错误处理和日志
4. 更新 API_REFERENCE.md

### 扩展 QVR 功能

1. 在 `qvr_api_wrapper.h` 中声明包装函数
2. 在 `qvr_api_wrapper.cpp` 中实现
3. 在 `xr2_platform.cpp` 中集成
4. 更新 QVR_INTEGRATION.md

### 调试技巧

1. **查看日志**
   ```bash
   adb logcat | grep XRRuntime
   ```

2. **GDB 调试**
   ```bash
   ./scripts/debug.sh gdb
   ```

3. **性能分析**
   ```bash
   ./scripts/debug.sh perf
   ```

详细调试指南请参考 [DEBUG.md](DEBUG.md)。

---

## 故障排除

### 常见问题

#### 1. Gradle 同步失败

**问题**: "No connection to gradle server"

**解决方案**:
- 检查 `local.properties` 中的 SDK 路径
- 重启 Android Studio
- File → Invalidate Caches / Restart

#### 2. 编译错误：找不到 OpenXR 头文件

**问题**: `fatal error: openxr/openxr.h: No such file or directory` 或 CMake 错误提示找不到 OpenXR SDK

**解决方案**:
- 参考 [OpenXR SDK 设置指南](OPENXR_SDK_SETUP.md) 设置 OpenXR SDK Source
- 确保头文件已正确复制到 `include/openxr/` 或 `libs/openxr/include/openxr/`
- CMakeLists.txt 会自动检查并给出清晰的错误提示

#### 3. 编译错误：找不到 QVR 头文件

**问题**: `fatal error: QVRServiceClient.h: No such file or directory`

**解决方案**:
- 确认 Snapdragon XR SDK 路径正确
- 检查 `CMakeLists.txt` 中的包含路径
- QVR 头文件位于 `SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc`

#### 4. 运行时错误：找不到 QVR 库

**问题**: `cannot locate symbol` 或 `library not found`

**解决方案**:
- 确认设备上存在 `libqvrservice_client.qti.so`
- 检查 SO 库路径和权限

#### 5. 追踪数据无效

**问题**: 空间定位返回无效数据

**解决方案**:
- 检查相机权限是否授予
- 确认 VR Mode 已启动
- 查看追踪状态日志

### 获取帮助

- 查看详细文档：`docs/` 目录
- 检查日志：`adb logcat | grep XRRuntime`
- 提交 Issue：项目 Issue 页面

---

## 相关文档

- [架构文档](ARCHITECTURE.md) - 详细的架构设计说明
- [构建指南](BUILD.md) - 编译步骤和配置
- [部署指南](DEPLOY.md) - 部署和安装方法
- [调试指南](DEBUG.md) - 调试技巧和故障排除
- [API 参考](API_REFERENCE.md) - API 实现参考
- [QVR 集成指南](QVR_INTEGRATION.md) - QVR API 集成详细说明
- [OpenXR SDK 设置指南](OPENXR_SDK_SETUP.md) - OpenXR SDK Source 设置说明
- [已知限制](KNOWN_LIMITATIONS.md) - 已知限制和待完善功能

---

## 许可证

本项目仅供学习和研究使用。

## 贡献

欢迎提交 Issue 和 Pull Request。

---

**最后更新**: 2024年12月
