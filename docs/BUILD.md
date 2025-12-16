# 构建指南

本文档描述如何编译 XR Runtime 项目。

## 前置要求

### 1. Android SDK
- Android SDK Platform 31 (Android 12)
- Android SDK Build Tools 33.0.0+
- Android NDK r25+ (推荐 r25c 或更高版本)

### 2. 开发环境
- Android Studio Hedgehog (2023.1.1) 或更高版本
- CMake 3.22.1 或更高版本
- Gradle 8.0+
- JDK 8 或更高版本

### 3. 第三方库
- **OpenXR SDK Source 1.1.x**（必需）
  - 下载地址: https://github.com/KhronosGroup/OpenXR-SDK-Source
  - **重要**: 需要 SDK Source（源代码版本），不是预编译版本
  - 详细设置步骤请参考 [OpenXR SDK 设置指南](OPENXR_SDK_SETUP.md)
  - 头文件应放置在 `include/openxr/` 或 `libs/openxr/include/openxr/`
  
- **高通 XR2 SDK**（已包含）
  - Snapdragon XR SDK 4.0.5（已包含在项目中）
  - QVR API 头文件位于 `SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc`

## 环境配置

### 1. 设置环境变量

```bash
export ANDROID_HOME=/path/to/android/sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/25.2.9519653
export PATH=$PATH:$ANDROID_HOME/platform-tools
```

### 2. 配置项目

1. 克隆或下载项目代码
2. **设置 OpenXR SDK Source**（必需）
   - 参考 [OpenXR SDK 设置指南](OPENXR_SDK_SETUP.md) 获取详细步骤
   - 将 OpenXR SDK Source 的头文件复制到 `include/openxr/` 或 `libs/openxr/include/openxr/`
3. 高通 XR2 SDK 已包含在项目中，无需额外配置

### 3. 验证配置

CMakeLists.txt 会自动检查 OpenXR 头文件是否存在。如果缺失，编译时会显示清晰的错误信息。

## 编译步骤

### 方法 1: 使用 Android Studio

1. 打开 Android Studio
2. 选择 "Open an Existing Project"
3. 选择项目根目录
4. 等待 Gradle 同步完成
5. 选择 Build → Make Project
6. 或选择 Build → Build Bundle(s) / APK(s) → Build APK(s)

### 方法 2: 使用命令行

#### Debug 版本

```bash
./gradlew assembleDebug
```

#### Release 版本

```bash
./gradlew assembleRelease
```

#### 使用构建脚本

**Linux/macOS:**
```bash
chmod +x scripts/build.sh
./scripts/build.sh debug    # 或 release
```

**Windows:**
```batch
scripts\build.bat debug     # 或 release
```

## 输出位置

编译完成后，SO 库位置：

- Debug: `app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so`
- Release: `app/build/intermediates/cmake/release/obj/arm64-v8a/libxrruntime.so`

APK 位置：

- Debug: `app/build/outputs/apk/debug/app-debug.apk`
- Release: `app/build/outputs/apk/release/app-release.apk`

## 编译选项

### CMake 选项

在 `src/main/cpp/CMakeLists.txt` 中可以配置：

- `CMAKE_CXX_FLAGS`: C++ 编译选项
- `CMAKE_CXX_FLAGS_RELEASE`: Release 模式编译选项
- `CMAKE_CXX_FLAGS_DEBUG`: Debug 模式编译选项

### Gradle 选项

在 `build.gradle` 中可以配置：

- `minSdk`: 最小 SDK 版本（当前 31）
- `targetSdk`: 目标 SDK 版本（当前 33）
- `abiFilters`: ABI 过滤（当前仅 arm64-v8a）

## 常见问题

### 1. NDK 未找到

**错误**: `NDK not found`

**解决**: 设置 `ANDROID_NDK_HOME` 环境变量，或在 `local.properties` 中设置：
```
ndk.dir=/path/to/ndk
```

### 2. OpenXR 头文件未找到

**错误**: `fatal error: openxr/openxr.h: No such file or directory` 或 CMake 错误提示找不到 OpenXR SDK

**解决**: 
- 参考 [OpenXR SDK 设置指南](OPENXR_SDK_SETUP.md) 设置 OpenXR SDK Source
- 确保头文件已正确复制到 `include/openxr/` 或 `libs/openxr/include/openxr/`
- CMakeLists.txt 会自动检查并给出清晰的错误提示

### 3. 高通 SDK 库未找到

**错误**: `cannot find -lqualcomm_xr2`

**解决**: 确保高通 SDK 库已正确放置到 `libs/qualcomm/lib/`，并更新 CMakeLists.txt 中的库链接。

### 4. 编译错误：C++17 特性不支持

**错误**: C++17 特性编译错误

**解决**: 确保 NDK 版本 >= r25，CMake 版本 >= 3.22.1。

## 清理构建

```bash
./gradlew clean
```

## 增量构建

Gradle 和 CMake 都支持增量构建，只编译修改的文件。

## 性能优化

### Release 构建优化

Release 构建已启用以下优化：
- `-O3`: 最高级别优化
- `-DNDEBUG`: 禁用断言
- 链接时优化（如果支持）

### Debug 构建

Debug 构建包含：
- `-g`: 调试信息
- `-O0`: 无优化
- 符号信息

## 验证构建

构建完成后，验证 SO 库：

```bash
file app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so
```

应该显示：
```
libxrruntime.so: ELF 64-bit LSB shared object, ARM aarch64
```

## 下一步

构建完成后，参考 [DEPLOY.md](DEPLOY.md) 进行部署。

