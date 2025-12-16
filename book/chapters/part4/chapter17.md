# 第17章：Java/Kotlin 集成

## 17.1 库加载方法

### System.loadLibrary 使用

#### Java 示例

```java
public class XRActivity extends Activity {
    static {
        try {
            System.loadLibrary("xrruntime");
            Log.i("XR", "XR Runtime library loaded");
        } catch (UnsatisfiedLinkError e) {
            Log.e("XR", "Failed to load library", e);
        }
    }
}
```

#### Kotlin 示例

```kotlin
class XRActivity : Activity() {
    companion object {
        init {
            try {
                System.loadLibrary("xrruntime")
                Log.i("XR", "XR Runtime library loaded")
            } catch (e: UnsatisfiedLinkError) {
                Log.e("XR", "Failed to load library", e)
            }
        }
    }
}
```

### 加载时机选择

#### 推荐时机

1. **静态初始化块**（推荐）
   - 在类加载时自动执行
   - 确保在使用前已加载

2. **Application.onCreate()**
   - 应用启动时加载
   - 适合全局使用

#### 避免的时机

- 在需要时才加载（可能导致延迟）
- 在非主线程加载（可能导致问题）

### 错误处理

#### 捕获异常

```java
try {
    System.loadLibrary("xrruntime");
} catch (UnsatisfiedLinkError e) {
    // 处理加载失败
    Log.e("XR", "Library load failed: " + e.getMessage());
    // 可以尝试备用方案或提示用户
}
```

## 17.2 JNI 接口调用

### Native 方法声明

#### 声明格式

```java
public class XRNative {
    // 加载库
    static {
        System.loadLibrary("xrruntime");
    }
    
    // 声明 native 方法
    public static native int nativeInitialize();
    public static native void nativeShutdown();
}
```

### 参数传递

#### 基本类型

```java
public static native int nativeSetValue(int value);
public static native float nativeGetFloat();
```

#### 对象传递

```java
public static native void nativeProcessData(byte[] data);
```

### 返回值处理

#### 返回值类型

```java
// 返回基本类型
public static native int nativeGetResult();

// 返回对象
public static native String nativeGetString();
```

## 17.3 Activity 集成示例

### 基础集成代码

#### 完整示例

```java
public class XRMainActivity extends Activity {
    private static final String TAG = "XRApp";
    
    static {
        System.loadLibrary("xrruntime");
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 初始化 OpenXR
        if (initializeOpenXR()) {
            Log.i(TAG, "OpenXR initialized");
        } else {
            Log.e(TAG, "OpenXR initialization failed");
            finish();
        }
    }
    
    private boolean initializeOpenXR() {
        // OpenXR 初始化代码
        // ...
        return true;
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        // 清理资源
    }
}
```

### 生命周期管理

#### 关键生命周期方法

```java
@Override
protected void onResume() {
    super.onResume();
    // 恢复 XR Session
}

@Override
protected void onPause() {
    super.onPause();
    // 暂停 XR Session
}

@Override
protected void onDestroy() {
    super.onDestroy();
    // 清理 XR 资源
}
```

### 错误处理模式

#### 统一错误处理

```java
private boolean handleXRResult(int result) {
    if (result != XR_SUCCESS) {
        Log.e(TAG, "XR Error: " + result);
        // 处理错误
        return false;
    }
    return true;
}
```

## 本章小结

本章介绍了 Java/Kotlin 中集成 XR Runtime 的方法，包括库加载、JNI 接口调用和 Activity 集成。正确集成是应用程序使用 XR Runtime 的基础。

## 下一步

- [第18章：OpenXR API 调用实践](chapter18.md)

