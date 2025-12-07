#include "android_platform.h"
#include "utils/logger.h"
#include <mutex>

static JavaVM* g_javaVM = nullptr;
static ANativeWindow* g_nativeWindow = nullptr;
static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static std::mutex g_platformMutex;

bool InitializeAndroidPlatform() {
    std::lock_guard<std::mutex> lock(g_platformMutex);
    
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        LOGW("Android platform already initialized");
        return true;
    }
    
    LOGI("Initializing Android platform");
    
    // Initialize EGL
    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }
    
    EGLint major, minor;
    if (!eglInitialize(g_eglDisplay, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        return false;
    }
    
    LOGI("EGL initialized: %d.%d", major, minor);
    
    // Create EGL context (will be created when window is available)
    
    return true;
}

void ShutdownAndroidPlatform() {
    std::lock_guard<std::mutex> lock(g_platformMutex);
    
    if (g_eglDisplay == EGL_NO_DISPLAY) {
        return;
    }
    
    LOGI("Shutting down Android platform");
    
    // Destroy EGL surface
    if (g_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(g_eglDisplay, g_eglSurface);
        g_eglSurface = EGL_NO_SURFACE;
    }
    
    // Destroy EGL context
    if (g_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(g_eglDisplay, g_eglContext);
        g_eglContext = EGL_NO_CONTEXT;
    }
    
    // Terminate EGL
    eglTerminate(g_eglDisplay);
    g_eglDisplay = EGL_NO_DISPLAY;
    
    g_nativeWindow = nullptr;
}

JavaVM* GetJavaVM() {
    return g_javaVM;
}

void SetJavaVM(JavaVM* vm) {
    std::lock_guard<std::mutex> lock(g_platformMutex);
    g_javaVM = vm;
}

JNIEnv* GetJNIEnv() {
    std::lock_guard<std::mutex> lock(g_platformMutex);
    if (!g_javaVM) {
        return nullptr;
    }
    
    JNIEnv* env = nullptr;
    jint result = g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    
    if (result == JNI_OK) {
        return env;
    } else if (result == JNI_EDETACHED) {
        // Attach current thread
        if (g_javaVM->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            return env;
        }
    }
    
    return nullptr;
}

ANativeWindow* GetNativeWindow() {
    std::lock_guard<std::mutex> lock(g_platformMutex);
    return g_nativeWindow;
}

void SetNativeWindow(ANativeWindow* window) {
    std::lock_guard<std::mutex> lock(g_platformMutex);
    g_nativeWindow = window;
    
    if (window && g_eglDisplay != EGL_NO_DISPLAY) {
        // Create EGL surface
        EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE
        };
        
        EGLConfig config;
        EGLint numConfigs;
        if (eglChooseConfig(g_eglDisplay, attribs, &config, 1, &numConfigs)) {
            g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, window, nullptr);
            if (g_eglSurface == EGL_NO_SURFACE) {
                LOGE("Failed to create EGL surface");
            }
        }
    }
}

EGLDisplay GetEGLDisplay() {
    return g_eglDisplay;
}

EGLContext GetEGLContext() {
    return g_eglContext;
}

EGLSurface GetEGLSurface() {
    return g_eglSurface;
}

