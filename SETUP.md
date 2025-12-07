# 项目设置指南

## 解决 "No connection to gradle server" 错误

### 方法 1: 使用 Android Studio（推荐）

1. **打开项目**
   - 在 Android Studio 中选择 File → Open
   - 选择项目根目录 `XRRuntimeStudy`

2. **让 Android Studio 自动设置**
   - Android Studio 会自动检测缺少的 Gradle wrapper
   - 选择 "Use Gradle wrapper" 选项
   - 等待 Gradle 同步完成

3. **如果仍然有问题**
   - File → Invalidate Caches / Restart
   - 选择 "Invalidate and Restart"

### 方法 2: 手动创建 Gradle Wrapper

如果 Android Studio 无法自动创建 wrapper，可以手动创建：

1. **下载 Gradle Wrapper JAR**
   ```powershell
   # 创建目录
   New-Item -ItemType Directory -Force -Path gradle\wrapper
   
   # 下载 gradle-wrapper.jar（需要手动下载或使用 Android Studio）
   # 或者运行以下命令（如果有 gradle 安装）：
   gradle wrapper --gradle-version 8.0
   ```

2. **或者使用 Android Studio**
   - File → Settings → Build, Execution, Deployment → Build Tools → Gradle
   - 选择 "Use Gradle wrapper task configuration"
   - 点击 "Create" 按钮

### 方法 3: 配置 local.properties

创建 `local.properties` 文件（已提供 `local.properties.example` 作为模板）：

```properties
sdk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk
ndk.dir=C:\\Users\\YourUsername\\AppData\\Local\\Android\\Sdk\\ndk\\25.2.9519653
```

将路径替换为您的实际 Android SDK 和 NDK 路径。

### 方法 4: 重启 Gradle Daemon

如果 Gradle daemon 卡住：

```powershell
# 停止所有 Gradle daemon
gradle --stop

# 或者在 Android Studio 中
# File → Settings → Build, Execution, Deployment → Build Tools → Gradle
# 点击 "Stop Gradle daemon"
```

## 验证设置

设置完成后，验证项目配置：

1. **检查 Gradle Wrapper**
   - 确认 `gradle/wrapper/gradle-wrapper.properties` 存在
   - 确认 `gradle/wrapper/gradle-wrapper.jar` 存在（Android Studio 会自动下载）

2. **检查 local.properties**
   - 确认 `local.properties` 存在且路径正确

3. **同步项目**
   - 在 Android Studio 中点击 "Sync Project with Gradle Files"
   - 或使用菜单：File → Sync Project with Gradle Files

## 常见问题

### 问题 1: "Gradle sync failed"

**解决方案**:
- 检查 `local.properties` 中的 SDK 路径是否正确
- 检查网络连接（需要下载依赖）
- 检查 Android Studio 的 Gradle 设置

### 问题 2: "NDK not found"

**解决方案**:
- 在 `local.properties` 中设置 `ndk.dir`
- 或在 Android Studio 中：File → Project Structure → SDK Location → Android NDK location

### 问题 3: "CMake not found"

**解决方案**:
- 安装 CMake：Android Studio → SDK Manager → SDK Tools → CMake
- 确保版本 >= 3.22.1

## 下一步

设置完成后，参考以下文档：
- [构建指南](docs/BUILD.md) - 如何编译项目
- [部署指南](docs/DEPLOY.md) - 如何部署到设备
- [调试指南](docs/DEBUG.md) - 如何调试

