#include "openxr_api.h"
#include "utils/logger.h"
#include "utils/error_handler.h"
#include "platform/android_platform.h"
#include "platform/display_manager.h"
#include "qualcomm/xr2_platform.h"
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <cstring>
#include <string>

// Forward declarations
struct XRInstance;
struct XRSession;
struct XRSpace;
struct XRSwapchain;
struct XRActionSet;
struct XRAction;

// Runtime initialization state
std::atomic<bool> g_runtimeInitialized(false);

// External declarations from other modules
extern std::mutex g_instanceMutex;
extern std::unordered_map<XrInstance, std::shared_ptr<XRInstance>> g_instances;

extern std::mutex g_sessionMutex;
extern std::unordered_map<XrSession, std::shared_ptr<XRSession>> g_sessions;

extern std::mutex g_spaceMutex;
extern std::unordered_map<XrSpace, std::shared_ptr<XRSpace>> g_spaces;

extern std::mutex g_swapchainMutex;
extern std::unordered_map<XrSwapchain, std::shared_ptr<XRSwapchain>> g_swapchains;

extern std::mutex g_actionSetMutex;
extern std::unordered_map<XrActionSet, std::shared_ptr<XRActionSet>> g_actionSets;

extern std::mutex g_actionMutex;
extern std::unordered_map<XrAction, std::shared_ptr<XRAction>> g_actions;

bool InitializeXRRuntime() {
    if (g_runtimeInitialized.exchange(true)) {
        LOGW("XR Runtime already initialized");
        return true;
    }
    
    LOGI("Initializing XR Runtime for Qualcomm XR2");
    
    // Initialize platform layer
    if (!InitializeAndroidPlatform()) {
        LOGE("Failed to initialize Android platform");
        return false;
    }
    
    // Initialize Qualcomm XR2 platform
    if (!InitializeXR2Platform()) {
        LOGE("Failed to initialize XR2 platform");
        ShutdownAndroidPlatform();
        return false;
    }
    
    LOGI("XR Runtime initialized successfully");
    return true;
}

void ShutdownXRRuntime() {
    if (!g_runtimeInitialized.exchange(false)) {
        return;
    }
    
    LOGI("Shutting down XR Runtime");
    
    // Destroy all remaining resources
    {
        std::lock_guard<std::mutex> lock(g_actionMutex);
        g_actions.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(g_actionSetMutex);
        g_actionSets.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(g_swapchainMutex);
        g_swapchains.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(g_spaceMutex);
        g_spaces.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(g_sessionMutex);
        g_sessions.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        g_instances.clear();
    }
    
    ShutdownXR2Platform();
    ShutdownAndroidPlatform();
    
    LOGI("XR Runtime shutdown complete");
}

// Additional OpenXR API implementations

XrResult xrStringToPath(XrInstance instance, const char* pathString, XrPath* path) {
    if (!pathString || !path) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Simple path string to path conversion
    // In a real implementation, this would maintain a path registry
    static std::mutex pathMutex;
    static std::unordered_map<std::string, XrPath> pathMap;
    static std::unordered_map<XrPath, std::string> pathToStringMap; // Reverse mapping
    static XrPath nextPath = 1;
    
    std::lock_guard<std::mutex> lock(pathMutex);
    auto it = pathMap.find(pathString);
    if (it != pathMap.end()) {
        *path = it->second;
    } else {
        *path = nextPath++;
        pathMap[pathString] = *path;
        // Also register reverse mapping for path to string conversion
        pathToStringMap[*path] = pathString;
    }
    
    return XR_SUCCESS;
}

XrResult xrPathToString(XrInstance instance, XrPath path, uint32_t bufferCapacityInput, 
                        uint32_t* bufferCountOutput, char* buffer) {
    if (!bufferCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Simple path to string conversion
    // Use the same registry as xrStringToPath
    static std::mutex pathMutex;
    static std::unordered_map<XrPath, std::string> pathToStringMap;
    
    std::lock_guard<std::mutex> lock(pathMutex);
    auto it = pathToStringMap.find(path);
    if (it == pathToStringMap.end()) {
        return XR_ERROR_PATH_INVALID;
    }
    
    const std::string& pathString = it->second;
    uint32_t requiredSize = static_cast<uint32_t>(pathString.length() + 1);
    
    *bufferCountOutput = requiredSize;
    
    if (buffer && bufferCapacityInput >= requiredSize) {
        strncpy(buffer, pathString.c_str(), bufferCapacityInput - 1);
        buffer[bufferCapacityInput - 1] = '\0';
    }
    
    return XR_SUCCESS;
}

XrResult xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId, 
                                      uint32_t viewConfigurationTypeCapacityInput, 
                                      uint32_t* viewConfigurationTypeCountOutput, 
                                      XrViewConfigurationType* viewConfigurationTypes) {
    if (!viewConfigurationTypeCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // XR2 supports primary stereo view configuration
    uint32_t count = 1;
    *viewConfigurationTypeCountOutput = count;
    
    if (viewConfigurationTypes && viewConfigurationTypeCapacityInput >= count) {
        viewConfigurationTypes[0] = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    }
    
    return XR_SUCCESS;
}

XrResult xrGetViewConfigurationProperties(XrInstance instance, XrSystemId systemId, 
                                          XrViewConfigurationType viewConfigurationType, 
                                          XrViewConfigurationProperties* configurationProperties) {
    if (!configurationProperties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (configurationProperties->type != XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
        return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
    }
    
    configurationProperties->viewConfigurationType = viewConfigurationType;
    configurationProperties->fovMutable = XR_FALSE;
    
    return XR_SUCCESS;
}

XrResult xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId, 
                                            XrViewConfigurationType viewConfigurationType, 
                                            uint32_t viewCapacityInput, uint32_t* viewCountOutput, 
                                            XrViewConfigurationView* views) {
    if (!viewCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
        return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
    }
    
    // Primary stereo has 2 views
    uint32_t count = 2;
    *viewCountOutput = count;
    
    if (views && viewCapacityInput >= count) {
        // Get view properties from platform
        if (!GetXR2ViewConfigurationViews(views, count)) {
            return XR_ERROR_RUNTIME_FAILURE;
        }
    }
    
    return XR_SUCCESS;
}

XrResult xrGetReferenceSpaceBoundsRect(XrSession session, XrReferenceSpaceType referenceSpaceType, 
                                       XrExtent2Df* bounds) {
    if (!bounds) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get bounds from platform
    if (referenceSpaceType == XR_REFERENCE_SPACE_TYPE_STAGE) {
        if (!GetXR2StageBounds(bounds)) {
            // XR_SPACE_BOUNDS_UNAVAILABLE is a state value, not an error code
            // Return success with zero bounds
            bounds->width = 0.0f;
            bounds->height = 0.0f;
            return XR_SUCCESS;
        }
    } else {
        // Other space types don't have bounds
        // Note: XR_SPACE_BOUNDS_UNAVAILABLE is a state value, not an error code
        // We should return XR_SUCCESS with bounds set to zero or return an appropriate error
        bounds->width = 0.0f;
        bounds->height = 0.0f;
        return XR_SUCCESS;
    }
    
    return XR_SUCCESS;
}
