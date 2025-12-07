# QVR API 集成指南

本文档描述如何将 QVR (Qualcomm VR) API 集成到 OpenXR Runtime 中。

## QVR API 概述

QVR API 是高通 XR2 平台的原生 VR API，位于 Snapdragon XR SDK 中。主要组件包括：

### 核心头文件

- `QVRServiceClient.h` - QVR Service 客户端 API（主要接口）
- `QVRTypes.h` - QVR 类型定义和数据结构
- `QXR.h` - OpenXR 兼容的类型定义
- `QVRCameraClient.h` - 相机客户端 API

### 关键 API 功能

1. **QVRServiceClient** - 主要服务客户端
   - `QVRServiceClient_Create()` - 创建客户端
   - `QVRServiceClient_StartVRMode()` - 启动 VR 模式
   - `QVRServiceClient_StopVRMode()` - 停止 VR 模式
   - `QVRServiceClient_GetHeadTrackingData()` - 获取头部追踪数据
   - `QVRServiceClient_SetDisplayInterruptConfig()` - 配置显示中断

2. **追踪数据**：
   - `qvrservice_head_tracking_data_t` - 头部追踪数据结构（6DOF）
   - 包含旋转、位置、时间戳、预测系数等

3. **显示管理**：
   - VSYNC 中断处理
   - 显示参数配置

## 集成实现

### 1. 配置头文件路径

在 `src/main/cpp/CMakeLists.txt` 中添加 QVR 头文件路径：

```cmake
include_directories(
    ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc
)
```

### 2. QVR API 封装层

实现了 `qualcomm/qvr_api_wrapper.cpp` 和 `qualcomm/qvr_api_wrapper.h`，提供：

- QVR Service Client 生命周期管理
- VR Mode 管理
- 追踪模式配置
- 头部追踪数据获取
- 显示中断配置
- 时间转换（QVR Time ↔ OpenXR Time）
- 姿态转换（QVR Pose → OpenXR Pose）

### 3. XR2 Platform 集成

在 `qualcomm/xr2_platform.cpp` 中集成了 QVR API：

- **初始化**: `InitializeXR2Platform()` 调用 `InitializeQVRAPI()`
- **显示管理**: 使用 QVR API 配置 VSYNC 中断
- **渲染管理**: 使用 QVR API 启动/停止 VR Mode
- **追踪管理**: 使用 QVR API 获取头部追踪数据
- **帧同步**: 使用 QVR API 获取显示中断时间戳

### 4. 数据转换

#### 时间转换

QVR 使用 QTimer 时间域，OpenXR 使用系统时间。转换函数：

```cpp
XrTime QVRTimeToXrTime(uint64_t qvrTime);
uint64_t XrTimeToQVRTime(XrTime xrTime);
```

#### 姿态转换

QVR 使用 Android Portrait 坐标系，OpenXR 使用右手坐标系。转换函数：

```cpp
void QVRPoseToXrPose(const qvrservice_head_tracking_data_t* qvrData, XrPosef* xrPose);
```

## QVR API 调用流程

### 初始化流程

```
1. InitializeXR2Platform()
   └─> InitializeQVRAPI()
       └─> QVRServiceClient_Create()
       └─> QVRServiceClient_GetVRMode() (验证支持)
```

### VR Mode 启动流程

```
1. StartXR2Rendering()
   └─> QVRServiceClient_SetTrackingMode() (设置追踪模式)
   └─> QVRServiceClient_StartVRMode() (启动 VR Mode)
```

### 追踪数据获取流程

```
1. LocateXR2ReferenceSpace() / GetXR2ViewPoses()
   └─> QVRServiceClient_GetHeadTrackingData()
   └─> QVRPoseToXrPose() (转换姿态)
   └─> QVRTimeToXrTime() (转换时间戳)
```

### 帧同步流程

```
1. WaitForXR2NextFrame()
   └─> QVRServiceClient_GetDisplayInterruptTimestamp() (获取 VSYNC 时间戳)
   └─> QVRTimeToXrTime() (转换时间)
```

### 关闭流程

```
1. StopXR2Rendering()
   └─> QVRServiceClient_StopVRMode()
   
2. ShutdownXR2Platform()
   └─> ShutdownQVRAPI()
       └─> QVRServiceClient_Destroy()
```

## 追踪模式

QVR API 支持以下追踪模式：

- `TRACKING_MODE_NONE` - 无追踪
- `TRACKING_MODE_ROTATIONAL` - 3DOF 旋转追踪
- `TRACKING_MODE_POSITIONAL` - 6DOF 位置追踪（推荐）
- `TRACKING_MODE_ROTATIONAL_MAG` - 3DOF 旋转追踪（带磁力计）

当前实现优先使用 `TRACKING_MODE_POSITIONAL`，如果不支持则回退到 `TRACKING_MODE_ROTATIONAL`。

## 追踪状态

QVR 追踪数据包含以下状态信息：

- `tracking_state` - 追踪状态位掩码
  - bit 2: TRACKING - 正在追踪
  - bit 1: TRACKING_SUSPENDED - 追踪暂停
  - bit 0: RELOCATION_IN_PROGRESS - 重定位进行中
  - bit 3: FATAL_ERROR - 致命错误

- `pose_quality` - 姿态质量（0.0 = 差, 1.0 = 好）

这些状态被转换为 OpenXR 的 `XrSpaceLocationFlags`。

## 显示中断

QVR API 支持两种显示中断：

- `DISP_INTERRUPT_VSYNC` - VSYNC 信号（用于帧同步）
- `DISP_INTERRUPT_LINEPTR` - 行指针中断

当前实现使用 VSYNC 中断进行帧同步。

## 错误处理

QVR API 返回以下错误码：

- `QVR_SUCCESS` (0) - 成功
- `QVR_ERROR` (-1) - 一般错误
- `QVR_INVALID_PARAM` (-4) - 无效参数
- `QVR_API_NOT_SUPPORTED` (-3) - API 不支持
- `QVR_CALLBACK_NOT_SUPPORTED` (-2) - 回调不支持

所有错误都会被记录到日志中。

## 注意事项

1. **库依赖**: QVR API 需要 `libqvrservice_client.qti.so` 或 `libqvrservice_client.so` 库
2. **权限要求**: 需要相机权限用于追踪
3. **线程安全**: QVR API 调用需要适当的同步
4. **生命周期**: QVR Client 必须在 VR Mode 启动前创建，在停止后销毁
5. **时间域**: QVR 使用 QTimer 时间域，需要转换为 OpenXR 时间域

## 测试建议

1. **验证初始化**: 检查 QVR Client 是否成功创建
2. **验证 VR Mode**: 检查 VR Mode 是否可以启动
3. **验证追踪**: 检查追踪数据是否正常获取
4. **验证帧同步**: 检查 VSYNC 时间戳是否正常
5. **性能测试**: 检查追踪延迟和帧率

## 未来改进

1. **眼动追踪**: 集成 QVR 眼动追踪 API
2. **相机支持**: 集成 QVR 相机 API（深度、RGB）
3. **预测姿态**: 使用 QVR 预测姿态 API 减少延迟
4. **性能优化**: 优化数据转换和内存使用
5. **错误恢复**: 实现更好的错误恢复机制

