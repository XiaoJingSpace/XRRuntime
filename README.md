# OpenXR Runtime for Qualcomm XR2

一个完全符合 OpenXR 1.1 规范的自定义 XR Runtime，运行在 Android 12 系统上，硬件平台为高通 XR2，支持 VR 和 AR 应用。

## 项目概述

本项目实现了一个完整的 OpenXR Runtime，包括：

- ✅ OpenXR 1.1 核心 API 实现
- ✅ Android 平台集成
- ✅ 高通 XR2 平台集成
- ✅ **QVR API 集成**（Snapdragon XR SDK 4.0.5）
- ✅ 显示管理
- ✅ 追踪系统
- ✅ 输入系统
- ✅ 事件处理

## 项目结构

```
XRRuntimeStudy/
├── CMakeLists.txt                 # CMake 构建配置
├── build.gradle                   # Android Gradle 配置
├── AndroidManifest.xml            # Android 清单文件
├── SnapdragonXR-SDK-source.rel.4.0.5/  # 高通 XR SDK
├── src/
│   ├── main/
│   │   ├── cpp/                   # C++ 源代码
│   │   │   ├── openxr/            # OpenXR API 实现
│   │   │   ├── platform/          # 平台抽象层
│   │   │   ├── qualcomm/          # 高通 XR2 集成
│   │   │   │   ├── qvr_api_wrapper.cpp  # QVR API 封装 ✅
│   │   │   │   ├── qvr_api_wrapper.h    # QVR API 头文件 ✅
│   │   │   │   └── xr2_platform.cpp     # XR2 平台集成（已集成 QVR）✅
│   │   │   ├── utils/             # 工具类
│   │   │   └── jni/               # JNI 桥接
│   │   ├── java/                  # Java 代码
│   │   └── assets/                # 资源文件
├── scripts/                       # 构建和部署脚本
└── docs/                          # 文档
    └── QVR_INTEGRATION.md         # QVR API 集成指南 ✅
```

## QVR API 集成状态

✅ **已完成**：
- QVR Service Client 封装层
- VR Mode 管理（启动/停止）
- 头部追踪数据获取
- 显示中断配置
- 时间戳转换（QVR Time ↔ OpenXR Time）
- 姿态转换（QVR Pose → OpenXR Pose）
- XR2 Platform 集成

## 快速开始

### 前置要求

- Android SDK Platform 31+
- Android NDK r25+
- CMake 3.22.1+
- OpenXR SDK 1.1.x
- **Snapdragon XR SDK 4.0.5**（已包含在项目中）

### 编译

```bash
# Debug 版本
./gradlew assembleDebug

# Release 版本
./gradlew assembleRelease

# 或使用脚本
./scripts/build.sh debug
```

### 部署

```bash
# 使用部署脚本
./scripts/deploy.sh

# 或手动安装
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

### 调试

```bash
# 查看日志
./scripts/debug.sh logcat

# GDB 调试
./scripts/debug.sh gdb

# 性能分析
./scripts/debug.sh perf
```

## 文档

- **[完整项目说明](docs/PROJECT_GUIDE.md)** - 项目完整说明文档（推荐阅读）⭐
- [架构文档](docs/ARCHITECTURE.md) - 详细的架构设计说明
- [构建指南](docs/BUILD.md) - 编译步骤和配置
- [部署指南](docs/DEPLOY.md) - 部署和安装方法
- [调试指南](docs/DEBUG.md) - 调试技巧和故障排除
- [API 参考](docs/API_REFERENCE.md) - API 实现参考
- **[QVR 集成指南](docs/QVR_INTEGRATION.md)** - QVR API 集成详细说明 ✅

## 功能特性

### 已实现

- ✅ Instance 管理
- ✅ Session 管理
- ✅ Frame 循环
- ✅ Space 追踪（使用 QVR API）
- ✅ Swapchain 管理
- ✅ Input 系统
- ✅ Event 系统
- ✅ Android 平台集成
- ✅ **XR2 平台集成（QVR API）** ✅

### QVR API 功能

- ✅ QVR Service Client 管理
- ✅ VR Mode 启动/停止
- ✅ 头部追踪（6DOF）
- ✅ 显示中断配置（VSYNC）
- ✅ 时间戳转换
- ✅ 姿态数据转换

### 待完善

- ⏳ 眼动追踪集成
- ⏳ 相机支持（深度、RGB）
- ⏳ 预测姿态优化
- ⏳ 性能优化
- ⏳ 错误恢复机制

## QVR API 使用

QVR API 已集成到 XR2 Platform 层：

1. **初始化**: `InitializeXR2Platform()` 自动初始化 QVR API
2. **追踪**: `LocateXR2ReferenceSpace()` 使用 QVR 获取追踪数据
3. **显示**: `WaitForXR2NextFrame()` 使用 QVR VSYNC 中断
4. **渲染**: `StartXR2Rendering()` 启动 QVR VR Mode

详细使用说明请参考 [QVR_INTEGRATION.md](docs/QVR_INTEGRATION.md)。

## 开发状态

本项目提供了完整的架构和框架实现，QVR API 集成已完成：

1. **QVR API 集成** ✅
   - 已完成 QVR Service Client 封装
   - 已完成追踪数据获取和转换
   - 已完成显示管理集成

2. **平台特定实现**
   - 显示管理使用 QVR API
   - 追踪系统使用 QVR API
   - 帧同步使用 QVR VSYNC 中断

## 注意事项

1. **SDK 依赖**: 需要实际的 OpenXR SDK 和高通 XR2 SDK（已包含）
2. **硬件要求**: 需要高通 XR2 硬件设备
3. **权限要求**: 需要相机、振动等权限
4. **QVR 库**: 需要 `libqvrservice_client.qti.so` 库（设备上应已存在）

## 许可证

本项目仅供学习和研究使用。

## 贡献

欢迎提交 Issue 和 Pull Request。

## 联系方式

如有问题，请提交 Issue。
