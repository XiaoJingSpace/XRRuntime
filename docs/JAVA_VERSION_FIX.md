# Java 版本问题修复指南

## 问题描述

错误信息：
```
com/android/tools/lint/model/LintModelSeverity has been compiled by a more recent version of the Java Runtime (class file version 61.0), this version of the Java Runtime only recognizes class file versions up to 55.0
```

**原因**:
- Android Gradle Plugin 8.1.0 需要 **Java 17** 或更高版本
- 当前系统使用的是 **Java 11** (class file version 55.0)
- AGP 8.x 编译的类需要 Java 17 (class file version 61.0)

## 解决方案

### 方案 1: 升级 Java 版本（推荐）✅

#### Windows

1. **下载 Java 17**
   - 访问 [Oracle JDK 17](https://www.oracle.com/java/technologies/javase/jdk17-archive-downloads.html)
   - 或 [OpenJDK 17](https://adoptium.net/temurin/releases/?version=17)
   - 或使用 [Microsoft Build of OpenJDK](https://learn.microsoft.com/en-us/java/openjdk/download)

2. **安装 Java 17**
   - 运行安装程序
   - 记住安装路径（例如：`C:\Program Files\Java\jdk-17`）

3. **配置 Gradle 使用 Java 17**

   编辑 `gradle.properties`，添加：
   ```properties
   org.gradle.java.home=C:\\Program Files\\Java\\jdk-17
   ```
   
   或者设置环境变量 `JAVA_HOME`：
   ```powershell
   [System.Environment]::SetEnvironmentVariable('JAVA_HOME', 'C:\Program Files\Java\jdk-17', 'User')
   ```

#### Linux/macOS

1. **使用包管理器安装**

   **Ubuntu/Debian:**
   ```bash
   sudo apt update
   sudo apt install openjdk-17-jdk
   ```

   **macOS (Homebrew):**
   ```bash
   brew install openjdk@17
   ```

2. **配置 Gradle**

   编辑 `gradle.properties`，添加：
   ```properties
   org.gradle.java.home=/usr/lib/jvm/java-17-openjdk-amd64
   ```
   
   或设置环境变量：
   ```bash
   export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
   ```

### 方案 2: 使用 Gradle Toolchain（推荐）✅

在 `build.gradle` 中配置 Java toolchain：

```gradle
java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(17)
    }
}
```

### 方案 3: 降级 Android Gradle Plugin（不推荐）

如果无法升级 Java，可以降级 AGP：

```gradle
// build.gradle
buildscript {
    dependencies {
        classpath 'com.android.tools.build:gradle:7.4.2'  // 支持 Java 11
    }
}
```

**注意**: AGP 7.4.2 支持 Java 11，但功能可能不如 8.1.0 完整。

## 验证 Java 版本

### 检查当前 Java 版本

```bash
# Windows
java -version

# Linux/macOS
java -version
```

### 检查 Gradle 使用的 Java 版本

```bash
# Windows
.\gradlew.bat --version

# Linux/macOS
./gradlew --version
```

输出应该显示：
```
Gradle 8.0
...
JVM: 17.0.x (Oracle Corporation 17.0.x)
```

## 快速修复步骤

1. **安装 Java 17**（如果还没有）
2. **编辑 `gradle.properties`**，添加：
   ```properties
   org.gradle.java.home=C:\\Path\\To\\Java17
   ```
   替换为您的实际 Java 17 路径
3. **重新编译**：
   ```bash
   .\gradlew.bat clean
   .\gradlew.bat assembleDebug
   ```

## 常见问题

### Q: 如何找到 Java 17 安装路径？

**Windows:**
- 检查 `C:\Program Files\Java\`
- 或在 PowerShell 中运行：
  ```powershell
  Get-ChildItem "C:\Program Files\Java\" | Select-Object Name
  ```

**Linux:**
```bash
which java
readlink -f $(which java)
```

**macOS:**
```bash
/usr/libexec/java_home -V
```

### Q: 可以同时安装多个 Java 版本吗？

**可以！** 只需在 `gradle.properties` 中指定 Gradle 使用的版本即可。

### Q: Android Studio 使用的是不同的 Java 版本？

Android Studio 自带 JDK，但 Gradle 命令行可能使用系统 JDK。在 `gradle.properties` 中明确指定可以解决这个问题。

## 参考

- [Gradle Java Toolchains](https://docs.gradle.org/current/userguide/toolchains.html)
- [Android Gradle Plugin Requirements](https://developer.android.com/studio/releases/gradle-plugin#updating-gradle)
- [Java Version History](https://en.wikipedia.org/wiki/Java_version_history)

