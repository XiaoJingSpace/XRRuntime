# 第18章：OpenXR API 调用实践

## 18.1 Instance 创建

### 创建流程

#### 基本流程

```java
// 1. 准备创建信息
XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
createInfo.setApplicationInfo(
    "My XR App",           // applicationName
    1,                     // applicationVersion
    "My Engine",           // engineName
    1,                     // engineVersion
    XrVersion.XR_CURRENT_API_VERSION
);

// 2. 添加扩展
createInfo.addEnabledExtension("XR_KHR_android_create_instance");
createInfo.addEnabledExtension("XR_KHR_opengl_es_enable");

// 3. 创建 Instance
XrInstance instance = new XrInstance();
int result = XR.xrCreateInstance(createInfo, instance);

if (result == XR_SUCCESS) {
    Log.i("XR", "Instance created successfully");
} else {
    Log.e("XR", "Failed to create instance: " + result);
}
```

### 扩展启用

#### Android 扩展

```java
// Android 特定扩展
XrInstanceCreateInfoAndroidKHR androidInfo = 
    new XrInstanceCreateInfoAndroidKHR();
androidInfo.setApplicationVM(getApplicationInfo().nativeLibraryDir);
androidInfo.setApplicationActivity(this);
createInfo.setNext(androidInfo);
```

### Android 特定配置

#### 完整配置示例

```java
XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
createInfo.setApplicationInfo("MyApp", 1, "MyEngine", 1, 
    XrVersion.XR_CURRENT_API_VERSION);

// Android 扩展
createInfo.addEnabledExtension("XR_KHR_android_create_instance");
createInfo.addEnabledExtension("XR_KHR_opengl_es_enable");

// Android 特定信息
XrInstanceCreateInfoAndroidKHR androidInfo = 
    new XrInstanceCreateInfoAndroidKHR();
androidInfo.setApplicationVM(getApplicationInfo().nativeLibraryDir);
androidInfo.setApplicationActivity(this);
createInfo.setNext(androidInfo);

XrInstance instance = new XrInstance();
XR.xrCreateInstance(createInfo, instance);
```

## 18.2 Session 创建与使用

### Session 创建

#### 创建步骤

```java
// 1. 获取 System
XrSystemGetInfo systemInfo = new XrSystemGetInfo();
systemInfo.setFormFactor(XrFormFactor.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY);

XrSystemId systemId = new XrSystemId();
XR.xrGetSystem(instance, systemInfo, systemId);

// 2. 创建 Session
XrSessionCreateInfo sessionInfo = new XrSessionCreateInfo();
sessionInfo.setSystemId(systemId);

// 图形绑定（OpenGL ES）
XrGraphicsBindingOpenGLESAndroidKHR glBinding = 
    new XrGraphicsBindingOpenGLESAndroidKHR();
glBinding.setDisplay(eglDisplay);
glBinding.setConfig(eglConfig);
glBinding.setContext(eglContext);
sessionInfo.setNext(glBinding);

XrSession session = new XrSession();
XR.xrCreateSession(instance, sessionInfo, session);
```

### 渲染循环

#### 基本循环

```java
while (isRunning) {
    // 1. 等待帧
    XrFrameWaitInfo waitInfo = new XrFrameWaitInfo();
    XrFrameState frameState = new XrFrameState();
    XR.xrWaitFrame(session, waitInfo, frameState);
    
    // 2. 开始帧
    XrFrameBeginInfo beginInfo = new XrFrameBeginInfo();
    XR.xrBeginFrame(session, beginInfo);
    
    // 3. 渲染
    renderFrame(frameState);
    
    // 4. 结束帧
    XrFrameEndInfo endInfo = new XrFrameEndInfo();
    endInfo.setDisplayTime(frameState.predictedDisplayTime);
    // 添加层...
    XR.xrEndFrame(session, endInfo);
}
```

### 资源清理

#### 清理顺序

```java
// 1. 结束 Session
if (session != null) {
    XR.xrEndSession(session);
    XR.xrDestroySession(session);
}

// 2. 销毁 Instance
if (instance != null) {
    XR.xrDestroyInstance(instance);
}
```

## 18.3 完整应用示例

### 最小化示例

#### 最简单的 XR 应用

```java
public class SimpleXRActivity extends Activity {
    static {
        System.loadLibrary("xrruntime");
    }
    
    private XrInstance instance;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 创建 Instance
        XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
        createInfo.setApplicationInfo("SimpleApp", 1, "Engine", 1, 
            XrVersion.XR_CURRENT_API_VERSION);
        
        instance = new XrInstance();
        XR.xrCreateInstance(createInfo, instance);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (instance != null) {
            XR.xrDestroyInstance(instance);
        }
    }
}
```

### 完整 VR 应用

#### 完整示例框架

```java
public class FullVRActivity extends Activity {
    private XrInstance instance;
    private XrSession session;
    private XrSystemId systemId;
    private boolean isRunning = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        if (initializeOpenXR()) {
            startRenderLoop();
        }
    }
    
    private boolean initializeOpenXR() {
        // 创建 Instance
        // 获取 System
        // 创建 Session
        // ...
        return true;
    }
    
    private void startRenderLoop() {
        isRunning = true;
        new Thread(() -> {
            while (isRunning) {
                renderFrame();
            }
        }).start();
    }
    
    private void renderFrame() {
        // 渲染逻辑
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        isRunning = false;
        // 清理资源
    }
}
```

### 最佳实践

#### 1. 错误处理

```java
private boolean checkXRResult(int result, String operation) {
    if (result != XR_SUCCESS) {
        Log.e("XR", operation + " failed: " + result);
        return false;
    }
    return true;
}
```

#### 2. 资源管理

```java
// 使用 try-finally 确保资源清理
try {
    // 使用 XR 资源
} finally {
    // 清理资源
}
```

#### 3. 线程安全

```java
// 在主线程进行 XR API 调用
runOnUiThread(() -> {
    XR.xrCreateSession(...);
});
```

## 本章小结

本章介绍了 OpenXR API 的调用实践，包括 Instance 创建、Session 管理和渲染循环。掌握这些内容是开发 XR 应用的基础。

## 下一步

- [第19章：性能优化与调试](chapter19.md)

