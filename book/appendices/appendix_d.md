# 附录D：Gradle 配置参考

## build.gradle 完整示例

### 根 build.gradle

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

### app/build.gradle

```gradle
plugins {
    id 'com.android.application'
}

android {
    compileSdkVersion 33
    
    defaultConfig {
        applicationId "com.example.xrapp"
        minSdkVersion 31
        targetSdkVersion 33
        versionCode 1
        versionName "1.0"
        
        ndk {
            abiFilters 'arm64-v8a'
        }
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

dependencies {
    implementation 'androidx.appcompat:appcompat:1.6.1'
}
```

## 配置项说明

### compileSdkVersion

编译时使用的 SDK 版本。

### minSdkVersion

最低支持的 Android 版本。

### targetSdkVersion

目标 Android 版本。

### ndkVersion

NDK 版本号。

### abiFilters

支持的 ABI 架构列表。

### externalNativeBuild

外部原生构建配置（CMake）。

## 更多信息

- [Android Gradle Plugin 文档](https://developer.android.com/studio/build)

