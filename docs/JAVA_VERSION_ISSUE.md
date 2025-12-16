# Java 版本问题解决方案

## 问题诊断

**错误信息**:
```
class file version 61.0 (Java 17) vs class file versions up to 55.0 (Java 11)
```

**当前状态**:
- 系统 Java 版本: **Java 8** (1.8.0_302)
- 需要的 Java 版本: **Java 17+**
- Android Gradle Plugin 8.1.0 要求: **Java 17**

## 解决方案

### 方案 1: 升级到 Java 17（推荐）⭐

#### Windows 步骤

1. **下载 Java 17**
   - [Microsoft Build of OpenJDK 17](https://learn.microsoft.com/en-us/java/openjdk/download) - 推荐
   - [Adoptium OpenJDK 17](https://adoptium.net/temurin/releases/?version=17)
   - [Oracle JDK 17](https://www.oracle.com/java/technologies/javase/jdk17-archive-downloads.html)

2. **安装 Java 17**
   - 运行安装程序
   - 记住安装路径（例如：`C:\Program Files\Microsoft\jdk-17.0.x`）

3. **配置 Gradle 使用 Java 17**

   编辑 `gradle.properties`，设置 Java 路径：
   ```properties
   org.gradle.java.home=C:\\Program Files\\Microsoft\\jdk-17.0.x
   ```
   
   或者设置系统环境变量 `JAVA_HOME`：
   ```powershell
   [System.Environment]::SetEnvironmentVariable('JAVA_HOME', 'C:\Program Files\Microsoft\jdk-17.0.x', 'User')
   ```

4. **验证**
   ```powershell
   .\gradlew.bat --version
   ```
   应该显示 JVM: 17.x

### 方案 2: 降级 Android Gradle Plugin（快速修复）⚡

如果暂时无法升级 Java，可以降级 AGP 到支持 Java 8 的版本：

**修改 `build.gradle`**:
```gradle
buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath 'com.android.tools.build:gradle:7.4.2'  // 支持 Java 8
    }
}
```

**同时需要降级 Gradle 版本**（编辑 `gradle/wrapper/gradle-wrapper.properties`）:
```properties
distributionUrl=https\://services.gradle.org/distributions/gradle-7.6-bin.zip
```

**注意**: AGP 7.4.2 支持 Java 8，但功能可能不如 8.1.0 完整。

## 快速修复（降级方案）

我已经为您准备了降级配置，如果您想快速开始编译，可以使用降级方案。

## 推荐方案

**强烈推荐升级到 Java 17**，因为：
- Android Gradle Plugin 8.x 是未来趋势
- 更好的性能和功能
- 长期支持

## 验证修复

修复后，运行：
```powershell
.\gradlew.bat --version
```

应该看到：
- Gradle: 8.0
- JVM: 17.x (如果升级了 Java) 或 1.8.x (如果降级了 AGP)

