#include <jni.h>
#include <android/log.h>
#include "jni/jni_bridge.h"
#include "openxr/openxr_api.h"

#define LOG_TAG "XRRuntime"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "platform/android_platform.h"

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("XRRuntime JNI_OnLoad called");
    
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get JNI environment");
        return JNI_ERR;
    }
    
    // Set JavaVM for platform layer
    SetJavaVM(vm);
    
    // Initialize OpenXR runtime
    if (!InitializeXRRuntime()) {
        LOGE("Failed to initialize XR Runtime");
        return JNI_ERR;
    }
    
    LOGI("XRRuntime initialized successfully");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("XRRuntime JNI_OnUnload called");
    ShutdownXRRuntime();
}

} // extern "C"

