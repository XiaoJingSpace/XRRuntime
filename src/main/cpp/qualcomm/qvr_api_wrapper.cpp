#include "qvr_api_wrapper.h"
#include "utils/logger.h"
#include <mutex>
#include <cstring>
#include <cstdlib>

// QVR API state
static QVRServiceClientHandle g_qvrClient = nullptr;
static bool g_qvrInitialized = false;
static std::mutex g_qvrMutex;
static uint64_t g_qvrAndroidOffsetNs = 0; // Offset between QTimer and Android time

bool InitializeQVRAPI() {
    std::lock_guard<std::mutex> lock(g_qvrMutex);
    
    if (g_qvrInitialized) {
        LOGW("QVR API already initialized");
        return true;
    }
    
    LOGI("Initializing QVR API");
    
    // Create QVR Service Client
    g_qvrClient = QVRServiceClient_CreateWrapper();
    if (!g_qvrClient) {
        LOGE("Failed to create QVR Service Client");
        return false;
    }
    
    // Get VR Mode state
    QVRSERVICE_VRMODE_STATE vrMode = QVRServiceClient_GetVRModeWrapper(g_qvrClient);
    if (vrMode == VRMODE_UNSUPPORTED) {
        LOGE("VR Mode is not supported on this device");
        QVRServiceClient_DestroyWrapper(g_qvrClient);
        g_qvrClient = nullptr;
        return false;
    }
    
    LOGI("VR Mode state: %d", vrMode);
    
    // Get tracker-android offset if available
    // This is used for time conversion between QTimer and Android time domains
    char offsetStr[64] = {0};
    uint32_t offsetLen = sizeof(offsetStr);
    int result = QVRServiceClient_GetParamWrapper(g_qvrClient, 
                                                   "tracker-android-offset-ns", 
                                                   &offsetLen, 
                                                   offsetStr);
    if (result == QVR_SUCCESS && offsetLen > 0) {
        // Parse offset string to uint64_t
        g_qvrAndroidOffsetNs = static_cast<uint64_t>(std::atoll(offsetStr));
        LOGI("QVR tracker-android offset: %llu ns", g_qvrAndroidOffsetNs);
    } else {
        // Try alternative parameter name
        offsetLen = sizeof(offsetStr);
        result = QVRServiceClient_GetParamWrapper(g_qvrClient,
                                                   "QVRSERVICE_TRACKER_ANDROID_OFFSET_NS",
                                                   &offsetLen,
                                                   offsetStr);
        if (result == QVR_SUCCESS && offsetLen > 0) {
            g_qvrAndroidOffsetNs = static_cast<uint64_t>(std::atoll(offsetStr));
            LOGI("QVR tracker-android offset (alt): %llu ns", g_qvrAndroidOffsetNs);
        } else {
            LOGW("Failed to get tracker-android offset, using 0");
            g_qvrAndroidOffsetNs = 0;
        }
    }
    
    g_qvrInitialized = true;
    LOGI("QVR API initialized successfully");
    return true;
}

void ShutdownQVRAPI() {
    std::lock_guard<std::mutex> lock(g_qvrMutex);
    
    if (!g_qvrInitialized) {
        return;
    }
    
    LOGI("Shutting down QVR API");
    
    if (g_qvrClient) {
        // Stop VR Mode if still active
        QVRSERVICE_VRMODE_STATE vrMode = QVRServiceClient_GetVRModeWrapper(g_qvrClient);
        if (vrMode == VRMODE_STARTED || vrMode == VRMODE_STARTING) {
            QVRServiceClient_StopVRModeWrapper(g_qvrClient);
        }
        
        QVRServiceClient_DestroyWrapper(g_qvrClient);
        g_qvrClient = nullptr;
    }
    
    g_qvrInitialized = false;
    LOGI("QVR API shut down");
}

QVRServiceClientHandle QVRServiceClient_CreateWrapper() {
    return QVRServiceClient_Create();
}

void QVRServiceClient_DestroyWrapper(QVRServiceClientHandle handle) {
    if (handle) {
        QVRServiceClient_Destroy(handle);
    }
}

QVRSERVICE_VRMODE_STATE QVRServiceClient_GetVRModeWrapper(QVRServiceClientHandle handle) {
    if (!handle) {
        return VRMODE_UNSUPPORTED;
    }
    return QVRServiceClient_GetVRMode(handle);
}

int QVRServiceClient_StartVRModeWrapper(QVRServiceClientHandle handle) {
    if (!handle) {
        return QVR_INVALID_PARAM;
    }
    
    // Check current state
    QVRSERVICE_VRMODE_STATE state = QVRServiceClient_GetVRMode(handle);
    if (state != VRMODE_STOPPED) {
        LOGE("Cannot start VR Mode, current state: %d", state);
        return QVR_ERROR;
    }
    
    return QVRServiceClient_StartVRMode(handle);
}

int QVRServiceClient_StopVRModeWrapper(QVRServiceClientHandle handle) {
    if (!handle) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_StopVRMode(handle);
}

int QVRServiceClient_GetTrackingModeWrapper(QVRServiceClientHandle handle, 
                                            QVRSERVICE_TRACKING_MODE* mode, 
                                            uint32_t* supportedModes) {
    if (!handle) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_GetTrackingMode(handle, mode, supportedModes);
}

int QVRServiceClient_SetTrackingModeWrapper(QVRServiceClientHandle handle, 
                                            QVRSERVICE_TRACKING_MODE mode) {
    if (!handle) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_SetTrackingMode(handle, mode);
}

int QVRServiceClient_GetHeadTrackingDataWrapper(QVRServiceClientHandle handle, 
                                                 qvrservice_head_tracking_data_t** data) {
    if (!handle || !data) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_GetHeadTrackingData(handle, data);
}

int QVRServiceClient_SetDisplayInterruptConfigWrapper(QVRServiceClientHandle handle, 
                                                       QVRSERVICE_DISP_INTERRUPT_ID interruptId, 
                                                       void* config, uint32_t configSize) {
    if (!handle) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_SetDisplayInterruptConfig(handle, interruptId, config, configSize);
}

int QVRServiceClient_GetDisplayInterruptTimestampWrapper(QVRServiceClientHandle handle,
                                                          QVRSERVICE_DISP_INTERRUPT_ID interruptId,
                                                          qvrservice_ts_t** ts) {
    if (!handle) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_GetDisplayInterruptTimestamp(handle, interruptId, ts);
}

int QVRServiceClient_GetParamWrapper(QVRServiceClientHandle handle, const char* name, 
                                     uint32_t* len, char* value) {
    if (!handle || !name || !len) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_GetParam(handle, name, len, value);
}

int QVRServiceClient_SetParamWrapper(QVRServiceClientHandle handle, const char* name, 
                                     const char* value) {
    if (!handle || !name || !value) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_SetParam(handle, name, value);
}

int QVRServiceClient_GetHwTransformsWrapper(QVRServiceClientHandle handle, 
                                            uint32_t* numTransforms, 
                                            qvrservice_hw_transform_t* transforms) {
    if (!handle || !numTransforms) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_GetHwTransforms(handle, numTransforms, transforms);
}

int QVRServiceClient_SetOperatingLevelWrapper(QVRServiceClientHandle handle,
                                               qvrservice_perf_level_t* perfLevels,
                                               uint32_t numPerfLevels) {
    if (!handle || !perfLevels || numPerfLevels == 0) {
        return QVR_INVALID_PARAM;
    }
    return QVRServiceClient_SetOperatingLevel(handle, perfLevels, numPerfLevels, nullptr, nullptr);
}

XrTime QVRTimeToXrTime(uint64_t qvrTime) {
    // QVR uses QTimer time domain, OpenXR uses system time
    // Add offset if available
    return static_cast<XrTime>(qvrTime + g_qvrAndroidOffsetNs);
}

uint64_t XrTimeToQVRTime(XrTime xrTime) {
    // Convert OpenXR time to QVR time
    return static_cast<uint64_t>(xrTime) - g_qvrAndroidOffsetNs;
}

void QVRPoseToXrPose(const qvrservice_head_tracking_data_t* qvrData, XrPosef* xrPose) {
    if (!qvrData || !xrPose) {
        return;
    }
    
    // QVR uses Android Portrait coordinate system
    // OpenXR uses right-handed coordinate system
    // Rotation: QVR quaternion is (x, y, z, w), OpenXR is also (x, y, z, w)
    xrPose->orientation.x = qvrData->rotation[0];
    xrPose->orientation.y = qvrData->rotation[1];
    xrPose->orientation.z = qvrData->rotation[2];
    xrPose->orientation.w = qvrData->rotation[3];
    
    // Position: Direct mapping (may need coordinate system conversion)
    xrPose->position.x = qvrData->translation[0];
    xrPose->position.y = qvrData->translation[1];
    xrPose->position.z = qvrData->translation[2];
}

// Get QVR client handle (for use in other modules)
QVRServiceClientHandle GetQVRClient() {
    std::lock_guard<std::mutex> lock(g_qvrMutex);
    return g_qvrClient;
}
