# 快速设置指南

## 解决 Gradle 连接问题

如果您遇到 "No connection to gradle server" 错误，请按以下步骤操作：

### 步骤 1: 在 Android Studio 中打开项目

1. 启动 Android Studio
2. 选择 **File → Open**
3. 选择项目根目录 `D:\WorkSpace\XRRuntimeStudy`
4. 点击 **OK**

### 步骤 2: 让 Android Studio 自动配置

Android Studio 会自动：
- 检测项目结构
- 下载 Gradle wrapper（如果需要）
- 同步项目

如果提示缺少 Gradle wrapper，选择：
- **Use Gradle wrapper** ✅
- 点击 **OK**

### 步骤 3: 配置 SDK 路径

1. 创建 `local.properties` 文件（如果不存在）
2. 添加以下内容（替换为您的实际路径）：

```properties
sdk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk
ndk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk\\ndk\\25.2.9519653
```

**如何找到 SDK 路径**：
- Android Studio → File → Settings → Appearance & Behavior → System Settings → Android SDK
- 查看 "Android SDK Location"

### 步骤 4: 同步项目

1. 点击工具栏的 **Sync Project with Gradle Files** 图标（🔄）
2. 或使用菜单：**File → Sync Project with Gradle Files**

### 步骤 5: 如果仍有问题

1. **清理并重启**：
   - File → Invalidate Caches / Restart
   - 选择 "Invalidate and Restart"

2. **停止 Gradle Daemon**：
   - File → Settings → Build, Execution, Deployment → Build Tools → Gradle
   - 点击 "Stop Gradle daemon"

3. **检查网络**：
   - 确保可以访问互联网（下载依赖需要）
   - 如果有代理，配置代理设置

## 验证设置

设置成功后，您应该看到：
- ✅ Gradle sync 成功
- ✅ 没有红色错误标记
- ✅ 可以展开项目结构

## 需要帮助？

如果问题仍然存在，请检查：
- [SETUP.md](SETUP.md) - 详细设置指南
- [BUILD.md](docs/BUILD.md) - 构建问题排查

