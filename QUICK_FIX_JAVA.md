# Java 版本问题快速修复

## 问题
- 当前 Java: **Java 8**
- 需要 Java: **Java 17** (for AGP 8.1.0)

## 快速修复选项

### 选项 A: 升级到 Java 17（推荐）⭐

1. **下载 Java 17**
   ```
   https://adoptium.net/temurin/releases/?version=17
   ```
   选择 Windows x64 版本

2. **安装后，编辑 `gradle.properties`**
   ```properties
   org.gradle.java.home=C:\\Program Files\\Eclipse Adoptium\\jdk-17.0.x-hotspot
   ```
   替换为您的实际安装路径

3. **验证**
   ```powershell
   .\gradlew.bat --version
   ```
   应该显示 JVM: 17.x

### 选项 B: 降级到支持 Java 8 的版本（快速）⚡

如果您暂时无法升级 Java，可以降级配置：

1. **备份当前文件**
   ```powershell
   Copy-Item build.gradle build.gradle.backup
   Copy-Item gradle\wrapper\gradle-wrapper.properties gradle\wrapper\gradle-wrapper.properties.backup
   ```

2. **使用 Java 8 兼容配置**
   ```powershell
   # 复制备用配置
   Copy-Item build.gradle.java8 build.gradle -Force
   Get-Content gradle-wrapper.properties.java8 | Set-Content gradle\wrapper\gradle-wrapper.properties
   ```

3. **重新编译**
   ```powershell
   .\gradlew.bat clean
   .\gradlew.bat assembleDebug
   ```

## 推荐方案

**强烈推荐升级到 Java 17**，因为：
- ✅ 更好的性能
- ✅ 支持最新的 Android Gradle Plugin
- ✅ 长期支持

降级方案只是临时解决方案。

