#include "jni_bridge.h"
#include "openxr/openxr_api.h"
#include "platform/android_platform.h"
#include "utils/logger.h"

// Note: InitializeXRRuntime() and ShutdownXRRuntime() are defined in openxr_api.cpp
// These functions are declared in jni_bridge.h for JNI interface compatibility
// but the actual implementation is in openxr_api.cpp to avoid duplicate symbols

