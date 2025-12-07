#include "jni_bridge.h"
#include "openxr/openxr_api.h"
#include "platform/android_platform.h"
#include "utils/logger.h"

bool InitializeXRRuntime() {
    return ::InitializeXRRuntime();
}

void ShutdownXRRuntime() {
    ::ShutdownXRRuntime();
}

