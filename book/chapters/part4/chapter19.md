# 第19章：性能优化与调试

## 19.1 性能分析工具

### Android Profiler

#### 使用步骤

1. **打开 Android Studio**
2. **View → Tool Windows → Profiler**
3. **连接设备并运行应用**
4. **选择应用进程**

#### 监控指标

- **CPU**: CPU 使用率
- **Memory**: 内存使用
- **Network**: 网络流量
- **Energy**: 能耗

### Snapdragon Profiler

#### 安装与配置

1. 下载 Snapdragon Profiler
2. 连接 XR2 设备
3. 启动 Profiler

#### 关键指标

- **GPU 性能**: 帧率、延迟
- **CPU 性能**: 各核心使用率
- **功耗**: 实时功耗监控
- **追踪数据**: 头部追踪延迟

### 性能指标监控

#### 帧率监控

```java
private long lastFrameTime = 0;
private int frameCount = 0;
private float fps = 0;

private void updateFPS() {
    long currentTime = System.currentTimeMillis();
    frameCount++;
    
    if (currentTime - lastFrameTime >= 1000) {
        fps = frameCount;
        frameCount = 0;
        lastFrameTime = currentTime;
        Log.d("Performance", "FPS: " + fps);
    }
}
```

## 19.2 调试技巧

### Logcat 日志分析

#### 过滤日志

```bash
# 查看 XR Runtime 相关日志
adb logcat | grep -i "xrruntime"

# 查看 OpenXR 相关日志
adb logcat | grep -i "openxr"

# 查看错误日志
adb logcat | grep -i "error"
```

#### 日志级别

```java
Log.v("TAG", "Verbose");   // 详细
Log.d("TAG", "Debug");     // 调试
Log.i("TAG", "Info");      // 信息
Log.w("TAG", "Warning");   // 警告
Log.e("TAG", "Error");     // 错误
```

### GDB 调试

#### 附加到进程

```bash
# 启动 GDB Server
adb shell gdbserver :5039 --attach <pid>

# 在主机上连接
gdb
(gdb) target remote :5039
```

#### 设置断点

```gdb
(gdb) break xrCreateInstance
(gdb) continue
```

### 断点调试

#### Android Studio 调试

1. 在代码中设置断点
2. Debug → Attach Debugger to Android Process
3. 选择应用进程
4. 逐步调试

## 19.3 常见问题排查

### 库加载失败

#### 症状

```
dlopen failed: library "libxrruntime.so" not found
```

#### 排查步骤

1. 检查库文件是否存在
2. 验证库文件架构匹配
3. 检查库文件依赖
4. 查看系统日志

### Runtime 未发现

#### 症状

```
OpenXR Runtime not found
```

#### 排查步骤

1. 检查 OpenXR Loader 配置
2. 验证配置文件格式
3. 确认库文件路径正确
4. 检查文件权限

### 性能问题诊断

#### 帧率低

1. **检查 GPU 使用率**
   - 使用 Snapdragon Profiler
   - 检查是否有 GPU 瓶颈

2. **检查 CPU 使用率**
   - 查看各核心负载
   - 优化 CPU 密集型操作

3. **检查内存使用**
   - 避免内存泄漏
   - 优化内存分配

#### 延迟高

1. **检查追踪延迟**
   - 使用 QVR API 获取追踪数据时间戳
   - 优化姿态预测算法

2. **检查渲染延迟**
   - 优化渲染管线
   - 减少不必要的绘制调用

## 性能优化建议

### 1. 减少绘制调用

- 合并绘制对象
- 使用实例化渲染
- 优化几何体

### 2. 优化内存使用

- 及时释放资源
- 使用对象池
- 避免频繁分配

### 3. 优化 CPU 使用

- 多线程处理
- 避免主线程阻塞
- 优化算法复杂度

## 本章小结

本章介绍了性能分析工具的使用、调试技巧以及常见问题的排查方法。掌握这些技能对于开发和优化 XR 应用非常重要。

## 延伸阅读

- [Android Profiler 文档](https://developer.android.com/studio/profile/android-profiler)
- [Snapdragon Profiler 文档](https://developer.qualcomm.com/software/snapdragon-profiler)

