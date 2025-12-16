# 在 Cursor 中编译项目指南

本指南说明如何在 Cursor（或 VS Code）中直接编译 Android 项目。

## 前置要求

1. **安装必要的扩展**（推荐）
   - C/C++ (ms-vscode.cpptools)
   - Gradle for Java (vscjava.vscode-gradle)
   - CMake Tools (ms-vscode.cmake-tools)

2. **配置环境**
   - 创建 `local.properties` 文件
   - 配置 Android SDK 和 NDK 路径

## 编译方法

### 方法 1: 使用任务（Tasks）✅ 推荐

1. **打开命令面板**
   - 按 `Ctrl+Shift+P` (Windows/Linux) 或 `Cmd+Shift+P` (macOS)
   - 输入 "Tasks: Run Task"

2. **选择编译任务**
   - `Gradle: Build Debug` - 编译 Debug 版本（默认任务）
   - `Gradle: Build Release` - 编译 Release 版本
   - `Gradle: Clean` - 清理构建
   - `Gradle: Sync` - 同步 Gradle

3. **快捷键**
   - `Ctrl+Shift+B` (Windows/Linux) 或 `Cmd+Shift+B` (macOS) - 运行默认构建任务

### 方法 2: 使用集成终端

1. **打开终端**
   - 按 `` Ctrl+` `` (反引号) 或 `View → Terminal`

2. **运行 Gradle 命令**
   ```bash
   # Windows
   .\gradlew.bat assembleDebug
   
   # Linux/macOS
   ./gradlew assembleDebug
   ```

### 方法 3: 使用 Gradle 扩展

如果安装了 "Gradle for Java" 扩展：

1. 在侧边栏找到 Gradle 图标
2. 展开项目
3. 右键点击任务 → Run

## 配置说明

### tasks.json

已创建 `.vscode/tasks.json`，包含以下任务：
- `Gradle: Build Debug` - 默认构建任务
- `Gradle: Build Release` - Release 构建
- `Gradle: Clean` - 清理
- `Gradle: Sync` - 同步项目

### settings.json

已创建 `.vscode/settings.json`，配置了：
- C++ IntelliSense 包含路径
- 文件关联
- 排除构建目录

### launch.json

已创建 `.vscode/launch.json`，用于调试配置（需要 GDB）。

### extensions.json

已创建 `.vscode/extensions.json`，推荐安装的扩展。

## 编译步骤

### 第一次编译

1. **配置 local.properties**
   ```properties
   sdk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk
   ndk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk\\ndk\\25.2.9519653
   ```

2. **同步项目**
   - 运行任务: `Gradle: Sync`
   - 或终端: `.\gradlew.bat tasks`

3. **编译项目**
   - 按 `Ctrl+Shift+B` 运行默认构建任务
   - 或运行任务: `Gradle: Build Debug`

### 后续编译

直接按 `Ctrl+Shift+B` 即可编译。

## 查看编译输出

编译输出会显示在：
- **终端面板** - 如果使用终端命令
- **输出面板** - 如果使用任务（Tasks）

查看输出：
- `View → Output`
- 选择 "Tasks" 或 "Gradle" 输出通道

## 编译结果

编译成功后，输出文件位置：

**SO 库**:
- Debug: `app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so`
- Release: `app/build/intermediates/cmake/release/obj/arm64-v8a/libxrruntime.so`

**APK**:
- Debug: `app/build/outputs/apk/debug/app-debug.apk`
- Release: `app/build/outputs/apk/release/app-release.apk`

## 常见问题

### Q: 任务找不到 gradlew.bat

**A**: 确保：
1. 项目根目录有 `gradlew.bat` (Windows) 或 `gradlew` (Unix)
2. 文件有执行权限（Unix）

### Q: 编译失败：找不到 SDK

**A**: 
1. 检查 `local.properties` 是否存在
2. 检查 SDK 路径是否正确
3. 确保 Android SDK 已安装

### Q: 编译失败：找不到 NDK

**A**:
1. 在 `local.properties` 中设置 `ndk.dir`
2. 确保 Android NDK r25+ 已安装

### Q: IntelliSense 找不到头文件

**A**:
1. 重新加载窗口：`Ctrl+Shift+P` → "Reload Window"
2. 检查 `.vscode/settings.json` 中的包含路径
3. 确保 OpenXR SDK 头文件在 `include/openxr/` 目录

## 调试

### C++ 代码调试

1. 安装 C/C++ 扩展
2. 配置 `launch.json`（已创建）
3. 需要 GDB 和 Android NDK 的 GDB 工具

### Java 代码调试

1. 安装 Java 扩展包
2. 使用 Gradle 扩展的调试功能

## 快捷键参考

| 操作 | Windows/Linux | macOS |
|------|---------------|-------|
| 运行构建任务 | `Ctrl+Shift+B` | `Cmd+Shift+B` |
| 运行任务 | `Ctrl+Shift+P` → "Run Task" | `Cmd+Shift+P` → "Run Task" |
| 打开终端 | `` Ctrl+` `` | `` Ctrl+` `` |
| 命令面板 | `Ctrl+Shift+P` | `Cmd+Shift+P` |

## 推荐工作流

1. **编辑代码** - 在 Cursor 中编辑
2. **编译** - 按 `Ctrl+Shift+B` 编译
3. **查看错误** - 在 Problems 面板查看
4. **部署** - 使用脚本或手动部署

## 相关文档

- [构建指南](BUILD.md) - 详细的编译说明
- [调试指南](DEBUG.md) - 调试技巧
- [部署指南](DEPLOY.md) - 部署方法

---

**提示**: Cursor 基于 VS Code，所有 VS Code 的功能和扩展都可以使用。

