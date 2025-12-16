# IDE 版本检查错误解决方案

## 问题描述

错误信息：
```
This version of the Android Support plugin for IntelliJ IDEA (or Android Studio) 
cannot open this project, please retry with version 2021.2.1 or newer.
```

**原因**:
- Cursor/IntelliJ IDEA 的 Android 插件版本太旧
- AGP 7.4.2 要求 IDE 插件版本 >= 2021.2.1
- 这个检查只在 IDE 导入项目时触发，不影响命令行编译

## 解决方案

### 方案 1: 使用命令行编译（推荐）✅

**命令行编译不受 IDE 版本限制**，可以直接编译：

```powershell
# Windows
.\gradlew.bat assembleDebug

# Linux/macOS
./gradlew assembleDebug
```

### 方案 2: 升级 IDE 插件

如果需要在 IDE 中打开项目：

1. **更新 Cursor/IntelliJ IDEA**
   - Help → Check for Updates
   - 更新到最新版本

2. **更新 Android 插件**
   - File → Settings → Plugins
   - 搜索 "Android"
   - 更新到最新版本

### 方案 3: 使用 Android Studio

Android Studio 自带最新版本的 Android 插件：

1. 下载并安装 [Android Studio](https://developer.android.com/studio)
2. File → Open → 选择项目目录
3. 等待同步完成

## 已应用的修复

1. ✅ **NDK 版本指定** - 在 `app/build.gradle` 中添加了 `ndkVersion`
2. ✅ **IDE 检查配置** - 在 `gradle.properties` 中添加了相关配置

## 验证修复

### 命令行编译测试

```powershell
# 清理并编译
.\gradlew.bat clean
.\gradlew.bat assembleDebug
```

如果编译成功，说明项目配置正确，只是 IDE 版本问题。

### 检查编译输出

编译成功后，输出文件位置：
- SO 库: `app/build/intermediates/cmake/debug/obj/arm64-v8a/libxrruntime.so`
- APK: `app/build/outputs/apk/debug/app-debug.apk`

## 注意事项

- ✅ **命令行编译完全正常** - IDE 版本不影响命令行构建
- ⚠️ **IDE 功能受限** - 如果 IDE 版本太旧，可能无法使用某些 IDE 功能
- 💡 **推荐使用命令行** - 对于 C++/NDK 项目，命令行编译更稳定

## 相关文档

- [Cursor 编译指南](CURSOR_COMPILE_GUIDE.md) - 在 Cursor 中编译
- [构建指南](BUILD.md) - 详细构建说明

