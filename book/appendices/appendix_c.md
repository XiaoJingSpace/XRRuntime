# 附录C：CMake 变量参考

## 常用 CMake 变量

### 项目变量

- `CMAKE_SOURCE_DIR` - 项目根目录
- `CMAKE_BINARY_DIR` - 构建目录
- `CMAKE_CURRENT_SOURCE_DIR` - 当前源目录
- `CMAKE_CURRENT_BINARY_DIR` - 当前构建目录

### 编译选项

- `CMAKE_CXX_STANDARD` - C++ 标准版本
- `CMAKE_CXX_FLAGS` - C++ 编译选项
- `CMAKE_CXX_FLAGS_DEBUG` - Debug 模式编译选项
- `CMAKE_CXX_FLAGS_RELEASE` - Release 模式编译选项

### Android 特定变量

- `ANDROID` - Android 平台标识
- `ANDROID_ABI` - Android ABI（如 arm64-v8a）
- `ANDROID_NDK` - NDK 路径
- `ANDROID_PLATFORM` - Android 平台版本

### 目录变量

- `CMAKE_INCLUDE_PATH` - 头文件搜索路径
- `CMAKE_LIBRARY_PATH` - 库文件搜索路径
- `CMAKE_PREFIX_PATH` - 前缀路径

## 编译选项说明

### 优化选项

- `-O0` - 无优化（Debug）
- `-O1` - 基本优化
- `-O2` - 标准优化
- `-O3` - 最高优化（Release）

### 调试选项

- `-g` - 包含调试信息
- `-DNDEBUG` - 禁用断言（Release）

### 警告选项

- `-Wall` - 启用所有警告
- `-Wextra` - 启用额外警告
- `-Werror` - 将警告视为错误

## 示例配置

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
```

## 更多信息

- [CMake 文档](https://cmake.org/documentation/)

