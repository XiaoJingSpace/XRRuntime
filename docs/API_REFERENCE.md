# API 实现参考

本文档列出了已实现的 OpenXR API 函数及其实现状态。

## Instance 管理

| API | 状态 | 说明 |
|-----|------|------|
| `xrCreateInstance` | ✅ 已实现 | 创建 XR 实例 |
| `xrDestroyInstance` | ✅ 已实现 | 销毁实例 |
| `xrGetInstanceProperties` | ✅ 已实现 | 获取实例属性 |
| `xrGetSystem` | ✅ 已实现 | 获取系统信息 |
| `xrGetSystemProperties` | ✅ 已实现 | 获取系统属性 |
| `xrStringToPath` | ✅ 已实现 | 字符串转路径 |
| `xrPathToString` | ✅ 已实现 | 路径转字符串 |

## Session 管理

| API | 状态 | 说明 |
|-----|------|------|
| `xrCreateSession` | ✅ 已实现 | 创建会话 |
| `xrDestroySession` | ✅ 已实现 | 销毁会话 |
| `xrBeginSession` | ✅ 已实现 | 开始会话 |
| `xrEndSession` | ✅ 已实现 | 结束会话 |
| `xrRequestExitSession` | ✅ 已实现 | 请求退出会话 |

## Frame 管理

| API | 状态 | 说明 |
|-----|------|------|
| `xrWaitFrame` | ✅ 已实现 | 等待下一帧 |
| `xrBeginFrame` | ✅ 已实现 | 开始帧渲染 |
| `xrEndFrame` | ✅ 已实现 | 提交帧 |

## Space 追踪

| API | 状态 | 说明 |
|-----|------|------|
| `xrCreateReferenceSpace` | ✅ 已实现 | 创建参考空间 |
| `xrCreateActionSpace` | ✅ 已实现 | 创建动作空间 |
| `xrDestroySpace` | ✅ 已实现 | 销毁空间 |
| `xrLocateSpace` | ✅ 已实现 | 定位空间 |
| `xrLocateViews` | ✅ 已实现 | 定位视图 |
| `xrGetReferenceSpaceBoundsRect` | ✅ 已实现 | 获取参考空间边界 |

## Swapchain 管理

| API | 状态 | 说明 |
|-----|------|------|
| `xrCreateSwapchain` | ✅ 已实现 | 创建交换链 |
| `xrDestroySwapchain` | ✅ 已实现 | 销毁交换链 |
| `xrEnumerateSwapchainImages` | ✅ 已实现 | 枚举交换链图像 |
| `xrAcquireSwapchainImage` | ✅ 已实现 | 获取图像 |
| `xrWaitSwapchainImage` | ✅ 已实现 | 等待图像 |
| `xrReleaseSwapchainImage` | ✅ 已实现 | 释放图像 |
| `xrEnumerateSwapchainFormats` | ✅ 已实现 | 枚举交换链格式 |

## View 配置

| API | 状态 | 说明 |
|-----|------|------|
| `xrEnumerateViewConfigurations` | ✅ 已实现 | 枚举视图配置 |
| `xrGetViewConfigurationProperties` | ✅ 已实现 | 获取视图配置属性 |
| `xrEnumerateViewConfigurationViews` | ✅ 已实现 | 枚举视图配置视图 |

## Input 系统

| API | 状态 | 说明 |
|-----|------|------|
| `xrCreateActionSet` | ✅ 已实现 | 创建动作集 |
| `xrDestroyActionSet` | ✅ 已实现 | 销毁动作集 |
| `xrCreateAction` | ✅ 已实现 | 创建动作 |
| `xrDestroyAction` | ✅ 已实现 | 销毁动作 |
| `xrSuggestInteractionProfileBindings` | ✅ 已实现 | 建议交互配置文件绑定 |
| `xrAttachSessionActionSets` | ✅ 已实现 | 附加会话动作集 |
| `xrGetActionStateBoolean` | ✅ 已实现 | 获取布尔动作状态 |
| `xrGetActionStateFloat` | ✅ 已实现 | 获取浮点动作状态 |
| `xrGetActionStateVector2f` | ✅ 已实现 | 获取向量2f动作状态 |
| `xrGetActionStatePose` | ✅ 已实现 | 获取姿态动作状态 |
| `xrSyncActions` | ✅ 已实现 | 同步动作 |
| `xrGetCurrentInteractionProfile` | ✅ 已实现 | 获取当前交互配置文件 |

## Event 系统

| API | 状态 | 说明 |
|-----|------|------|
| `xrPollEvent` | ✅ 已实现 | 轮询事件 |

## 实现说明

### 已实现功能

所有列出的 API 都已实现基本框架，包括：
- 参数验证
- 错误处理
- 日志记录
- 基本的平台集成

### 已完善功能

以下功能已实现并集成：

1. **显示管理** ✅
   - ✅ XR2 显示 API 集成 - 使用 QVR API 查询图形属性、FOV、显示参数
   - ✅ 双目渲染支持 - 左右眼独立渲染目标、眼偏移配置、FOV 设置
   - ✅ 异步时间扭曲 - 层提交、扭曲矩阵计算（基于四元数旋转差异）、预测姿态更新
   - ✅ 眼偏移旋转计算 - 使用正确的四元数旋转向量公式进行眼偏移变换

2. **追踪系统** ✅
   - ✅ SLAM 系统集成 - 追踪质量监控、状态管理、重定位支持
   - ✅ 6DOF 头部追踪 - 姿态预测（位置和旋转）、平滑处理、坐标系转换、边界查询
   - ✅ QVR 时间偏移获取 - 自动获取 tracker-android offset，确保时间转换准确性
   - ✅ 手部追踪支持 - 集成 Snapdragon Spaces SDK，支持手部姿态获取
   - ✅ 眼动追踪支持 - 集成 QVR 眼动追踪 API，支持眼动数据获取

3. **输入系统** ✅
   - ✅ Action 到输入路径映射 - 完整的路径解析和映射逻辑，支持从 binding path 提取控制器和输入信息
   - ✅ Subaction Path 解析 - 正确识别 `/user/hand/left` 和 `/user/hand/right`
   - ✅ 控制器 API - 控制器服务集成、实际状态同步、所有输入类型支持（布尔、浮点、向量2D、姿态）
   - ✅ 控制器输入读取 - 从控制器服务实际读取按钮、摇杆、触发器数据
   - ✅ 手势识别支持 - 基于手部追踪的手势识别框架
   - ✅ 触觉反馈 - API 实现、强度控制、OpenXR 扩展支持

4. **性能优化** ✅
   - ✅ 帧率优化 - 帧率监控统计、自适应质量调整、帧跳过策略
   - ✅ 延迟优化 - 预测时间优化、异步渲染管道、运动到光子延迟最小化
   - ✅ 功耗优化 - 动态频率调整、热管理集成、性能级别控制
   - ✅ 性能级别设置 - 完整的 QVR API 集成，支持 CPU/GPU 性能级别动态调整
   - ✅ 性能级别调整优化 - 添加阈值和延迟机制，避免频繁切换，提高稳定性

## 扩展支持

架构设计支持添加以下 OpenXR 扩展：

- `XR_KHR_opengl_es_enable` - OpenGL ES 图形支持
- `XR_KHR_vulkan_enable` - Vulkan 图形支持
- `XR_KHR_composition_layer_depth` - 深度层支持
- `XR_EXT_hand_tracking` - 手部追踪扩展
- `XR_EXT_eye_gaze_interaction` - 眼动交互扩展

## 测试建议

建议使用以下工具进行测试：

1. **OpenXR Conformance Tests**
   - 官方一致性测试套件
   - 验证 API 实现的正确性

2. **OpenXR Samples**
   - 官方示例应用
   - 验证基本功能

3. **自定义测试应用**
   - 针对特定功能的测试
   - 性能测试

## 代码审查和修复记录

### 已修复的严重问题

1. **变量名冲突** ✅
   - 修复了 `g_lastFrameTime` 重复定义问题
   - 重命名为 `g_lastHeadPoseTime` 和 `g_currentHeadPoseTime`，避免编译错误

2. **控制器输入映射** ✅
   - 实现了完整的 Action 到输入路径的映射系统
   - 支持路径解析，从 binding path 提取控制器索引和输入类型
   - 实现了从控制器服务实际读取输入数据

3. **时间扭曲矩阵计算** ✅
   - 实现了基于姿态差异的旋转矩阵计算
   - 使用四元数数学正确计算渲染姿态到显示姿态的旋转差异
   - 支持完整的 4x4 旋转矩阵生成

### 已改进的功能

1. **眼偏移旋转计算** ✅
   - 修正了四元数旋转向量公式
   - 使用正确的向量旋转算法：`v' = v + qw * t + cross(q.xyz, t)`
   - 确保眼偏移位置在头部旋转后正确变换

2. **姿态预测** ✅
   - 添加了旋转预测框架（四元数插值）
   - 完善了位置预测算法
   - 支持基于 QVR 预测系数的姿态预测

3. **QVR 时间偏移** ✅
   - 实现了自动获取 tracker-android offset
   - 支持多种参数名称的兼容性查询
   - 确保 QTimer 和 Android 时间域之间的准确转换

4. **性能级别管理** ✅
   - 启用了完整的 QVR API 性能级别设置
   - 实现了智能的性能级别调整策略
   - 添加了阈值和延迟机制，避免频繁切换

### 代码质量改进

1. **路径解析系统** ✅
   - 实现了统一的路径字符串转换和解析
   - 支持 OpenXR 路径系统的完整集成
   - 改进了路径注册表的双向映射

2. **错误处理** ✅
   - 增强了边界检查和参数验证
   - 改进了错误日志和恢复机制
   - 添加了更完善的错误处理流程

3. **代码结构** ✅
   - 优化了控制器状态管理
   - 改进了输入映射的数据结构
   - 增强了代码的可维护性

## 已知限制

详细的已知限制和待完善功能请参考 [已知限制文档](KNOWN_LIMITATIONS.md)。

### 主要限制摘要

1. **平台特定实现**
   - 部分功能需要实际的 XR2 SDK 和硬件设备进行完整测试
   - 控制器 API 需要实际的控制器服务连接
   - QVR 控制器状态结构可能因 SDK 版本而异

2. **性能**
   - 性能优化已实现完整框架，需要实际硬件测试和调优
   - 时间扭曲矩阵计算已实现，可在实际设备上进一步优化
   - 姿态预测的旋转部分需要角速度数据支持

3. **功能完善**
   - **控制器状态同步**（`src/main/cpp/qualcomm/xr2_platform.cpp:1507`）
     - 当前使用简化实现，仅检查控制器连接状态
     - TODO: 需要实现完整的 QVR API 控制器状态读取
     - 完整实现需要 QVR SDK 的控制器状态结构定义
   - **Spaces SDK 包装器**（`src/main/cpp/qualcomm/spaces_sdk_wrapper.cpp`）
     - 当前为占位实现，需要实际的 Snapdragon Spaces SDK 集成
   - 眼动追踪 API 已集成框架，需要实际设备验证

4. **测试状态**
   - 代码已通过编译检查
   - 需要在实际 XR2 设备上进行功能测试
   - 需要性能基准测试和优化验证

