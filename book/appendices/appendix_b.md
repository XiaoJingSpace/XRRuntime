# 附录B：QVR API 参考

## QVR API 函数列表

### Service Client 管理

- `QVRServiceClient_Init` - 初始化 Service Client
- `QVRServiceClient_Release` - 释放 Service Client
- `QVRServiceClient_GetClient` - 获取 Client 实例

### VR Mode 管理

- `QVRServiceClient_StartVRMode` - 启动 VR 模式
- `QVRServiceClient_StopVRMode` - 停止 VR 模式
- `QVRServiceClient_GetVRMode` - 获取 VR 模式状态

### 追踪

- `QVRServiceClient_GetPose` - 获取姿态
- `QVRServiceClient_GetPoseWithPrediction` - 获取预测姿态
- `QVRServiceClient_GetControllerState` - 获取控制器状态

### 显示

- `QVRServiceClient_GetDisplayConfig` - 获取显示配置
- `QVRServiceClient_SetDisplayInterrupt` - 设置显示中断

### 性能管理

- `QVRServiceClient_SetPerfLevel` - 设置性能级别
- `QVRServiceClient_GetPerfLevel` - 获取性能级别

## 数据结构说明

### QVRPose

```cpp
typedef struct {
    float posx, posy, posz;
    float rotx, roty, rotz, rotw;
} QVRPose;
```

### QVRControllerState

```cpp
typedef struct {
    uint32_t buttons;
    float trigger;
    float grip;
    QVRPose pose;
} QVRControllerState;
```

## 使用示例

### 初始化 QVR Service

```cpp
QVRServiceClient* client = nullptr;
if (QVRServiceClient_Init(&client) == QVR_SUCCESS) {
    // 使用 client
}
```

### 获取头部姿态

```cpp
QVRPose pose;
uint64_t timestamp;
if (QVRServiceClient_GetPoseWithPrediction(
    client, QVR_TRACKING_TYPE_HEAD, &timestamp, &pose) == QVR_SUCCESS) {
    // 使用姿态数据
}
```

## 更多信息

- [Snapdragon XR SDK 文档](https://developer.qualcomm.com/software/snapdragon-xr-sdk)

