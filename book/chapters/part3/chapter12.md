# 第12章：输入系统实现

## 12.1 Action Set 管理

### Action Set 创建

创建动作集，组织相关的动作。

### Action 创建

创建动作，定义用户输入：
- Boolean Action
- Float Action
- Vector2f Action
- Pose Action

### 输入路径解析

解析输入路径字符串为 XrPath。

## 12.2 控制器输入处理

### QVR 控制器 API

使用 QVR API 获取控制器状态。

### 输入状态同步

同步控制器输入状态到 OpenXR Action。

### Action 映射机制

将 QVR 控制器输入映射到 OpenXR Action。

## 12.3 手势识别（可选）

### 手势检测

检测手部手势（如果支持）。

### 手势到 Action 映射

将手势识别结果映射到 OpenXR Action。

## 本章小结

本章介绍了输入系统的实现，包括 Action Set 管理、控制器输入处理和手势识别。

## 下一步

- [第13章：事件系统实现](chapter13.md)

