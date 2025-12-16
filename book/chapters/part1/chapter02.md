# 第2章：开发环境准备

## 2.1 Android 开发环境搭建

### Android SDK 安装与配置

#### 下载 Android SDK

1. 访问 [Android Developer 官网](https://developer.android.com/studio)
2. 下载 Android Studio（推荐）或 Command Line Tools
3. 安装 Android SDK Platform 31+ (Android 12)

#### 配置环境变量

**Windows (PowerShell):**
```powershell
$env:ANDROID_HOME = "D:\AndroidSDK"
$env:PATH += ";$env:ANDROID_HOME\platform-tools"
```

**Linux/macOS:**
```bash
export ANDROID_HOME=/path/to/android/sdk
export PATH=$PATH:$ANDROID_HOME/platform-tools
```

#### 验证安装

```bash
adb version
```

### Android NDK 安装与版本选择

#### 推荐版本

- **NDK r25c** 或更高版本
- 最低要求：NDK r23+

#### 安装方法

**方法1：通过 Android Studio**
1. Tools → SDK Manager
2. SDK Tools 标签页
3. 勾选 NDK (Side by side)
4. 选择版本并安装

**方法2：命令行安装**
```bash
sdkmanager "ndk;25.2.9519653"
```

#### 配置 NDK 路径

在项目的 `local.properties` 中配置：
```properties
sdk.dir=D:\\AndroidSDK
# NDK 路径会在 build.gradle 中自动检测
```

### Android Studio 配置

#### 必需插件

- Android SDK Platform 31+
- Android SDK Build Tools 33.0.0+
- CMake 3.22.1+
- NDK (Side by side)

#### IDE 设置

1. **File → Settings → Appearance & Behavior → System Settings → Android SDK**
   - 确保安装了正确的 SDK Platform 和 Build Tools

2. **File → Settings → Build, Execution, Deployment → Build Tools → Gradle**
   - 使用 Gradle Wrapper（推荐）

## 2.2 构建工具链配置

### CMake 安装与配置

#### 安装 CMake

**Windows:**
- 通过 Android Studio 自动安装（推荐）
- 或从 [CMake 官网](https://cmake.org/download/) 下载

**Linux:**
```bash
sudo apt-get install cmake
```

**macOS:**
```bash
brew install cmake
```

#### 验证安装

```bash
cmake --version
# 应显示 3.22.1 或更高版本
```

### Gradle 配置与版本管理

#### Gradle 版本

项目使用 **Gradle 7.6**，配置在 `gradle/wrapper/gradle-wrapper.properties`:

```properties
distributionUrl=https\://services.gradle.org/distributions/gradle-7.6-bin.zip
```

#### Android Gradle Plugin (AGP)

项目使用 **AGP 7.2.2**，配置在 `build.gradle`:

```gradle
buildscript {
    dependencies {
        classpath 'com.android.tools.build:gradle:7.2.2'
    }
}
```

### JDK 版本要求

#### 推荐版本

- **JDK 8** 或更高版本
- 推荐使用 JDK 11 或 JDK 17

#### 配置 JDK

在 Android Studio 中：
1. File → Project Structure → SDK Location
2. 设置 JDK location

## 2.3 OpenXR SDK Source 获取与配置

### OpenXR SDK Source 下载

#### 下载地址

- GitHub: https://github.com/KhronosGroup/OpenXR-SDK-Source
- 推荐版本: **1.1.x** (release-1.1 分支)

#### 下载方法

```bash
# 克隆仓库
git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
cd OpenXR-SDK-Source

# 切换到稳定版本
git checkout release-1.1
```

### 头文件目录结构

#### 目录结构

```
OpenXR-SDK-Source/
├── include/
│   └── openxr/
│       ├── openxr.h
│       ├── openxr_platform.h
│       ├── openxr_reflection.h
│       └── ...
```

#### 复制到项目

将头文件复制到项目的 `include/openxr/` 目录：

**Windows:**
```powershell
xcopy /E /I OpenXR-SDK-Source\include\openxr include\openxr
```

**Linux/macOS:**
```bash
cp -r OpenXR-SDK-Source/include/openxr include/
```

### 版本选择与兼容性

#### 版本要求

- **最低版本**: OpenXR 1.0
- **推荐版本**: OpenXR 1.1.x
- **API 版本**: XR_CURRENT_API_VERSION (1.1.54)

#### 验证头文件

检查关键头文件是否存在：
```bash
# Windows
Test-Path include\openxr\openxr.h

# Linux/macOS
test -f include/openxr/openxr.h
```

## 2.4 Snapdragon XR SDK 集成

### QVR API 头文件位置

项目已包含 Snapdragon XR SDK 4.0.5，QVR API 头文件位于：

```
SnapdragonXR-SDK-source.rel.4.0.5/
└── 3rdparty/
    └── qvr/
        └── inc/
            ├── qvr.h
            ├── qvrservice_client.h
            └── ...
```

### 库依赖关系

#### 运行时依赖

- `libqvrservice_client.qti.so` - QVR Service Client 库（设备上应已存在）

#### 编译时依赖

- QVR API 头文件（已包含在项目中）
- OpenXR SDK 头文件（需要手动配置）

### 平台特定配置

#### CMake 配置

在 `CMakeLists.txt` 中已配置 QVR API 路径：

```cmake
set(QVR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc")
```

## 2.5 环境验证与测试

### 环境变量配置

#### 必需的环境变量

**Windows:**
```powershell
$env:ANDROID_HOME = "D:\AndroidSDK"
$env:ANDROID_NDK_HOME = "$env:ANDROID_HOME\ndk\25.2.9519653"
```

**Linux/macOS:**
```bash
export ANDROID_HOME=/path/to/android/sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/25.2.9519653
```

### 路径验证脚本

创建验证脚本 `scripts/verify_env.sh` (Linux/macOS) 或 `scripts/verify_env.ps1` (Windows):

**Windows (verify_env.ps1):**
```powershell
Write-Host "验证开发环境..." -ForegroundColor Cyan

# 检查 Android SDK
if (Test-Path $env:ANDROID_HOME) {
    Write-Host "✓ Android SDK: $env:ANDROID_HOME" -ForegroundColor Green
} else {
    Write-Host "✗ Android SDK 未找到" -ForegroundColor Red
}

# 检查 NDK
if (Test-Path $env:ANDROID_NDK_HOME) {
    Write-Host "✓ Android NDK: $env:ANDROID_NDK_HOME" -ForegroundColor Green
} else {
    Write-Host "✗ Android NDK 未找到" -ForegroundColor Red
}

# 检查 OpenXR 头文件
if (Test-Path "include\openxr\openxr.h") {
    Write-Host "✓ OpenXR SDK 头文件已配置" -ForegroundColor Green
} else {
    Write-Host "✗ OpenXR SDK 头文件未找到" -ForegroundColor Red
}

# 检查 CMake
$cmakeVersion = cmake --version 2>$null
if ($cmakeVersion) {
    Write-Host "✓ CMake 已安装" -ForegroundColor Green
} else {
    Write-Host "✗ CMake 未安装" -ForegroundColor Red
}
```

### 常见环境问题排查

#### 问题1: Android SDK 路径错误

**症状**: Gradle 同步失败，提示找不到 SDK

**解决方案**:
1. 检查 `local.properties` 中的 `sdk.dir` 路径
2. 确保路径使用正确的分隔符（Windows 使用 `\\`）
3. 验证路径是否存在

#### 问题2: NDK 版本不兼容

**症状**: CMake 配置失败

**解决方案**:
1. 确保使用 NDK r25+ 版本
2. 在 `build.gradle` 中明确指定 NDK 版本
3. 清理并重新同步项目

#### 问题3: OpenXR 头文件缺失

**症状**: CMake 编译错误，提示找不到 `openxr.h`

**解决方案**:
1. 确认已下载 OpenXR SDK Source
2. 检查头文件是否在 `include/openxr/` 目录
3. 参考 [OpenXR SDK 设置指南](../../../docs/OPENXR_SDK_SETUP.md)

#### 问题4: JDK 版本不兼容

**症状**: Gradle 构建失败，提示 Java 版本错误

**解决方案**:
1. 确保使用 JDK 8 或更高版本
2. 在 Android Studio 中配置正确的 JDK 路径
3. 检查 `JAVA_HOME` 环境变量

## 本章小结

本章详细介绍了开发环境的搭建过程，包括 Android SDK、NDK、CMake、Gradle 等工具的安装和配置，以及 OpenXR SDK Source 和 Snapdragon XR SDK 的集成方法。正确配置开发环境是后续编译和开发的基础。

## 下一步

完成环境配置后，可以继续学习：
- [第3章：项目结构与构建系统](../part2/chapter03.md)

