# 第14章：平台集成实现

## 14.1 Android 平台集成

### JNI 桥接实现

实现 JNI 桥接，连接 Java 层和 Native 层。

#### JNI_OnLoad

```cpp
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    // 初始化 Runtime
    InitializeXRRuntime();
    return JNI_VERSION_1_6;
}
```

### Android Activity 集成

集成 Android Activity 生命周期。

### EGL 上下文管理

管理 EGL 显示、上下文和表面。

## 14.2 QVR API 集成

### QVR Service Client

初始化和管理 QVR Service Client。

### VR Mode 管理

启动和停止 VR 模式。

### 性能级别管理

管理 CPU/GPU 性能级别。

## 14.3 时间戳处理

### 时间戳转换

在 OpenXR 时间戳和 QVR 时间戳之间转换。

### Tracker-Android Offset

处理追踪器和 Android 系统之间的时间偏移。

### 时间同步

确保时间戳的同步和准确性。

## 本章小结

本章介绍了平台集成的实现，包括 Android 平台集成、QVR API 集成和时间戳处理。

## 下一步

- [第15章：运行时库文件部署](../part4/chapter15.md)

