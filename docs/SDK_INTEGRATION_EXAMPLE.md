# SDK 集成示例

本文档提供完整的示例代码，展示如何在应用 SDK 中使用 XR Runtime。

## 快速开始

### 1. 添加依赖

在你的应用的 `build.gradle` 中添加：

```gradle
dependencies {
    // OpenXR Loader (如果使用)
    implementation 'org.khronos.openxr:openxr_loader:1.1.0'
    
    // 或者直接使用原生库
    // 将 libxrruntime.so 放到 app/src/main/jniLibs/arm64-v8a/
}
```

### 2. 加载库

```java
public class XRManager {
    static {
        // 加载 XR Runtime 库
        System.loadLibrary("xrruntime");
    }
    
    // 库会在加载时自动初始化（通过 JNI_OnLoad）
}
```

---

## 完整示例：OpenXR 应用

### Java 版本

```java
package com.example.xrapp;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import org.khronos.openxr.*;

public class XRMainActivity extends Activity {
    private static final String TAG = "XRApp";
    
    static {
        // 加载原生库
        System.loadLibrary("xrruntime");
    }
    
    private XrInstance instance;
    private XrSession session;
    private XrSystemId systemId;
    private GLSurfaceView glSurfaceView;
    private boolean xrInitialized = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 初始化 OpenXR
        if (initializeOpenXR()) {
            setupGLSurfaceView();
            xrInitialized = true;
        } else {
            Log.e(TAG, "Failed to initialize OpenXR");
            finish();
        }
    }
    
    private boolean initializeOpenXR() {
        try {
            // 1. 创建 Instance
            XrInstanceCreateInfo createInfo = new XrInstanceCreateInfo();
            createInfo.setApplicationInfo(
                "My XR App",           // applicationName
                1,                     // applicationVersion
                "My Engine",           // engineName
                1,                     // engineVersion
                XrVersion.XR_CURRENT_API_VERSION
            );
            
            // 添加必需的扩展
            createInfo.addEnabledExtension("XR_KHR_android_create_instance");
            createInfo.addEnabledExtension("XR_KHR_opengl_es_enable");
            
            // Android 特定配置
            XrInstanceCreateInfoAndroidKHR androidInfo = 
                new XrInstanceCreateInfoAndroidKHR();
            androidInfo.setApplicationVM(getApplicationInfo().nativeLibraryDir);
            androidInfo.setApplicationActivity(this);
            createInfo.setNext(androidInfo);
            
            instance = new XrInstance();
            int result = XR.xrCreateInstance(createInfo, instance);
            
            if (result != XrResult.XR_SUCCESS) {
                Log.e(TAG, "xrCreateInstance failed: " + result);
                return false;
            }
            
            Log.i(TAG, "OpenXR instance created");
            
            // 2. 获取 System
            XrSystemGetInfo systemInfo = new XrSystemGetInfo();
            systemInfo.setFormFactor(XrFormFactor.XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY);
            
            systemId = new XrSystemId();
            result = XR.xrGetSystem(instance, systemInfo, systemId);
            
            if (result != XrResult.XR_SUCCESS) {
                Log.e(TAG, "xrGetSystem failed: " + result);
                return false;
            }
            
            Log.i(TAG, "XR System found: " + systemId);
            
            // 3. 获取系统属性
            XrSystemProperties properties = new XrSystemProperties();
            result = XR.xrGetSystemProperties(instance, systemId, properties);
            
            if (result == XrResult.XR_SUCCESS) {
                Log.i(TAG, "System Name: " + properties.getSystemName());
                Log.i(TAG, "Vendor ID: " + properties.getVendorId());
            }
            
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "Error initializing OpenXR", e);
            return false;
        }
    }
    
    private void setupGLSurfaceView() {
        glSurfaceView = new GLSurfaceView(this);
        glSurfaceView.setEGLContextClientVersion(3);
        glSurfaceView.setRenderer(new XRRenderer());
        setContentView(glSurfaceView);
    }
    
    private void createSession() {
        if (session != null) {
            return; // Session already created
        }
        
        try {
            // 创建 OpenGL ES 上下文
            EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(display, null, null);
            
            // 配置 EGL
            int[] attribList = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_DEPTH_SIZE, 24,
                EGL_NONE
            };
            
            EGLConfig[] configs = new EGLConfig[1];
            int[] numConfigs = new int[1];
            eglChooseConfig(display, attribList, configs, 1, numConfigs);
            
            EGLContext context = eglCreateContext(display, configs[0], 
                EGL_NO_CONTEXT, new int[]{EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE});
            
            // 创建 Session
            XrSessionCreateInfo sessionInfo = new XrSessionCreateInfo();
            sessionInfo.setSystemId(systemId);
            
            // OpenGL ES 绑定
            XrGraphicsBindingOpenGLESAndroidKHR glBinding = 
                new XrGraphicsBindingOpenGLESAndroidKHR();
            glBinding.setDisplay(display);
            glBinding.setConfig(configs[0]);
            glBinding.setContext(context);
            sessionInfo.setNext(glBinding);
            
            session = new XrSession();
            int result = XR.xrCreateSession(instance, sessionInfo, session);
            
            if (result == XrResult.XR_SUCCESS) {
                Log.i(TAG, "XR Session created");
                
                // 开始 Session
                XrSessionBeginInfo beginInfo = new XrSessionBeginInfo();
                beginInfo.setPrimaryViewConfigurationType(
                    XrViewConfigurationType.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
                
                result = XR.xrBeginSession(session, beginInfo);
                if (result == XrResult.XR_SUCCESS) {
                    Log.i(TAG, "XR Session started");
                }
            } else {
                Log.e(TAG, "xrCreateSession failed: " + result);
            }
            
        } catch (Exception e) {
            Log.e(TAG, "Error creating session", e);
        }
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        if (xrInitialized && glSurfaceView != null) {
            glSurfaceView.onResume();
            createSession();
        }
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        if (glSurfaceView != null) {
            glSurfaceView.onPause();
        }
        if (session != null) {
            XR.xrEndSession(session);
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清理资源
        if (session != null) {
            XR.xrDestroySession(session);
            session = null;
        }
        if (instance != null) {
            XR.xrDestroyInstance(instance);
            instance = null;
        }
    }
    
    // OpenGL ES Renderer
    private class XRRenderer implements GLSurfaceView.Renderer {
        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            // OpenGL 初始化
        }
        
        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            // 表面大小改变
        }
        
        @Override
        public void onDrawFrame(GL10 gl) {
            if (session == null) {
                return;
            }
            
            // 渲染 XR 帧
            renderFrame();
        }
        
        private void renderFrame() {
            try {
                // 等待下一帧
                XrFrameWaitInfo waitInfo = new XrFrameWaitInfo();
                XrFrameState frameState = new XrFrameState();
                
                int result = XR.xrWaitFrame(session, waitInfo, frameState);
                if (result != XrResult.XR_SUCCESS) {
                    return;
                }
                
                // 开始帧
                XrFrameBeginInfo beginInfo = new XrFrameBeginInfo();
                result = XR.xrBeginFrame(session, beginInfo);
                if (result != XrResult.XR_SUCCESS) {
                    return;
                }
                
                // 获取视图
                XrViewLocateInfo locateInfo = new XrViewLocateInfo();
                locateInfo.setViewConfigurationType(
                    XrViewConfigurationType.XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
                locateInfo.setDisplayTime(frameState.getPredictedDisplayTime());
                locateInfo.setSpace(/* reference space */);
                
                XrViewState viewState = new XrViewState();
                XrView[] views = new XrView[2];
                int viewCount = 2;
                
                result = XR.xrLocateViews(session, locateInfo, viewState, views, viewCount);
                
                // 渲染左右眼视图
                // ...
                
                // 结束帧
                XrFrameEndInfo endInfo = new XrFrameEndInfo();
                endInfo.setDisplayTime(frameState.getPredictedDisplayTime());
                // endInfo.setLayers(...);
                
                XR.xrEndFrame(session, endInfo);
                
            } catch (Exception e) {
                Log.e(TAG, "Error rendering frame", e);
            }
        }
    }
}
```

---

## 简化示例：仅加载库

如果你只需要加载库并让系统自动发现，可以使用更简单的方式：

```java
package com.example.xrapp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class SimpleXRActivity extends Activity {
    private static final String TAG = "XRApp";
    
    static {
        try {
            // 加载 XR Runtime 库
            // 库会在加载时自动初始化（通过 JNI_OnLoad）
            System.loadLibrary("xrruntime");
            Log.i(TAG, "XR Runtime library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load XR Runtime library", e);
        }
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 库已经初始化，可以通过 OpenXR API 使用
        // 或者等待系统 OpenXR Loader 自动发现
        Log.i(TAG, "Activity created, XR Runtime is ready");
    }
}
```

---

## 检查库是否加载成功

### 方法 1: 查看 Logcat

```bash
adb logcat | grep -i "xrruntime"
```

应该看到：
```
I/XRRuntime: XRRuntime JNI_OnLoad called
I/XRRuntime: Initializing XR Runtime for Qualcomm XR2
I/XRRuntime: XR Runtime initialized successfully
```

### 方法 2: 在代码中检查

```java
public class XRUtils {
    public static boolean isXRRuntimeLoaded() {
        try {
            System.loadLibrary("xrruntime");
            return true;
        } catch (UnsatisfiedLinkError e) {
            return false;
        }
    }
}
```

---

## 使用部署脚本

### Windows PowerShell

```powershell
# 推送到系统分区
.\scripts\deploy_to_system.ps1

# 推送到厂商分区
.\scripts\deploy_to_system.ps1 -Vendor

# 查看帮助
.\scripts\deploy_to_system.ps1 -Help
```

### Linux/macOS Bash

```bash
# 推送到系统分区
chmod +x scripts/deploy_to_system.sh
./scripts/deploy_to_system.sh

# 推送到厂商分区
./scripts/deploy_to_system.sh --vendor
```

---

## 相关文档

- [使用指南](USAGE_GUIDE.md) - 详细的使用说明
- [系统集成指南](SYSTEM_INTEGRATION.md) - 系统分区集成方法
- [编译问题解决方案](COMPILE_WORKAROUND.md) - 编译相关问题

