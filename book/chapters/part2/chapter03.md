# 第3章：项目结构与构建系统

## 3.1 项目目录结构解析

### 整体目录结构

```
XRRuntimeStudy/
├── CMakeLists.txt              # 根 CMake 配置
├── build.gradle                # 根 Gradle 配置
├── AndroidManifest.xml         # Android 清单文件
├── local.properties            # 本地配置（SDK 路径）
├── include/                    # OpenXR SDK 头文件
│   └── openxr/
├── SnapdragonXR-SDK-source.rel.4.0.5/  # 高通 XR SDK
├── src/
│   └── main/
│       ├── cpp/                # C++ 源代码
│       ├── java/               # Java 代码
│       └── assets/            # 资源文件
├── app/
│   └── build.gradle           # App 构建配置
└── scripts/                   # 构建和部署脚本
```

### CMakeLists.txt 配置

根目录的 `CMakeLists.txt` 是项目的入口点：

```cmake
cmake_minimum_required(VERSION 3.22.1)
project(XRRuntime)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android specific settings
if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Add subdirectories
add_subdirectory(src/main/cpp)
```

### Gradle 构建配置

根目录的 `build.gradle` 配置项目级别的构建设置：

```gradle
buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath 'com.android.tools.build:gradle:7.2.2'
    }
}

allprojects {
    repositories {
        google()
        mavenCentral()
    }
}
```

### 源代码组织结构

#### C++ 源代码结构

```
src/main/cpp/
├── CMakeLists.txt              # C++ 模块 CMake 配置
├── main.cpp                    # 入口文件
├── openxr/                     # OpenXR API 实现
│   ├── openxr_api.cpp
│   ├── instance.cpp
│   ├── session.cpp
│   └── ...
├── platform/                   # 平台抽象层
│   ├── android_platform.cpp
│   ├── display_manager.cpp
│   └── ...
├── qualcomm/                   # 高通平台集成
│   ├── qvr_api_wrapper.cpp
│   └── ...
├── utils/                      # 工具类
│   ├── logger.cpp
│   └── ...
└── jni/                        # JNI 桥接
    └── jni_bridge.cpp
```

## 3.2 CMake 构建配置详解

### CMakeLists.txt 逐行解析

#### 基本配置

```cmake
cmake_minimum_required(VERSION 3.22.1)
```
指定最低 CMake 版本要求。

```cmake
project(XRRuntime)
```
定义项目名称。

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```
设置 C++ 标准为 C++17，并强制要求。

#### Android 特定配置

```cmake
if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
```
启用编译警告。

```cmake
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
```
Release 模式优化选项。

```cmake
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
```
Debug 模式调试选项。

### 编译选项配置

#### 常用编译选项

- `-Wall -Wextra`: 启用所有警告
- `-O3`: 最高优化级别
- `-g`: 包含调试信息
- `-DNDEBUG`: Release 模式下禁用断言

#### 平台特定选项

```cmake
if(ANDROID)
    # Android 特定配置
    set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
    set(CMAKE_ANDROID_NDK ${ANDROID_NDK})
endif()
```

### 依赖库链接

#### OpenXR SDK 头文件

```cmake
set(OPENXR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../include/openxr")
include_directories(${OPENXR_INCLUDE_DIR})
```

#### QVR API 头文件

```cmake
set(QVR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc")
include_directories(${QVR_INCLUDE_DIR})
```

#### 链接系统库

```cmake
target_link_libraries(xrruntime
    android
    log
    EGL
    GLESv3
)
```

## 3.3 Gradle 构建配置详解

### build.gradle 配置

#### App 模块配置

`app/build.gradle` 是应用模块的主要配置文件：

```gradle
android {
    compileSdkVersion 33
    
    defaultConfig {
        minSdkVersion 31
        targetSdkVersion 33
        versionCode 1
        versionName "1.0"
    }
    
    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt')
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.22.1"
        }
    }
    
    ndkVersion "25.2.9519653"
}
```

### NDK 版本指定

#### 方法1: 在 build.gradle 中指定

```gradle
android {
    ndkVersion "25.2.9519653"
}
```

#### 方法2: 在 local.properties 中指定

```properties
ndk.dir=D\:\\AndroidSDK\\ndk\\25.2.9519653
```

### ABI 过滤配置

```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a'
        }
    }
}
```

只编译 arm64-v8a 架构，减少编译时间。

### 自动复制任务配置

在 `app/build.gradle` 中添加任务，自动将编译好的 `.so` 文件复制到输出目录：

```gradle
android {
    // ... 其他配置
    
    applicationVariants.all { variant ->
        variant.outputs.all { output ->
            def buildType = variant.buildType.name
            def task = project.tasks.create("copyNativeLibs${variant.name.capitalize()}", Copy) {
                from("${project.buildDir}/intermediates/cxx/${buildType}/obj/arm64-v8a/")
                into("${project.buildDir}/outputs/jniLibs/arm64-v8a/")
                include("libxrruntime.so")
            }
            variant.assembleProvider.get().dependsOn(task)
        }
    }
}
```

## 3.4 编译流程与机制

### JNI 编译流程

#### 编译步骤

1. **Gradle 同步**
   - 读取 `build.gradle` 配置
   - 下载依赖
   - 配置 NDK 和 CMake

2. **CMake 配置**
   - 读取 `CMakeLists.txt`
   - 检测工具链
   - 配置编译选项

3. **C++ 编译**
   - 编译 `.cpp` 文件为 `.o` 目标文件
   - 链接生成 `.so` 动态库

4. **APK 打包**
   - 将 `.so` 文件打包到 APK
   - 生成最终的 APK 文件

### 静态库链接

如果项目依赖静态库，需要在 CMakeLists.txt 中链接：

```cmake
add_library(static_lib STATIC IMPORTED)
set_target_properties(static_lib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/libstatic.a
)

target_link_libraries(xrruntime static_lib)
```

### 动态库生成（libxrruntime.so）

#### 生成配置

```cmake
add_library(xrruntime SHARED
    ${SOURCES}
)

set_target_properties(xrruntime PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)
```

#### 输出位置

编译后的 `.so` 文件位置：
- Debug: `app/build/intermediates/cxx/debug/obj/arm64-v8a/libxrruntime.so`
- Release: `app/build/intermediates/cxx/release/obj/arm64-v8a/libxrruntime.so`

自动复制到：
- `app/build/outputs/jniLibs/arm64-v8a/libxrruntime.so`

## 本章小结

本章详细介绍了项目的目录结构、CMake 和 Gradle 构建配置，以及编译流程。理解这些内容对于后续的编译实践和问题排查非常重要。

## 下一步

- [第4章：编译实践](chapter04.md)

