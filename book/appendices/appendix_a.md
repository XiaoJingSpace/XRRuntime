# 附录A：OpenXR API 快速参考

## 核心 API 列表

### Instance 管理

- `xrCreateInstance` - 创建 OpenXR Instance
- `xrDestroyInstance` - 销毁 Instance
- `xrGetInstanceProperties` - 获取 Instance 属性
- `xrGetSystem` - 获取 System
- `xrGetSystemProperties` - 获取 System 属性

### Session 管理

- `xrCreateSession` - 创建 Session
- `xrDestroySession` - 销毁 Session
- `xrBeginSession` - 开始 Session
- `xrEndSession` - 结束 Session
- `xrRequestExitSession` - 请求退出 Session

### Frame 管理

- `xrWaitFrame` - 等待下一帧
- `xrBeginFrame` - 开始帧渲染
- `xrEndFrame` - 提交帧

### Space 管理

- `xrCreateReferenceSpace` - 创建参考空间
- `xrCreateActionSpace` - 创建动作空间
- `xrDestroySpace` - 销毁空间
- `xrLocateSpace` - 定位空间
- `xrLocateViews` - 定位视图

### Swapchain 管理

- `xrCreateSwapchain` - 创建交换链
- `xrDestroySwapchain` - 销毁交换链
- `xrEnumerateSwapchainImages` - 枚举图像
- `xrAcquireSwapchainImage` - 获取图像
- `xrWaitSwapchainImage` - 等待图像
- `xrReleaseSwapchainImage` - 释放图像

### 输入系统

- `xrCreateActionSet` - 创建动作集
- `xrCreateAction` - 创建动作
- `xrSyncActions` - 同步动作
- `xrGetActionStateBoolean` - 获取布尔状态
- `xrGetActionStateFloat` - 获取浮点状态
- `xrGetActionStateVector2f` - 获取向量状态
- `xrGetActionStatePose` - 获取姿态状态

### 事件系统

- `xrPollEvent` - 轮询事件

## 参数说明

### XrInstanceCreateInfo

```cpp
typedef struct XrInstanceCreateInfo {
    XrStructureType             type;
    const void*                 next;
    XrApplicationInfo           applicationInfo;
    uint32_t                    enabledApiLayerCount;
    const char* const*          enabledApiLayerNames;
    uint32_t                    enabledExtensionCount;
    const char* const*          enabledExtensionNames;
} XrInstanceCreateInfo;
```

### XrSessionCreateInfo

```cpp
typedef struct XrSessionCreateInfo {
    XrStructureType             type;
    const void*                 next;
    XrSessionCreateFlags        createFlags;
    XrSystemId                  systemId;
} XrSessionCreateInfo;
```

## 返回值说明

### XrResult 错误码

- `XR_SUCCESS` - 成功
- `XR_ERROR_VALIDATION_FAILURE` - 验证失败
- `XR_ERROR_RUNTIME_FAILURE` - 运行时失败
- `XR_ERROR_OUT_OF_MEMORY` - 内存不足
- `XR_ERROR_INSTANCE_LOST` - Instance 丢失
- `XR_ERROR_SESSION_LOST` - Session 丢失

## 更多信息

- [OpenXR 规范](https://www.khronos.org/registry/OpenXR/specs/1.1/html/xrspec.html)

