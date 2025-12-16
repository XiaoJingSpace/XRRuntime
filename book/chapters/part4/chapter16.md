# 第16章：OpenXR Loader 配置

## 16.1 Loader 机制原理

### OpenXR Loader 作用

OpenXR Loader 是 OpenXR 架构中的关键组件，它负责：

1. **Runtime 发现**: 查找并加载可用的 XR Runtime
2. **API 转发**: 将应用程序的 OpenXR API 调用转发到 Runtime
3. **版本管理**: 处理不同版本的 OpenXR API
4. **扩展管理**: 管理扩展的加载和启用

### Runtime 发现机制

#### 发现顺序

OpenXR Loader 按以下顺序查找 Runtime：

1. **应用级配置** (`assets/openxr_loader.json`)
2. **系统级配置** (`/vendor/etc/openxr/openxr_loader.json`)
3. **默认路径** (`/system/lib64/libopenxr_loader.so`)

#### 配置文件格式

Runtime 通过 JSON 配置文件声明：

```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "library_path": "libxrruntime.so",
        "name": "Qualcomm XR2 Runtime",
        "api_version": "1.1.0"
    }
}
```

### 加载优先级

#### 优先级规则

1. **应用级配置** > 系统级配置
2. **显式路径** > 默认路径
3. **多个 Runtime**: 按配置文件顺序尝试

#### 示例

如果同时存在应用级和系统级配置：

```
应用级配置 (assets/openxr_loader.json)
    ↓ (优先)
系统级配置 (/vendor/etc/openxr/openxr_loader.json)
    ↓ (备选)
默认 Runtime
```

## 16.2 JSON 配置文件

### 配置文件格式

#### 基本格式

```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "library_path": "libxrruntime.so",
        "name": "Qualcomm XR2 Runtime",
        "api_version": "1.1.0",
        "extensions": [
            "XR_KHR_android_create_instance",
            "XR_KHR_opengl_es_enable"
        ]
    }
}
```

#### 字段说明

- **file_format_version**: 配置文件格式版本（当前为 "1.0.0"）
- **runtime**: Runtime 配置对象
  - **library_path**: 库文件路径（相对于配置文件或绝对路径）
  - **name**: Runtime 名称
  - **api_version**: 支持的 OpenXR API 版本
  - **extensions**: 支持的扩展列表（可选）

### 应用级配置

#### 配置文件位置

```
your-app/
└── app/
    └── src/
        └── main/
            └── assets/
                └── openxr_loader.json
```

#### 配置示例

```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "library_path": "libxrruntime.so",
        "name": "Qualcomm XR2 Runtime",
        "api_version": "1.1.0",
        "extensions": [
            "XR_KHR_android_create_instance",
            "XR_KHR_opengl_es_enable",
            "XR_KHR_vulkan_enable"
        ]
    }
}
```

#### 库路径说明

- **相对路径**: `libxrruntime.so` - 相对于 APK 的 `lib/arm64-v8a/` 目录
- **绝对路径**: `/system/lib64/libxrruntime.so` - 系统库路径

### 系统级配置

#### 配置文件位置

```
/vendor/etc/openxr/openxr_loader.json
```

或

```
/system/etc/openxr/openxr_loader.json
```

#### 配置示例

```json
{
    "file_format_version": "1.0.0",
    "runtime": [
        {
            "library_path": "/system/lib64/libxrruntime.so",
            "name": "Qualcomm XR2 Runtime",
            "api_version": "1.1.0",
            "extensions": [
                "XR_KHR_android_create_instance",
                "XR_KHR_opengl_es_enable"
            ]
        }
    ]
}
```

#### 推送到系统

**Windows:**
```powershell
# 创建配置目录
adb shell mkdir -p /vendor/etc/openxr

# 推送配置文件
adb push openxr_loader.json /vendor/etc/openxr/openxr_loader.json

# 设置权限
adb shell chmod 644 /vendor/etc/openxr/openxr_loader.json
adb shell chown root:root /vendor/etc/openxr/openxr_loader.json
```

**Linux/macOS:**
```bash
# 创建配置目录
adb shell mkdir -p /vendor/etc/openxr

# 推送配置文件
adb push openxr_loader.json /vendor/etc/openxr/openxr_loader.json

# 设置权限
adb shell chmod 644 /vendor/etc/openxr/openxr_loader.json
adb shell chown root:root /vendor/etc/openxr/openxr_loader.json
```

## 16.3 多 Runtime 管理

### Runtime 优先级

#### 优先级顺序

1. **应用级配置** - 最高优先级
2. **系统级配置** - 中等优先级
3. **默认 Runtime** - 最低优先级

#### 多 Runtime 配置

系统级配置可以包含多个 Runtime：

```json
{
    "file_format_version": "1.0.0",
    "runtime": [
        {
            "library_path": "/system/lib64/libxrruntime.so",
            "name": "Qualcomm XR2 Runtime",
            "api_version": "1.1.0"
        },
        {
            "library_path": "/vendor/lib64/libother_runtime.so",
            "name": "Other Runtime",
            "api_version": "1.0.0"
        }
    ]
}
```

Loader 会按顺序尝试加载，直到找到可用的 Runtime。

### 运行时切换

#### 应用级切换

应用程序可以通过应用级配置文件选择特定的 Runtime：

```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "library_path": "/system/lib64/libxrruntime.so",
        "name": "Qualcomm XR2 Runtime",
        "api_version": "1.1.0"
    }
}
```

#### 系统级切换

修改系统级配置文件可以切换默认 Runtime：

```bash
# 编辑配置文件
adb shell
vi /vendor/etc/openxr/openxr_loader.json

# 或使用 sed 修改
adb shell "echo '{\"runtime\":{\"library_path\":\"/system/lib64/libxrruntime.so\"}}' > /vendor/etc/openxr/openxr_loader.json"
```

### 配置验证

#### 验证配置文件格式

使用 JSON 验证工具检查配置文件：

```bash
# 使用 Python 验证
python3 -m json.tool openxr_loader.json
```

#### 验证 Runtime 加载

查看日志确认 Runtime 是否成功加载：

```bash
adb logcat | grep -i "openxr"
```

应该看到类似：
```
I/OpenXR: Loader initialized
I/OpenXR: Found runtime: Qualcomm XR2 Runtime
I/OpenXR: Runtime loaded: /system/lib64/libxrruntime.so
```

#### 验证 API 版本

在应用程序中检查 API 版本：

```java
XrInstanceProperties instanceProps = new XrInstanceProperties();
xrGetInstanceProperties(instance, instanceProps);
Log.i("XR", "Runtime API Version: " + instanceProps.runtimeVersion);
```

## 常见问题

### 问题1: Runtime 未发现

**症状**: 应用程序无法找到 OpenXR Runtime

**解决方案**:
1. 检查配置文件是否存在
2. 验证配置文件格式是否正确
3. 确认库文件路径正确
4. 检查文件权限

### 问题2: 库文件加载失败

**症状**: Runtime 被发现但无法加载

**解决方案**:
1. 检查库文件是否存在
2. 验证库文件架构匹配（arm64-v8a）
3. 检查库文件依赖是否满足
4. 查看系统日志: `adb logcat | grep -i "dlopen"`

### 问题3: API 版本不匹配

**症状**: Runtime API 版本低于应用程序要求

**解决方案**:
1. 检查配置文件中的 `api_version`
2. 确认 Runtime 实际支持的 API 版本
3. 更新 Runtime 或降低应用程序 API 版本要求

## 本章小结

本章介绍了 OpenXR Loader 的机制原理、JSON 配置文件的格式和使用方法，以及多 Runtime 管理。正确配置 Loader 是应用程序能够使用 XR Runtime 的关键步骤。

## 下一步

- [第17章：Java/Kotlin 集成](chapter17.md)

