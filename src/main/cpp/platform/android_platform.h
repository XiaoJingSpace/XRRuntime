#ifndef ANDROID_PLATFORM_H
#define ANDROID_PLATFORM_H

#include <jni.h>
#include <android/native_window.h>
#include <EGL/egl.h>

// Android platform initialization
bool InitializeAndroidPlatform();
void ShutdownAndroidPlatform();

// JNI environment
JavaVM* GetJavaVM();
void SetJavaVM(JavaVM* vm);
JNIEnv* GetJNIEnv();

// Native window management
ANativeWindow* GetNativeWindow();
void SetNativeWindow(ANativeWindow* window);

// EGL context management
EGLDisplay GetEGLDisplay();
EGLContext GetEGLContext();
EGLSurface GetEGLSurface();

#endif // ANDROID_PLATFORM_H

