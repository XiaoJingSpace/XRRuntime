# 第5章：输出产物与验证

## 5.1 编译输出文件结构

### libxrruntime.so 位置

编译成功后，`libxrruntime.so` 文件会在多个位置出现：

#### 主要输出位置

1. **CMake 编译输出**（原始位置）
   - Debug: `app/build/intermediates/cxx/debug/obj/arm64-v8a/libxrruntime.so`
   - Release: `app/build/intermediates/cxx/release/obj/arm64-v8a/libxrruntime.so`

2. **自动复制位置**（用于部署）
   - `app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so`

3. **APK 内部位置**（打包后）
   - `app/build/outputs/apk/debug/app-debug.apk` (lib/arm64-v8a/libxrruntime.so)

### 中间文件说明

#### 目标文件 (.o)

位置: `app/build/intermediates/cxx/[buildType]/obj/arm64-v8a/`

- 每个 `.cpp` 文件编译后生成对应的 `.o` 文件
- 这些文件会被链接器合并成最终的 `.so` 文件

#### 链接映射文件

位置: `app/build/intermediates/cxx/[buildType]/obj/arm64-v8a/libxrruntime.so.map`

- 包含符号表信息
- 用于调试和分析

### AAR 文件生成

#### 生成 AAR

如果需要将库打包成 AAR 格式：

```bash
./gradlew assembleRelease
```

AAR 文件位置：
- `app/build/outputs/aar/app-release.aar`

#### AAR 结构

AAR 文件是一个 ZIP 压缩包，包含：
- `AndroidManifest.xml`
- `classes.jar`
- `jni/arm64-v8a/libxrruntime.so`
- `R.txt`
- `res/`

## 5.2 库文件验证

### 文件完整性检查

#### 检查文件是否存在

**Windows:**
```powershell
Test-Path app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so
```

**Linux/macOS:**
```bash
test -f app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so && echo "文件存在"
```

#### 检查文件大小

```bash
# Windows
(Get-Item app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so).Length

# Linux/macOS
ls -lh app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
```

正常的库文件大小应该在几 MB 到几十 MB 之间。

#### 检查文件类型

**Linux/macOS:**
```bash
file app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
```

应该显示类似：
```
libxrruntime.so: ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked
```

### 依赖库检查

#### 查看依赖库列表

**Linux:**
```bash
readelf -d app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so | grep NEEDED
```

**macOS:**
```bash
otool -L app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
```

#### 常见依赖库

- `libc.so` - C 标准库
- `libm.so` - 数学库
- `liblog.so` - Android 日志库
- `libandroid.so` - Android NDK 库
- `libEGL.so` - EGL 库
- `libGLESv3.so` - OpenGL ES 库

#### 检查缺失的依赖

在 Android 设备上检查：

```bash
adb shell "cd /data/local/tmp && ldd libxrruntime.so"
```

### 符号表验证

#### 查看导出符号

**Linux:**
```bash
nm -D app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so | grep -E "JNI_OnLoad|Java_"
```

应该能看到：
- `JNI_OnLoad` - JNI 入口函数
- OpenXR API 函数（如 `xrCreateInstance`）

#### 验证关键符号

检查是否包含必要的 OpenXR API：

```bash
nm -D app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so | grep xrCreateInstance
```

## 5.3 自动化构建脚本

### Windows 构建脚本

#### 完整构建脚本 (build.bat)

```batch
@echo off
setlocal

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=debug

echo 开始构建 %BUILD_TYPE% 版本...

call gradlew.bat assemble%BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
    echo 构建失败！
    exit /b 1
)

echo 构建成功！
echo 输出文件位置：
echo   - APK: app\build\outputs\apk\%BUILD_TYPE%\app-%BUILD_TYPE%.apk
echo   - SO: app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so

endlocal
```

#### 使用示例

```batch
# Debug 版本
scripts\build.bat debug

# Release 版本
scripts\build.bat release
```

### Linux/macOS 构建脚本

#### 完整构建脚本 (build.sh)

```bash
#!/bin/bash

BUILD_TYPE=${1:-debug}

echo "开始构建 $BUILD_TYPE 版本..."

./gradlew assemble${BUILD_TYPE^}

if [ $? -ne 0 ]; then
    echo "构建失败！"
    exit 1
fi

echo "构建成功！"
echo "输出文件位置："
echo "  - APK: app/build/outputs/apk/$BUILD_TYPE/app-$BUILD_TYPE.apk"
echo "  - SO: app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so"
```

#### 使用示例

```bash
chmod +x scripts/build.sh

# Debug 版本
./scripts/build.sh debug

# Release 版本
./scripts/build.sh release
```

### CI/CD 集成

#### GitHub Actions 示例

创建 `.github/workflows/build.yml`:

```yaml
name: Build XRRuntime

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up JDK
      uses: actions/setup-java@v3
      with:
        java-version: '11'
        distribution: 'temurin'
    
    - name: Setup Android SDK
      uses: android-actions/setup-android@v2
    
    - name: Setup OpenXR SDK
      run: |
        git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
        mkdir -p include/openxr
        cp -r OpenXR-SDK-Source/include/openxr/* include/openxr/
    
    - name: Grant execute permission for gradlew
      run: chmod +x gradlew
    
    - name: Build Debug
      run: ./gradlew assembleDebug
    
    - name: Build Release
      run: ./gradlew assembleRelease
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: libxrruntime.so
        path: app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
```

#### GitLab CI 示例

创建 `.gitlab-ci.yml`:

```yaml
image: android/android:30

before_script:
  - apt-get update -qq && apt-get install -y -qq git
  - git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
  - mkdir -p include/openxr
  - cp -r OpenXR-SDK-Source/include/openxr/* include/openxr/

build:
  script:
    - chmod +x gradlew
    - ./gradlew assembleRelease
  artifacts:
    paths:
      - app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so
    expire_in: 1 week
```

## 验证清单

编译完成后，使用以下清单验证：

- [ ] `.so` 文件存在于输出目录
- [ ] 文件大小合理（非零）
- [ ] 文件类型正确（ELF shared object）
- [ ] 包含必要的依赖库
- [ ] 包含 JNI_OnLoad 符号
- [ ] 包含 OpenXR API 函数符号
- [ ] APK 文件可以正常安装（如果生成）

## 本章小结

本章介绍了编译输出的文件结构、库文件验证方法以及自动化构建脚本的配置。正确验证输出产物是确保后续部署和使用成功的关键步骤。

## 下一步

完成编译和验证后，可以继续学习：
- [第6章：XRRuntime 框架架构设计](../part3/chapter06.md)
- 或直接跳转到 [第15章：运行时库文件部署](../part4/chapter15.md) 学习如何使用编译好的库

