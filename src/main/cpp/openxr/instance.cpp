#include "openxr_api.h"
#include "platform/android_platform.h"
#include "qualcomm/xr2_platform.h"
#include "utils/logger.h"
#include "utils/error_handler.h"
#include <cstring>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>

// External runtime initialization
extern std::atomic<bool> g_runtimeInitialized;

struct XRInstance {
    XrInstanceProperties properties;
    XrSystemId systemId;
    bool initialized;
    std::mutex mutex;
    
    XRInstance() : initialized(false) {
        memset(&properties, 0, sizeof(properties));
        properties.type = XR_TYPE_INSTANCE_PROPERTIES;
        strncpy(properties.runtimeName, "Custom XR2 Runtime", XR_MAX_RUNTIME_NAME_SIZE - 1);
        properties.runtimeVersion = XR_MAKE_VERSION(1, 1, 0);
    }
};

std::mutex g_instanceMutex;
std::unordered_map<XrInstance, std::shared_ptr<XRInstance>> g_instances;
static XrInstance g_nextInstanceHandle = reinterpret_cast<XrInstance>(1);

XrResult xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) {
    if (!createInfo || !instance) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    LOGI("xrCreateInstance called");
    
    // Check if runtime is initialized
    if (!g_runtimeInitialized.load()) {
        if (!InitializeXRRuntime()) {
            LOGE("Failed to initialize XR Runtime");
            return XR_ERROR_RUNTIME_FAILURE;
        }
    }
    
    // Create instance
    auto xrInstance = std::make_shared<XRInstance>();
    
    // Initialize platform
    if (!InitializeAndroidPlatform()) {
        LOGE("Failed to initialize Android platform");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    if (!InitializeXR2Platform()) {
        LOGE("Failed to initialize XR2 platform");
        ShutdownAndroidPlatform();
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    xrInstance->initialized = true;
    
    // Register instance
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    // Increment handle using integer arithmetic (handles are pointers in 64-bit)
    uintptr_t handleValue = reinterpret_cast<uintptr_t>(g_nextInstanceHandle);
    handleValue++;
    XrInstance handle = reinterpret_cast<XrInstance>(handleValue);
    g_nextInstanceHandle = handle;
    g_instances[handle] = xrInstance;
    *instance = handle;
    
    LOGI("Instance created: %p", handle);
    return XR_SUCCESS;
}

XrResult xrDestroyInstance(XrInstance instance) {
    if (!instance) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    LOGI("xrDestroyInstance called for instance: %p", instance);
    
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Cleanup platform resources
    ShutdownXR2Platform();
    ShutdownAndroidPlatform();
    
    g_instances.erase(it);
    
    LOGI("Instance destroyed");
    return XR_SUCCESS;
}

XrResult xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) {
    if (!instanceProperties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    *instanceProperties = it->second->properties;
    return XR_SUCCESS;
}

XrResult xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) {
    if (!getInfo || !systemId) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (getInfo->type != XR_TYPE_SYSTEM_GET_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Check if system is supported
    XrFormFactor formFactor = getInfo->formFactor;
    if (formFactor != XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) {
        return XR_ERROR_FORM_FACTOR_UNSUPPORTED;
    }
    
    // Create system ID (simple implementation)
    if (it->second->systemId == XR_NULL_SYSTEM_ID) {
        it->second->systemId = 1; // Simple system ID
    }
    
    *systemId = it->second->systemId;
    
    LOGI("System ID: %llu", *systemId);
    return XR_SUCCESS;
}

XrResult xrGetSystemProperties(XrInstance instance, XrSystemId systemId, XrSystemProperties* properties) {
    if (!properties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (properties->type != XR_TYPE_SYSTEM_PROPERTIES) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    auto it = g_instances.find(instance);
    if (it == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    if (it->second->systemId != systemId) {
        return XR_ERROR_SYSTEM_INVALID;
    }
    
    // Fill system properties
    properties->vendorId = 0x5143; // Qualcomm vendor ID
    properties->systemId = systemId;
    strncpy(properties->systemName, "Qualcomm XR2", XR_MAX_SYSTEM_NAME_SIZE - 1);
    
    // Get graphics properties from platform
    XrGraphicsPropertiesOpenGLESKHR xr2GraphicsProps = {};
    if (!GetXR2GraphicsProperties(&xr2GraphicsProps)) {
        return XR_ERROR_RUNTIME_FAILURE;
    }
    // Copy to standard OpenXR structure
    properties->graphicsProperties.maxSwapchainImageWidth = xr2GraphicsProps.maxSwapchainImageWidth;
    properties->graphicsProperties.maxSwapchainImageHeight = xr2GraphicsProps.maxSwapchainImageHeight;
    properties->graphicsProperties.maxLayerCount = xr2GraphicsProps.maxSwapchainImageLayers;
    
    // Get tracking properties
    if (!GetXR2TrackingProperties(&properties->trackingProperties)) {
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    return XR_SUCCESS;
}

