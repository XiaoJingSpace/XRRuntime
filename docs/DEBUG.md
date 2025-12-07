# 调试指南

本文档描述如何调试 XR Runtime。

## 调试工具

### 1. Logcat

查看 Runtime 日志：

```bash
adb logcat | grep XRRuntime
```

查看所有相关日志：

```bash
adb logcat | grep -E "(XRRuntime|OpenXR|XR2)"
```

清除日志缓冲区：

```bash
adb logcat -c
```

### 2. 使用调试脚本

```bash
chmod +x scripts/debug.sh
./scripts/debug.sh logcat
```

## GDB 调试

### 前置要求

- NDK 包含 GDB
- 设备已 Root（推荐）或使用 `run-as`

### 步骤 1: 启动应用

```bash
adb shell am start -n com.xrruntime/.MainActivity
```

### 步骤 2: 获取进程 ID

```bash
PID=$(adb shell pidof -s com.xrruntime)
echo "Process ID: $PID"
```

### 步骤 3: 启动 GDB Server

```bash
adb shell "gdbserver :5039 --attach $PID" &
```

### 步骤 4: 端口转发

```bash
adb forward tcp:5039 tcp:5039
```

### 步骤 5: 连接 GDB

```bash
$ANDROID_NDK_HOME/prebuilt/linux-x86_64/bin/gdb \
  -ex "target remote :5039" \
  -ex "file app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so" \
  -ex "continue"
```

### 使用调试脚本

```bash
./scripts/debug.sh gdb
```

## 性能分析

### 1. CPU 性能分析

使用 `perf` 工具：

```bash
adb shell perf record -p <PID> -g sleep 10
adb pull /data/local/tmp/perf.data
```

### 2. 内存分析

查看内存使用：

```bash
adb shell dumpsys meminfo com.xrruntime
```

或使用脚本：

```bash
./scripts/debug.sh memcheck
```

### 3. 帧率监控

```bash
adb shell dumpsys gfxinfo com.xrruntime
```

## OpenXR 验证层

OpenXR 提供了验证层来检查 API 调用的正确性。

### 启用验证层

在创建 Instance 时启用：

```cpp
XrInstanceCreateInfo createInfo = {};
createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
createInfo.enabledApiLayerCount = 1;
const char* layers[] = { "XR_APILAYER_LUNARG_core_validation" };
createInfo.enabledApiLayerNames = layers;
```

### 查看验证层输出

验证层的输出会通过 Logcat 显示：

```bash
adb logcat | grep -E "(Validation|OpenXR)"
```

## 常见调试场景

### 1. Runtime 初始化失败

**症状**: `xrCreateInstance` 返回错误

**调试步骤**:
1. 检查日志：
   ```bash
   adb logcat | grep -E "(XRRuntime|Initialize)"
   ```
2. 检查平台初始化：
   - Android 平台是否初始化成功
   - XR2 平台是否初始化成功
3. 检查权限：
   ```bash
   adb shell dumpsys package com.xrruntime | grep permission
   ```

### 2. Session 创建失败

**症状**: `xrCreateSession` 返回错误

**调试步骤**:
1. 检查显示初始化：
   ```bash
   adb logcat | grep -E "(Display|EGL)"
   ```
2. 检查追踪初始化：
   ```bash
   adb logcat | grep -E "(Tracking|XR2)"
   ```
3. 检查图形绑定是否正确

### 3. 帧率不稳定

**症状**: 帧率波动或掉帧

**调试步骤**:
1. 监控帧时间：
   ```bash
   adb logcat | grep "Frame"
   ```
2. 检查 CPU 使用：
   ```bash
   adb shell top -n 1 | grep xrruntime
   ```
3. 检查内存压力：
   ```bash
   adb shell dumpsys meminfo com.xrruntime
   ```

### 4. 追踪丢失

**症状**: 空间定位失败

**调试步骤**:
1. 检查追踪状态：
   ```bash
   adb logcat | grep -E "(Tracking|Locate)"
   ```
2. 检查相机权限：
   ```bash
   adb shell dumpsys package com.xrruntime | grep camera
   ```
3. 检查追踪数据更新频率

### 5. 输入无响应

**症状**: 控制器输入无效

**调试步骤**:
1. 检查输入同步：
   ```bash
   adb logcat | grep -E "(Input|Action)"
   ```
2. 检查交互配置文件：
   ```bash
   adb logcat | grep "InteractionProfile"
   ```
3. 检查动作绑定是否正确

## 断点调试

### 在代码中添加断点

使用 `__builtin_trap()` 或 `raise(SIGTRAP)`：

```cpp
#include <csignal>

void DebugBreak() {
    raise(SIGTRAP);
}

// 在需要调试的地方调用
DebugBreak();
```

### 使用条件日志

```cpp
#ifdef DEBUG
    LOGD("Debug info: %s", debugString);
#endif
```

## 内存调试

### 检测内存泄漏

使用 AddressSanitizer（如果支持）：

在 CMakeLists.txt 中添加：
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
```

### 检测未初始化内存

使用 MemorySanitizer：

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
```

## 线程调试

### 检查死锁

添加线程 ID 到日志：

```cpp
#include <sys/syscall.h>
#include <unistd.h>

pid_t gettid() {
    return syscall(__NR_gettid);
}

LOGI("Thread %d: Message", gettid());
```

### 检查竞态条件

使用线程安全的数据结构，添加互斥锁检查。

## 网络调试（如果使用云追踪）

如果 Runtime 使用云追踪服务：

```bash
adb shell tcpdump -i any -w /sdcard/capture.pcap
```

## 性能基准测试

### 帧时间统计

```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... 操作 ...
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
LOGI("Operation took %lld microseconds", duration.count());
```

### 内存使用统计

```cpp
#include <malloc.h>

struct mallinfo info = mallinfo();
LOGI("Total allocated: %d bytes", info.uordblks);
```

## 远程调试

### 使用 Android Studio

1. 在 Android Studio 中打开项目
2. 选择 Run → Attach to Process
3. 选择目标进程
4. 设置断点并调试

### 使用 VS Code

配置 `.vscode/launch.json`：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Attach to XRRuntime",
            "type": "cppdbg",
            "request": "attach",
            "program": "${workspaceFolder}/app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so",
            "processId": "${command:pickProcess}",
            "MIMode": "gdb",
            "miDebuggerPath": "${env:ANDROID_NDK_HOME}/prebuilt/linux-x86_64/bin/gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```

## 日志级别

可以通过环境变量控制日志级别：

```bash
adb shell setprop log.tag.XRRuntime DEBUG
```

## 最佳实践

1. **使用有意义的日志标签**
   - 不同模块使用不同的标签
   - 便于过滤和查找

2. **记录关键操作**
   - API 调用入口和出口
   - 错误情况
   - 性能关键路径

3. **避免过度日志**
   - 在循环中谨慎使用日志
   - Release 构建中禁用详细日志

4. **使用断言**
   - 检查前置条件
   - 检查后置条件
   - Debug 构建中启用，Release 中禁用

5. **定期检查内存**
   - 使用工具检测泄漏
   - 监控内存使用趋势

## 下一步

调试完成后，参考测试文档进行验证。

