# 附录E：故障排除速查表

## 问题-解决方案对照表

### 编译问题

| 问题 | 解决方案 |
|------|----------|
| OpenXR 头文件未找到 | 检查 `include/openxr/` 目录 |
| NDK 版本不兼容 | 使用 NDK r25+ |
| Gradle 版本错误 | 使用 Gradle 7.6 |
| CMake 版本错误 | 使用 CMake 3.22.1+ |

### 运行时问题

| 问题 | 解决方案 |
|------|----------|
| 库加载失败 | 检查库文件路径和权限 |
| Runtime 未发现 | 检查 OpenXR Loader 配置 |
| API 版本不匹配 | 检查配置文件中的 api_version |
| 权限不足 | 检查 SELinux 上下文 |

### 部署问题

| 问题 | 解决方案 |
|------|----------|
| adb remount 失败 | 确保设备已 root |
| 文件权限错误 | 使用 chmod 644 和 chown root:root |
| SELinux 阻止 | 检查并设置正确的 SELinux 上下文 |

## 错误代码说明

### OpenXR 错误码

- `XR_ERROR_VALIDATION_FAILURE` - 参数验证失败
- `XR_ERROR_RUNTIME_FAILURE` - 运行时错误
- `XR_ERROR_OUT_OF_MEMORY` - 内存不足
- `XR_ERROR_INSTANCE_LOST` - Instance 丢失
- `XR_ERROR_SESSION_LOST` - Session 丢失

### QVR 错误码

- `QVR_SUCCESS` - 成功
- `QVR_ERROR_NOT_INITIALIZED` - 未初始化
- `QVR_ERROR_INVALID_PARAMETER` - 无效参数
- `QVR_ERROR_SERVICE_NOT_AVAILABLE` - 服务不可用

## 调试命令

### 检查库文件

```bash
adb shell ls -la /system/lib64/libxrruntime.so
```

### 查看日志

```bash
adb logcat | grep -i "xrruntime"
```

### 检查进程

```bash
adb shell ps | grep xr
```

## 更多信息

参考各章节的"常见问题"部分获取详细解决方案。

