# XR Runtime 架构文档

## 概述

本文档描述了为高通 XR2 平台设计的自定义 OpenXR Runtime 的架构设计。

## 整体架构

```
┌─────────────────────────────────────────┐
│      OpenXR Application Layer           │
│    (使用 OpenXR Loader)                 │
└──────────────────┬──────────────────────┘
                   │
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
                   │
┌──────────────────▼──────────────────────┐
│      Qualcomm XR2 Platform Layer        │
│  - Snapdragon Spaces SDK                │
│  - QVR API                              │
│  - Android NDK APIs                     │
└─────────────────────────────────────────┘
```

## 核心组件

### 1. OpenXR API 层

实现所有 OpenXR 1.1 规范要求的 API 函数：

- **Instance 管理** (`openxr/instance.cpp`)
  - `xrCreateInstance`: 创建 XR 实例
  - `xrDestroyInstance`: 销毁实例
  - `xrGetInstanceProperties`: 获取实例属性
  - `xrGetSystem`: 获取系统信息

- **Session 管理** (`openxr/session.cpp`)
  - `xrCreateSession`: 创建会话
  - `xrDestroySession`: 销毁会话
  - `xrBeginSession`: 开始会话
  - `xrEndSession`: 结束会话

- **Frame 管理** (`openxr/frame.cpp`)
  - `xrWaitFrame`: 等待下一帧
  - `xrBeginFrame`: 开始帧渲染
  - `xrEndFrame`: 提交帧

- **Space 追踪** (`openxr/space.cpp`)
  - `xrCreateReferenceSpace`: 创建参考空间
  - `xrCreateActionSpace`: 创建动作空间
  - `xrLocateSpace`: 定位空间
  - `xrLocateViews`: 定位视图

- **Swapchain 管理** (`openxr/swapchain.cpp`)
  - `xrCreateSwapchain`: 创建交换链
  - `xrEnumerateSwapchainImages`: 枚举图像
  - `xrAcquireSwapchainImage`: 获取图像
  - `xrReleaseSwapchainImage`: 释放图像

- **Input 系统** (`openxr/input.cpp`)
  - `xrCreateActionSet`: 创建动作集
  - `xrCreateAction`: 创建动作
  - `xrSyncActions`: 同步动作
  - `xrGetActionState*`: 获取动作状态

- **Event 系统** (`openxr/event.cpp`)
  - `xrPollEvent`: 轮询事件

### 2. 平台抽象层

#### Android 平台 (`platform/android_platform.cpp`)
- JNI 接口管理
- EGL 上下文管理
- Native Window 管理
- 生命周期管理

#### 显示管理 (`platform/display_manager.cpp`)
- Swapchain 图像创建和销毁
- 支持的格式枚举
- 视图配置属性

#### 输入管理 (`platform/input_manager.cpp`)
- 交互配置文件绑定
- 动作状态查询
- 输入同步

#### 帧同步 (`platform/frame_sync.cpp`)
- 帧等待和同步
- 帧渲染开始/结束
- 层提交

### 3. 高通 XR2 平台层

#### XR2 平台集成 (`qualcomm/xr2_platform.cpp`)
- 平台初始化和关闭
- 显示管理
- 追踪系统集成
- 帧管理
- 输入系统集成

#### QVR API 封装 (`qualcomm/qvr_api_wrapper.cpp`)
- QVR API 调用封装（需要 NDA）

#### Snapdragon Spaces SDK 封装 (`qualcomm/spaces_sdk_wrapper.cpp`)
- Spaces SDK 调用封装

## 数据流

### 渲染流程

1. 应用调用 `xrWaitFrame` → 等待显示刷新
2. 应用调用 `xrBeginFrame` → 开始帧渲染
3. 应用渲染到 Swapchain 图像
4. 应用调用 `xrEndFrame` → 提交帧到合成器
5. Runtime 将帧提交到 XR2 显示系统

### 追踪流程

1. 应用调用 `xrLocateSpace` → 查询空间位置
2. Runtime 从 XR2 追踪系统获取姿态数据
3. 转换到请求的参考空间
4. 返回位置和方向

### 输入流程

1. 应用调用 `xrSyncActions` → 同步输入状态
2. Runtime 从 XR2 输入系统读取控制器数据
3. 应用调用 `xrGetActionState*` → 获取动作状态
4. Runtime 返回映射后的动作值

## 线程模型

- **主线程**: OpenXR API 调用（线程安全）
- **渲染线程**: 帧渲染和提交
- **追踪线程**: 追踪数据更新
- **输入线程**: 输入数据读取

## 内存管理

- 使用智能指针管理对象生命周期
- 对齐内存分配用于性能优化
- 及时释放资源避免泄漏

## 错误处理

- 所有 API 返回标准的 XrResult
- 详细的错误日志记录
- 错误码到字符串的转换

## 性能优化

- 帧率稳定在 90Hz
- 异步时间扭曲（如果支持）
- 高效的追踪数据更新
- 最小化内存分配

## 扩展性

架构设计支持：
- 添加新的 OpenXR 扩展
- 支持新的图形 API（Vulkan）
- 添加新的输入设备
- 支持新的追踪特性

