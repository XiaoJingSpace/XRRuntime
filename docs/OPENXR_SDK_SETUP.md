# OpenXR SDK 设置指南

本文档说明如何正确设置 OpenXR SDK Source，以便项目能够成功编译。

## 重要说明

本项目需要 **OpenXR SDK Source**（源代码版本），而不是预编译的 OpenXR SDK。

- **OpenXR SDK Source**: https://github.com/KhronosGroup/OpenXR-SDK-Source
- **OpenXR SDK** (预编译): https://github.com/KhronosGroup/OpenXR-SDK

## 为什么需要 SDK Source？

OpenXR SDK Source 包含：
- OpenXR 头文件（`include/openxr/`）
- OpenXR Loader 源代码
- 验证层和示例代码

对于 Runtime 开发，我们需要头文件来编译代码。

## 设置步骤

### 方法 1: 使用 Git 克隆（推荐）

```bash
# 克隆 OpenXR SDK Source 仓库
git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git

# 进入仓库目录
cd OpenXR-SDK-Source

# 切换到稳定版本（推荐使用最新的 1.1.x 版本）
git checkout release-1.1

# 返回项目根目录
cd ..
```

### 方法 2: 下载 ZIP 文件（推荐 - 使用预编译版本）

**注意**: OpenXR SDK Source 的头文件需要从 specification 生成。更简单的方法是使用预编译的 OpenXR SDK，它已经包含了生成好的头文件。

1. 访问 https://github.com/KhronosGroup/OpenXR-SDK/releases
2. 下载最新版本的 Source code (zip) - 这是预编译版本，包含生成好的头文件
3. 解压到项目根目录

**或者使用 OpenXR SDK Source**:
1. 访问 https://github.com/KhronosGroup/OpenXR-SDK-Source/releases
2. 下载最新版本的 Source code (zip)
3. 解压到项目根目录
4. 需要从 specification 生成头文件（需要 Python 和构建工具）

## 目录结构配置

项目支持两种目录结构配置方式：

### 方式 A: 使用 `include/` 目录（推荐）

1. 从 OpenXR SDK Source 复制头文件：
   ```bash
   # Windows (PowerShell)
   New-Item -ItemType Directory -Force -Path include\openxr
   Copy-Item -Path OpenXR-SDK-Source\include\openxr\* -Destination include\openxr\ -Recurse
   
   # Linux/macOS
   mkdir -p include/openxr
   cp -r OpenXR-SDK-Source/include/openxr/* include/openxr/
   ```

2. 最终目录结构：
   ```
   XRRuntimeStudy/
   ├── include/
   │   └── openxr/
   │       ├── openxr.h
   │       ├── openxr_platform.h
   │       └── ...
   └── ...
   ```

### 方式 B: 使用 `libs/openxr/include/` 目录

1. 创建目录结构：
   ```bash
   # Windows (PowerShell)
   New-Item -ItemType Directory -Force -Path libs\openxr\include
   
   # Linux/macOS
   mkdir -p libs/openxr/include
   ```

2. 复制头文件：
   ```bash
   # Windows (PowerShell)
   Copy-Item -Path OpenXR-SDK-Source\include\openxr\* -Destination libs\openxr\include\ -Recurse
   
   # Linux/macOS
   cp -r OpenXR-SDK-Source/include/openxr/* libs/openxr/include/
   ```

3. 最终目录结构：
   ```
   XRRuntimeStudy/
   ├── libs/
   │   └── openxr/
   │       └── include/
   │           └── openxr/
   │               ├── openxr.h
   │               ├── openxr_platform.h
   │               └── ...
   └── ...
   ```

## 验证设置

设置完成后，验证头文件是否存在：

```bash
# Windows (PowerShell)
Test-Path include\openxr\openxr.h
# 或
Test-Path libs\openxr\include\openxr\openxr.h

# Linux/macOS
test -f include/openxr/openxr.h
# 或
test -f libs/openxr/include/openxr/openxr.h
```

## CMakeLists.txt 配置

项目的 `src/main/cpp/CMakeLists.txt` 已经配置了多个可能的路径：

```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include          # 方式 A
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/openxr/include  # 方式 B
    ...
)
```

CMake 会按顺序查找这些路径，找到第一个存在的路径就会使用。

## 常见问题

### Q: 编译时提示找不到 `openxr/openxr.h`

**A**: 检查以下几点：
1. 确认已从 OpenXR SDK Source 复制了头文件
2. 确认目录结构正确（`include/openxr/` 或 `libs/openxr/include/openxr/`）
3. 确认文件名大小写正确（Linux/macOS 区分大小写）

### Q: 应该使用哪个版本的 OpenXR SDK Source？

**A**: 推荐使用 OpenXR 1.1.x 版本，与项目兼容。可以查看项目的 `README.md` 确认所需的版本。

### Q: 可以直接使用预编译的 OpenXR SDK 吗？

**A**: 可以！实际上，使用预编译的 OpenXR SDK（https://github.com/KhronosGroup/OpenXR-SDK）更简单，因为它已经包含了生成好的头文件。对于 Runtime 开发，我们只需要头文件，不需要构建整个 SDK。预编译版本的头文件可以直接使用，无需生成步骤。

### Q: 需要编译 OpenXR SDK Source 吗？

**A**: 不需要。我们只需要头文件。OpenXR Loader 会在运行时动态加载。

## 相关文档

- [构建指南](BUILD.md) - 完整的编译步骤
- [项目指南](PROJECT_GUIDE.md) - 项目完整说明
- [OpenXR 官方文档](https://www.khronos.org/openxr/)

## 下一步

设置完 OpenXR SDK 后，参考以下文档继续：
1. [构建指南](BUILD.md) - 编译项目
2. [部署指南](DEPLOY.md) - 部署到设备
3. [调试指南](DEBUG.md) - 调试和故障排除

