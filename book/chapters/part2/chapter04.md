# 第4章：编译实践

## 4.1 Android Studio 编译

### 项目导入

#### 导入步骤

1. **打开 Android Studio**
2. **选择 "Open an Existing Project"**
3. **选择项目根目录** (`XRRuntimeStudy`)
4. **等待 Gradle 同步完成**

> **截图位置**: 此处应添加 Android Studio 导入项目界面的截图

#### 首次导入注意事项

- 确保已配置 `local.properties` 文件
- 确保已下载 OpenXR SDK Source 头文件
- 等待 Gradle 下载依赖（可能需要较长时间）

> **截图位置**: 此处应添加 Gradle 同步过程的截图

### Gradle 同步

#### 同步过程

1. **自动同步**: Android Studio 会自动检测 `build.gradle` 变化并同步
2. **手动同步**: File → Sync Project with Gradle Files
3. **查看同步状态**: 底部状态栏显示同步进度

> **截图位置**: 此处应添加 Gradle 同步成功后的界面截图

#### 同步问题排查

如果同步失败：
1. 检查网络连接（需要下载依赖）
2. 检查 `local.properties` 配置
3. 检查 OpenXR 头文件是否存在
4. 查看 Gradle Console 错误信息

### 编译步骤详解

#### 方法1: 使用菜单

1. **Build → Make Project** (Ctrl+F9 / Cmd+F9)
   - 编译整个项目
   - 包括 C++ 代码和 Java 代码

2. **Build → Build Bundle(s) / APK(s) → Build APK(s)**
   - 编译并打包 APK
   - 包含所有资源文件

> **截图位置**: 此处应添加 Build 菜单的截图

#### 方法2: 使用 Gradle 面板

1. 打开右侧 **Gradle** 面板
2. 展开项目 → app → Tasks → build
3. 双击 `assembleDebug` 或 `assembleRelease`

> **截图位置**: 此处应添加 Gradle 面板的截图

### 输出文件定位

#### 编译输出位置

**Debug 版本:**
- `.so` 文件: `app/build/intermediates/cxx/debug/obj/arm64-v8a/libxrruntime.so`
- APK 文件: `app/build/outputs/apk/debug/app-debug.apk`

**Release 版本:**
- `.so` 文件: `app/build/intermediates/cxx/release/obj/arm64-v8a/libxrruntime.so`
- APK 文件: `app/build/outputs/apk/release/app-release.apk`

**自动复制位置:**
- `.so` 文件: `app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so`

> **截图位置**: 此处应添加文件浏览器中输出目录的截图

## 4.2 命令行编译

### Gradle Wrapper 使用

#### Windows

```powershell
.\gradlew.bat assembleDebug
```

#### Linux/macOS

```bash
./gradlew assembleDebug
```

#### 首次运行

首次运行会自动下载 Gradle 发行版（如果不存在）。

> **截图位置**: 此处应添加命令行编译过程的截图

### Debug/Release 编译

#### Debug 版本

```bash
# Windows
.\gradlew.bat assembleDebug

# Linux/macOS
./gradlew assembleDebug
```

特点：
- 包含调试信息
- 未优化
- 适合开发和调试

#### Release 版本

```bash
# Windows
.\gradlew.bat assembleRelease

# Linux/macOS
./gradlew assembleRelease
```

特点：
- 代码优化
- 不包含调试信息
- 适合发布

### 编译脚本使用

项目提供了便捷的编译脚本：

#### Windows (build.bat)

```batch
@echo off
if "%1"=="debug" (
    call gradlew.bat assembleDebug
) else if "%1"=="release" (
    call gradlew.bat assembleRelease
) else (
    echo Usage: build.bat [debug|release]
    exit /b 1
)
```

使用方法：
```batch
scripts\build.bat debug
scripts\build.bat release
```

#### Linux/macOS (build.sh)

```bash
#!/bin/bash
if [ "$1" == "debug" ]; then
    ./gradlew assembleDebug
elif [ "$1" == "release" ]; then
    ./gradlew assembleRelease
else
    echo "Usage: build.sh [debug|release]"
    exit 1
fi
```

使用方法：
```bash
chmod +x scripts/build.sh
./scripts/build.sh debug
./scripts/build.sh release
```

## 4.3 Cursor/VS Code 编译

### 任务配置

#### 创建 tasks.json

在 `.vscode/tasks.json` 中配置：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Gradle: Build Debug",
            "type": "shell",
            "command": "./gradlew",
            "args": ["assembleDebug"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        },
        {
            "label": "Gradle: Build Release",
            "type": "shell",
            "command": "./gradlew",
            "args": ["assembleRelease"],
            "group": "build",
            "problemMatcher": []
        }
    ]
}
```

> **截图位置**: 此处应添加 VS Code tasks.json 配置的截图

### 快捷键编译

#### Windows/Linux

- `Ctrl+Shift+B`: 运行默认构建任务

#### macOS

- `Cmd+Shift+B`: 运行默认构建任务

### 调试配置

#### 创建 launch.json

在 `.vscode/launch.json` 中配置：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Attach to Android",
            "type": "android",
            "request": "attach",
            "appSrcRoot": "${workspaceFolder}/app/src/main"
        }
    ]
}
```

## 4.4 编译问题诊断与解决

### 常见编译错误

#### 错误1: OpenXR 头文件未找到

**错误信息:**
```
fatal error: 'openxr/openxr.h' file not found
```

**解决方案:**
1. 确认已下载 OpenXR SDK Source
2. 检查头文件是否在 `include/openxr/` 目录
3. 验证 CMakeLists.txt 中的路径配置

> **截图位置**: 此处应添加错误信息的截图和解决方案的截图

#### 错误2: NDK 版本不兼容

**错误信息:**
```
NDK version is unsupported
```

**解决方案:**
1. 确保使用 NDK r25+ 版本
2. 在 `build.gradle` 中明确指定 NDK 版本
3. 清理项目并重新同步

#### 错误3: Gradle 版本不兼容

**错误信息:**
```
Gradle version X.X is required
```

**解决方案:**
1. 检查 `gradle/wrapper/gradle-wrapper.properties`
2. 确保使用 Gradle 7.6
3. 运行 `./gradlew wrapper --gradle-version 7.6`

### 链接错误处理

#### 错误1: 未定义的符号

**错误信息:**
```
undefined reference to 'function_name'
```

**解决方案:**
1. 检查函数声明和定义是否匹配
2. 确认所有源文件都添加到 CMakeLists.txt
3. 检查链接库是否正确

#### 错误2: 重复符号

**错误信息:**
```
multiple definition of 'symbol'
```

**解决方案:**
1. 检查是否有重复定义
2. 使用 `static` 或 `inline` 关键字
3. 使用头文件保护（include guards）

### 版本兼容性问题

#### AGP 版本问题

如果 Android Studio 提示 AGP 版本不支持：

1. 检查 Android Studio 版本
2. 确保使用 AGP 7.2.2（项目已配置）
3. 如果必须升级，需要同步更新 Gradle 版本

#### Java 版本问题

如果提示 Java 版本错误：

1. 确保使用 JDK 8 或更高版本
2. 在 Android Studio 中配置正确的 JDK 路径
3. 检查 `JAVA_HOME` 环境变量

### 依赖缺失问题

#### QVR 库缺失

**症状**: 运行时找不到 `libqvrservice_client.qti.so`

**解决方案:**
1. 确认设备上已安装 QVR Service
2. 检查库文件路径
3. 如果是开发测试，可以推送到设备

#### OpenXR Loader 缺失

**症状**: 应用无法找到 OpenXR Runtime

**解决方案:**
1. 配置 OpenXR Loader JSON 文件
2. 确保 Loader 配置文件路径正确
3. 参考 [第16章：OpenXR Loader 配置](../part4/chapter16.md)

## 编译成功验证

### 检查输出文件

```bash
# Windows
Test-Path app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so

# Linux/macOS
test -f app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
```

### 验证库文件

```bash
# 检查文件大小（不应为 0）
ls -lh app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so

# 检查依赖库（Android）
readelf -d app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so | grep NEEDED
```

> **截图位置**: 此处应添加编译成功后的输出文件截图

## 本章小结

本章介绍了三种编译方法：Android Studio、命令行和 Cursor/VS Code，以及常见编译问题的解决方案。掌握这些方法可以大大提高开发效率。

## 下一步

- [第5章：输出产物与验证](chapter05.md)
