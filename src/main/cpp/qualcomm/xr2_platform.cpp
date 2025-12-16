#include "xr2_platform.h"
#include "qvr_api_wrapper.h"
#include "spaces_sdk_wrapper.h"
#include "platform/input_manager.h"
#include "utils/logger.h"
#include <mutex>
#include <chrono>
#include <cstring>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <openxr/openxr.h>

// Platform state
static bool g_xr2Initialized = false;
static bool g_displayInitialized = false;
static bool g_trackingInitialized = false;
static bool g_handTrackingInitialized = false; // Separate flag for hand tracking
static bool g_renderingActive = false;
static std::mutex g_xr2Mutex;

// Prediction coefficients for pose prediction
static float g_predictionCoeffS[3] = {0.0f, 0.0f, 0.0f}; // Velocity coefficients
static float g_predictionCoeffB[3] = {0.0f, 0.0f, 0.0f}; // Acceleration coefficients
static float g_predictionCoeffBdt[3] = {0.0f, 0.0f, 0.0f}; // Higher order terms

// Display properties (XR2 typical values)
static const uint32_t XR2_RECOMMENDED_WIDTH = 1832;
static const uint32_t XR2_RECOMMENDED_HEIGHT = 1920;
static const uint32_t XR2_MAX_WIDTH = 1832;
static const uint32_t XR2_MAX_HEIGHT = 1920;
static const uint32_t XR2_REFRESH_RATE = 90; // Hz

// Performance monitoring
static uint64_t g_frameCount = 0;
static XrTime g_lastFrameTime = 0;
static float g_currentFPS = 0.0f;
static float g_averageFrameTime = 0.0f;
static uint32_t g_droppedFrames = 0;
static const uint32_t FPS_SAMPLE_COUNT = 60;

// Power management
static bool g_powerOptimizationEnabled = false;
static uint32_t g_currentPerfLevel = 0; // 0 = balanced, higher = more performance
static const uint32_t XR2_MAX_PERF_LEVEL = 3;

bool InitializeXR2Platform() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (g_xr2Initialized) {
        LOGW("XR2 platform already initialized");
        return true;
    }
    
    LOGI("Initializing Qualcomm XR2 platform");
    
    // Initialize QVR API
    if (!InitializeQVRAPI()) {
        LOGE("Failed to initialize QVR API");
        return false;
    }
    
    // Initialize Snapdragon Spaces SDK (for hand tracking and scene understanding)
    // Note: This is optional and will be initialized when hand tracking is requested
    // InitializeSpacesSDK();
    
    g_xr2Initialized = true;
    
    LOGI("XR2 platform initialized");
    return true;
}

void ShutdownXR2Platform() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_xr2Initialized) {
        return;
    }
    
    LOGI("Shutting down XR2 platform");
    
    if (g_renderingActive) {
        StopXR2Rendering();
    }
    
    if (g_trackingInitialized) {
        ShutdownXR2Tracking();
    }
    
    if (g_displayInitialized) {
        ShutdownXR2Display();
    }
    
    // Shutdown QVR API
    ShutdownQVRAPI();
    
    // Shutdown Snapdragon Spaces SDK if initialized
    if (g_handTrackingInitialized) {
        ShutdownXR2HandTracking();
    }
    ShutdownSpacesSDK();
    
    g_xr2Initialized = false;
    
    LOGI("XR2 platform shut down");
}

bool InitializeXR2Display() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (g_displayInitialized) {
        return true;
    }
    
    LOGI("Initializing XR2 display");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        LOGE("QVR client not available");
        return false;
    }
    
    // Configure VSYNC interrupt for frame synchronization
    qvrservice_vsync_interrupt_config_t vsyncConfig = {};
    vsyncConfig.cb = nullptr; // We'll use polling instead of callback
    vsyncConfig.ctx = nullptr;
    
    int result = QVRServiceClient_SetDisplayInterruptConfigWrapper(
        qvrClient, DISP_INTERRUPT_VSYNC, &vsyncConfig, sizeof(vsyncConfig));
    
    if (result != QVR_SUCCESS && result != QVR_CALLBACK_NOT_SUPPORTED) {
        LOGW("Failed to configure VSYNC interrupt: %d", result);
        // Continue anyway, we can use polling
    }
    
    // Verify display is working
    qvrservice_ts_t* ts = nullptr;
    result = QVRServiceClient_GetDisplayInterruptTimestampWrapper(qvrClient, DISP_INTERRUPT_VSYNC, &ts);
    if (result != QVR_SUCCESS) {
        LOGW("Warning: Cannot get display interrupt timestamp, display may not be working properly");
        // Continue anyway - we'll use fallback timing
    }
    
    g_displayInitialized = true;
    
    LOGI("XR2 display initialized");
    return true;
}

void ShutdownXR2Display() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_displayInitialized) {
        return;
    }
    
    LOGI("Shutting down XR2 display");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (qvrClient) {
        // Clear VSYNC interrupt config
        qvrservice_vsync_interrupt_config_t vsyncConfig = {};
        vsyncConfig.cb = nullptr;
        vsyncConfig.ctx = nullptr;
        int result = QVRServiceClient_SetDisplayInterruptConfigWrapper(
            qvrClient, DISP_INTERRUPT_VSYNC, &vsyncConfig, sizeof(vsyncConfig));
        if (result != QVR_SUCCESS) {
            LOGW("Failed to clear VSYNC interrupt config: %d", result);
        }
    }
    
    // Reset display state
    g_displayInitialized = false;
    g_frameCount = 0;
    g_lastFrameTime = 0;
    g_currentFPS = 0.0f;
    g_averageFrameTime = 0.0f;
    g_droppedFrames = 0;
    
    LOGI("XR2 display shut down");
}

bool GetXR2DisplayProperties(uint32_t* recommendedWidth, uint32_t* recommendedHeight,
                             uint32_t* maxWidth, uint32_t* maxHeight) {
    if (!recommendedWidth || !recommendedHeight || !maxWidth || !maxHeight) {
        return false;
    }
    
    *recommendedWidth = XR2_RECOMMENDED_WIDTH;
    *recommendedHeight = XR2_RECOMMENDED_HEIGHT;
    *maxWidth = XR2_MAX_WIDTH;
    *maxHeight = XR2_MAX_HEIGHT;
    
    return true;
}

bool GetXR2GraphicsProperties(XrGraphicsPropertiesOpenGLESKHR* properties) {
    if (!properties) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        // Fallback to default values
        properties->maxSwapchainImageWidth = XR2_MAX_WIDTH;
        properties->maxSwapchainImageHeight = XR2_MAX_HEIGHT;
        properties->maxSwapchainImageLayers = 1;
        return true;
    }
    
    // Query actual graphics properties from QVR
    // Try to get display resolution from QVR params
    char value[256] = {0};
    uint32_t len = sizeof(value);
    
    // Query max swapchain dimensions (if available)
    int result = QVRServiceClient_GetParamWrapper(qvrClient, "max-swapchain-width", &len, value);
    if (result == QVR_SUCCESS && len > 0) {
        properties->maxSwapchainImageWidth = static_cast<uint32_t>(atoi(value));
    } else {
        properties->maxSwapchainImageWidth = XR2_MAX_WIDTH;
    }
    
    len = sizeof(value);
    result = QVRServiceClient_GetParamWrapper(qvrClient, "max-swapchain-height", &len, value);
    if (result == QVR_SUCCESS && len > 0) {
        properties->maxSwapchainImageHeight = static_cast<uint32_t>(atoi(value));
    } else {
        properties->maxSwapchainImageHeight = XR2_MAX_HEIGHT;
    }
    
    // Max layers supported (typically 1 for XR2)
    properties->maxSwapchainImageLayers = 1;
    
    LOGI("Graphics properties: maxWidth=%u, maxHeight=%u, maxLayers=%u",
         properties->maxSwapchainImageWidth, 
         properties->maxSwapchainImageHeight,
         properties->maxSwapchainImageLayers);
    
    return true;
}

bool GetXR2TrackingProperties(XrSystemTrackingProperties* properties) {
    if (!properties) {
        return false;
    }
    
    // XR2 supports 6DOF head tracking
    properties->orientationTracking = XR_TRUE;
    properties->positionTracking = XR_TRUE;
    
    return true;
}

bool StartXR2Rendering() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (g_renderingActive) {
        return true;
    }
    
    LOGI("Starting XR2 rendering");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        LOGE("QVR client not available");
        return false;
    }
    
    // Set tracking mode to positional (6DOF)
    int result = QVRServiceClient_SetTrackingModeWrapper(qvrClient, TRACKING_MODE_POSITIONAL);
    if (result != QVR_SUCCESS) {
        LOGW("Failed to set tracking mode to positional, trying rotational: %d", result);
        // Fallback to rotational tracking
        result = QVRServiceClient_SetTrackingModeWrapper(qvrClient, TRACKING_MODE_ROTATIONAL);
        if (result != QVR_SUCCESS) {
            LOGE("Failed to set tracking mode: %d", result);
            return false;
        }
    }
    
    // Start VR Mode
    result = QVRServiceClient_StartVRModeWrapper(qvrClient);
    if (result != QVR_SUCCESS) {
        LOGE("Failed to start VR Mode: %d", result);
        return false;
    }
    
    g_renderingActive = true;
    
    LOGI("XR2 rendering started");
    return true;
}

void StopXR2Rendering() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_renderingActive) {
        return;
    }
    
    LOGI("Stopping XR2 rendering");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (qvrClient) {
        int result = QVRServiceClient_StopVRModeWrapper(qvrClient);
        if (result != QVR_SUCCESS) {
            LOGW("Failed to stop VR Mode: %d", result);
        }
    }
    
    g_renderingActive = false;
    
    LOGI("XR2 rendering stopped");
}

// Tracking state
static float g_trackingQuality = 0.0f;
static uint16_t g_trackingState = 0;
static uint16_t g_trackingWarningFlags = 0;
static bool g_relocationInProgress = false;

bool InitializeXR2Tracking() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (g_trackingInitialized) {
        return true;
    }
    
    LOGI("Initializing XR2 tracking (SLAM system)");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        LOGE("QVR client not available");
        return false;
    }
    
    // Check tracking mode support
    QVRSERVICE_TRACKING_MODE currentMode;
    uint32_t supportedModes;
    int result = QVRServiceClient_GetTrackingModeWrapper(qvrClient, &currentMode, &supportedModes);
    if (result != QVR_SUCCESS) {
        LOGE("Failed to get tracking mode: %d", result);
        return false;
    }
    
    LOGI("Current tracking mode: %u, Supported modes: 0x%x", currentMode, supportedModes);
    
    // Prefer positional tracking (6DOF SLAM) if available
    if (supportedModes & TRACKING_MODE_POSITIONAL) {
        result = QVRServiceClient_SetTrackingModeWrapper(qvrClient, TRACKING_MODE_POSITIONAL);
        if (result == QVR_SUCCESS) {
            LOGI("Set tracking mode to POSITIONAL (6DOF SLAM)");
        } else {
            LOGW("Failed to set tracking mode to POSITIONAL: %d", result);
        }
    }
    
    // Initialize tracking state
    g_trackingQuality = 0.0f;
    g_trackingState = 0;
    g_trackingWarningFlags = 0;
    g_relocationInProgress = false;
    
    g_trackingInitialized = true;
    
    LOGI("XR2 tracking (SLAM) initialized");
    return true;
}

void ShutdownXR2Tracking() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_trackingInitialized) {
        return;
    }
    
    LOGI("Shutting down XR2 tracking");
    
    // Reset tracking state
    g_trackingQuality = 0.0f;
    g_trackingState = 0;
    g_trackingWarningFlags = 0;
    g_relocationInProgress = false;
    
    g_trackingInitialized = false;
    LOGI("XR2 tracking shut down");
}

bool GetXR2ViewPoses(XrTime time, XrSpace space, XrView* views, uint32_t count, 
                     XrViewStateFlags* viewStateFlags) {
    if (!views || count < 2 || !viewStateFlags) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        return false;
    }
    
    // Get head tracking data from QVR
    qvrservice_head_tracking_data_t* trackingData = nullptr;
    int result = QVRServiceClient_GetHeadTrackingDataWrapper(qvrClient, &trackingData);
    if (result != QVR_SUCCESS || !trackingData) {
        LOGE("Failed to get head tracking data: %d", result);
        return false;
    }
    
    // Convert QVR pose to OpenXR pose
    XrPosef headPose;
    QVRPoseToXrPose(trackingData, &headPose);
    
    // Get eye offsets and FOV
    XrVector3f leftEyeOffset = {0, 0, 0};
    XrVector3f rightEyeOffset = {0, 0, 0};
    XrFovf leftEyeFov = {0, 0, 0, 0};
    XrFovf rightEyeFov = {0, 0, 0, 0};
    
    GetXR2EyeOffsets(&leftEyeOffset, &rightEyeOffset);
    GetXR2ViewFOV(&leftEyeFov, &rightEyeFov);
    
    // Set view poses with eye offsets
    for (uint32_t i = 0; i < count && i < 2; ++i) {
        views[i].type = XR_TYPE_VIEW;
        
        // Apply eye offset to head pose
        XrVector3f eyeOffset = (i == 0) ? leftEyeOffset : rightEyeOffset;
        
        // Transform eye offset by head rotation using quaternion rotation
        // Correct formula: v' = q * v * q^-1, where v is treated as pure quaternion (0, x, y, z)
        float qx = headPose.orientation.x;
        float qy = headPose.orientation.y;
        float qz = headPose.orientation.z;
        float qw = headPose.orientation.w;
        
        // Rotate vector by quaternion: v' = q * v * q^-1
        // For pure quaternion v = (0, x, y, z), this simplifies to:
        // v' = 2 * dot(q.xyz, v) * q.xyz + (qw^2 - dot(q.xyz, q.xyz)) * v + 2 * qw * cross(q.xyz, v)
        // Or using the standard formula:
        // t = 2 * cross(q.xyz, v)
        // v' = v + qw * t + cross(q.xyz, t)
        
        // Cross product: q.xyz × eyeOffset
        float crossX = qy * eyeOffset.z - qz * eyeOffset.y;
        float crossY = qz * eyeOffset.x - qx * eyeOffset.z;
        float crossZ = qx * eyeOffset.y - qy * eyeOffset.x;
        
        // t = 2 * cross
        float tx = 2.0f * crossX;
        float ty = 2.0f * crossY;
        float tz = 2.0f * crossZ;
        
        // v' = v + qw * t + cross(q.xyz, t)
        float rotatedX = eyeOffset.x + qw * tx + (qy * tz - qz * ty);
        float rotatedY = eyeOffset.y + qw * ty + (qz * tx - qx * tz);
        float rotatedZ = eyeOffset.z + qw * tz + (qx * ty - qy * tx);
        
        views[i].pose.position.x = headPose.position.x + rotatedX;
        views[i].pose.position.y = headPose.position.y + rotatedY;
        views[i].pose.position.z = headPose.position.z + rotatedZ;
        views[i].pose.orientation = headPose.orientation;
        
        // Set FOV
        views[i].fov = (i == 0) ? leftEyeFov : rightEyeFov;
    }
    
    // Update tracking state and quality
    g_trackingState = trackingData->tracking_state;
    g_trackingWarningFlags = trackingData->tracking_warning_flags;
    g_trackingQuality = trackingData->pose_quality;
    g_relocationInProgress = (trackingData->tracking_state & 0x1) != 0; // RELOCATION_IN_PROGRESS bit
    
    // Check tracking state
    *viewStateFlags = 0;
    
    // TRACKING bit (bit 2)
    if (g_trackingState & 0x4) {
        *viewStateFlags |= XR_VIEW_STATE_ORIENTATION_TRACKED_BIT;
        *viewStateFlags |= XR_VIEW_STATE_POSITION_TRACKED_BIT;
    }
    
    // Log tracking quality and warnings
    if (g_trackingQuality < 0.5f) {
        LOGW("Low tracking quality: %.2f", g_trackingQuality);
    }
    
    if (g_trackingWarningFlags != 0) {
        if (g_trackingWarningFlags & 0x1) {
            LOGW("Tracking warning: LOW_FEATURE_COUNT_ERROR");
        }
        if (g_trackingWarningFlags & 0x2) {
            LOGW("Tracking warning: LOW_LIGHT_ERROR");
        }
        if (g_trackingWarningFlags & 0x4) {
            LOGW("Tracking warning: BRIGHT_LIGHT_ERROR");
        }
        if (g_trackingWarningFlags & 0x8) {
            LOGW("Tracking warning: STEREO_CAMERA_CALIBRATION_ERROR");
        }
    }
    
    if (g_relocationInProgress) {
        LOGI("Tracking: Relocation in progress");
    }
    
    return true;
}

// Predict pose forward in time using QVR prediction coefficients
static void PredictPose(const qvrservice_head_tracking_data_t* trackingData, 
                        XrTime targetTime, XrPosef* predictedPose) {
    if (!trackingData || !predictedPose) {
        return;
    }
    
    // Get current pose
    QVRPoseToXrPose(trackingData, predictedPose);
    
    // Calculate prediction delta time in seconds
    XrTime currentTime = QVRTimeToXrTime(trackingData->ts);
    XrDuration deltaTimeNs = targetTime - currentTime;
    float deltaTimeS = static_cast<float>(deltaTimeNs) / 1e9f;
    
    if (deltaTimeS <= 0.0f || deltaTimeS > 0.1f) {
        // No prediction needed or prediction too far in future
        return;
    }
    
    // Apply prediction coefficients (using QVR prediction model)
    // QVR provides prediction coefficients for velocity (s), acceleration (b), 
    // and higher order terms (bdt, bdt2)
    float predX = g_predictionCoeffS[0] * deltaTimeS + 
                  g_predictionCoeffB[0] * deltaTimeS * deltaTimeS +
                  g_predictionCoeffBdt[0] * deltaTimeS * deltaTimeS * deltaTimeS;
    float predY = g_predictionCoeffS[1] * deltaTimeS + 
                  g_predictionCoeffB[1] * deltaTimeS * deltaTimeS +
                  g_predictionCoeffBdt[1] * deltaTimeS * deltaTimeS * deltaTimeS;
    float predZ = g_predictionCoeffS[2] * deltaTimeS + 
                  g_predictionCoeffB[2] * deltaTimeS * deltaTimeS +
                  g_predictionCoeffBdt[2] * deltaTimeS * deltaTimeS * deltaTimeS;
    
    // Apply position prediction
    predictedPose->position.x += predX;
    predictedPose->position.y += predY;
    predictedPose->position.z += predZ;
    
    // Apply rotation prediction using quaternion interpolation
    // Use angular velocity approximation for rotation prediction
    // Extract angular velocity from prediction coefficients if available
    // For now, use SLERP (Spherical Linear Interpolation) based on time delta
    
    // Calculate rotation prediction using quaternion exponentiation
    // q(t) = q(0) * exp(0.5 * omega * t), where omega is angular velocity
    // Simplified: use quaternion slerp with a small rotation based on time delta
    
    // Get current rotation quaternion
    XrQuaternionf currentRot = predictedPose->orientation;
    
    // For small time deltas, approximate rotation change
    // This is a simplified approach - full implementation would use angular velocity
    if (deltaTimeS > 0.0f && deltaTimeS < 0.1f) {
        // Use identity quaternion as base (no rotation change prediction for now)
        // In a full implementation, we'd extract angular velocity from tracking data
        // and apply it: q_predicted = q_current * q_angular_velocity_delta
        // For now, keep current rotation (rotation prediction requires angular velocity data)
    }
    
    // Note: Orientation prediction would require quaternion math
    // For now, we only predict position
}

bool LocateXR2ReferenceSpace(XrReferenceSpaceType space, XrReferenceSpaceType baseSpace,
                             XrTime time, XrPosef* pose, XrSpaceLocationFlags* locationFlags) {
    if (!pose || !locationFlags) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        return false;
    }
    
    // Get head tracking data from QVR
    qvrservice_head_tracking_data_t* trackingData = nullptr;
    int result = QVRServiceClient_GetHeadTrackingDataWrapper(qvrClient, &trackingData);
    if (result != QVR_SUCCESS || !trackingData) {
        LOGE("Failed to get head tracking data: %d", result);
        return false;
    }
    
    // Convert QVR pose to OpenXR pose
    QVRPoseToXrPose(trackingData, pose);
    
    // Apply pose prediction if target time is in the future
    if (time > QVRTimeToXrTime(trackingData->ts)) {
        PredictPose(trackingData, time, pose);
    }
    
    // Transform between reference spaces if needed
    if (space != baseSpace && baseSpace == XR_REFERENCE_SPACE_TYPE_LOCAL) {
        // For now, assume LOCAL is the base space
        // In a full implementation, we would transform between spaces
    }
    
    // Update tracking state
    g_trackingState = trackingData->tracking_state;
    g_trackingWarningFlags = trackingData->tracking_warning_flags;
    g_trackingQuality = trackingData->pose_quality;
    g_relocationInProgress = (trackingData->tracking_state & 0x1) != 0;
    
    // Set location flags based on tracking state
    *locationFlags = 0;
    
    // TRACKING bit (bit 2)
    if (g_trackingState & 0x4) {
        *locationFlags |= XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
        *locationFlags |= XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
    }
    
    // Check if position is valid (for positional tracking)
    if (g_trackingQuality > 0.0f) {
        *locationFlags |= XR_SPACE_LOCATION_POSITION_VALID_BIT;
    }
    
    // Handle relocation state
    if (g_relocationInProgress) {
        // During relocation, position may be invalid
        *locationFlags &= ~XR_SPACE_LOCATION_POSITION_VALID_BIT;
    }
    
    // Handle tracking suspension (bit 1)
    if (g_trackingState & 0x2) {
        LOGW("Tracking suspended");
        *locationFlags &= ~(XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | 
                          XR_SPACE_LOCATION_POSITION_TRACKED_BIT);
    }
    
    // Handle fatal error (bit 3)
    if (g_trackingState & 0x8) {
        LOGE("Tracking fatal error - attempting recovery");
        *locationFlags = 0;
        
        // Attempt recovery by reinitializing tracking
        static uint32_t recoveryAttempts = 0;
        if (recoveryAttempts < 3) {
            recoveryAttempts++;
            LOGI("Attempting tracking recovery (attempt %u)", recoveryAttempts);
            
            // Reinitialize tracking
            ShutdownXR2Tracking();
            if (InitializeXR2Tracking()) {
                LOGI("Tracking recovery successful");
                recoveryAttempts = 0;
            } else {
                LOGW("Tracking recovery failed");
            }
        } else {
            LOGE("Tracking recovery failed after %u attempts", recoveryAttempts);
        }
        
        return false;
    }
    
    return true;
}

bool GetXR2ViewFOV(XrFovf* leftEyeFov, XrFovf* rightEyeFov) {
    if (!leftEyeFov || !rightEyeFov) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        // Default FOV values (typical for XR2: ~90 degrees horizontal)
        float defaultFov = 1.0f; // ~57.3 degrees in radians (tan(57.3/2) ≈ 0.5)
        *leftEyeFov = {-defaultFov, defaultFov, -defaultFov, defaultFov};
        *rightEyeFov = {-defaultFov, defaultFov, -defaultFov, defaultFov};
        return true;
    }
    
    // Try to get FOV from QVR hardware transforms or params
    // For now, use default values - actual FOV would come from device calibration
    // Typical XR2 FOV: ~90-100 degrees horizontal per eye
    float hFov = 1.0f; // ~57.3 degrees
    float vFov = 1.0f; // ~57.3 degrees
    
    char value[256] = {0};
    uint32_t len = sizeof(value);
    int result = QVRServiceClient_GetParamWrapper(qvrClient, "fov-horizontal", &len, value);
    if (result == QVR_SUCCESS && len > 0) {
        hFov = static_cast<float>(atof(value));
    }
    
    len = sizeof(value);
    result = QVRServiceClient_GetParamWrapper(qvrClient, "fov-vertical", &len, value);
    if (result == QVR_SUCCESS && len > 0) {
        vFov = static_cast<float>(atof(value));
    }
    
    // Set FOV (left/right, up/down angles in radians)
    *leftEyeFov = {-hFov, hFov, -vFov, vFov};
    *rightEyeFov = {-hFov, hFov, -vFov, vFov};
    
    LOGI("View FOV: left=[%.3f, %.3f, %.3f, %.3f], right=[%.3f, %.3f, %.3f, %.3f]",
         leftEyeFov->angleLeft, leftEyeFov->angleRight,
         leftEyeFov->angleUp, leftEyeFov->angleDown,
         rightEyeFov->angleLeft, rightEyeFov->angleRight,
         rightEyeFov->angleUp, rightEyeFov->angleDown);
    
    return true;
}

bool GetXR2EyeOffsets(XrVector3f* leftEyeOffset, XrVector3f* rightEyeOffset) {
    if (!leftEyeOffset || !rightEyeOffset) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        // Default IPD: ~64mm (0.064m), half per eye
        leftEyeOffset->x = -0.032f;
        leftEyeOffset->y = 0.0f;
        leftEyeOffset->z = 0.0f;
        rightEyeOffset->x = 0.032f;
        rightEyeOffset->y = 0.0f;
        rightEyeOffset->z = 0.0f;
        return true;
    }
    
    // Get eye offsets from hardware transforms
    uint32_t numTransforms = 0;
    int result = QVRServiceClient_GetHwTransformsWrapper(qvrClient, &numTransforms, nullptr);
    if (result == QVR_SUCCESS && numTransforms > 0) {
        std::vector<qvrservice_hw_transform_t> transforms(numTransforms);
        result = QVRServiceClient_GetHwTransformsWrapper(qvrClient, &numTransforms, transforms.data());
        if (result == QVR_SUCCESS) {
            // Find transforms from HMD to left/right eye
            for (uint32_t i = 0; i < numTransforms; ++i) {
                if (transforms[i].from == QVRSERVICE_HW_COMP_ID_HMD) {
                    if (transforms[i].to == QVRSERVICE_HW_COMP_ID_EYE_TRACKING_CAM_L) {
                        // Left eye offset from transform matrix (position is in last column)
                        leftEyeOffset->x = transforms[i].m[12];
                        leftEyeOffset->y = transforms[i].m[13];
                        leftEyeOffset->z = transforms[i].m[14];
                    } else if (transforms[i].to == QVRSERVICE_HW_COMP_ID_EYE_TRACKING_CAM_R) {
                        // Right eye offset
                        rightEyeOffset->x = transforms[i].m[12];
                        rightEyeOffset->y = transforms[i].m[13];
                        rightEyeOffset->z = transforms[i].m[14];
                    }
                }
            }
        }
    }
    
    // Fallback to default if not found
    if (leftEyeOffset->x == 0.0f && leftEyeOffset->y == 0.0f && leftEyeOffset->z == 0.0f) {
        leftEyeOffset->x = -0.032f; // Default IPD half
        rightEyeOffset->x = 0.032f;
    }
    
    LOGI("Eye offsets: left=(%.4f, %.4f, %.4f), right=(%.4f, %.4f, %.4f)",
         leftEyeOffset->x, leftEyeOffset->y, leftEyeOffset->z,
         rightEyeOffset->x, rightEyeOffset->y, rightEyeOffset->z);
    
    return true;
}

bool GetXR2StageBounds(XrExtent2Df* bounds) {
    if (!bounds) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        // Default stage bounds: 2m x 2m
        bounds->width = 2.0f;
        bounds->height = 2.0f;
        return true;
    }
    
    // Try to get stage bounds from QVR params
    char value[256] = {0};
    uint32_t len = sizeof(value);
    
    int result = QVRServiceClient_GetParamWrapper(qvrClient, "stage-width", &len, value);
    if (result == QVR_SUCCESS && len > 0) {
        bounds->width = static_cast<float>(atof(value));
    } else {
        bounds->width = 2.0f; // Default 2m
    }
    
    len = sizeof(value);
    result = QVRServiceClient_GetParamWrapper(qvrClient, "stage-height", &len, value);
    if (result == QVR_SUCCESS && len > 0) {
        bounds->height = static_cast<float>(atof(value));
    } else {
        bounds->height = 2.0f; // Default 2m
    }
    
    LOGI("Stage bounds: %.2f x %.2f meters", bounds->width, bounds->height);
    
    return true;
}

bool WaitForXR2NextFrame(XrTime* predictedDisplayTime, XrDuration* predictedDisplayPeriod) {
    if (!predictedDisplayTime || !predictedDisplayPeriod) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        return false;
    }
    
    // Get display interrupt timestamp (VSYNC)
    qvrservice_ts_t* ts = nullptr;
    int result = QVRServiceClient_GetDisplayInterruptTimestampWrapper(qvrClient, DISP_INTERRUPT_VSYNC, &ts);
    
    XrTime currentTime = 0;
    if (result == QVR_SUCCESS && ts) {
        // Convert QVR timestamp to OpenXR time
        currentTime = QVRTimeToXrTime(ts->ts);
        // Calculate period from refresh rate
        *predictedDisplayPeriod = 1000000000ULL / XR2_REFRESH_RATE;
    } else {
        // Fallback: use current time
        auto now = std::chrono::steady_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        currentTime = static_cast<XrTime>(nanos);
        *predictedDisplayPeriod = 1000000000ULL / XR2_REFRESH_RATE;
    }
    
    // Performance monitoring: Calculate FPS and frame time
    if (g_lastFrameTime > 0) {
        XrDuration frameTime = currentTime - g_lastFrameTime;
        float frameTimeMs = static_cast<float>(frameTime) / 1e6f;
        
        // Update average frame time (exponential moving average)
        if (g_averageFrameTime == 0.0f) {
            g_averageFrameTime = frameTimeMs;
        } else {
            g_averageFrameTime = g_averageFrameTime * 0.9f + frameTimeMs * 0.1f;
        }
        
        // Calculate FPS
        g_currentFPS = 1000.0f / g_averageFrameTime;
        
        // Detect dropped frames (frame time > 1.5x expected)
        float expectedFrameTime = 1000.0f / XR2_REFRESH_RATE;
        if (frameTimeMs > expectedFrameTime * 1.5f) {
            g_droppedFrames++;
            LOGW("Frame drop detected: frameTime=%.2f ms, expected=%.2f ms", 
                 frameTimeMs, expectedFrameTime);
        }
        
        // Log performance stats periodically
        if (g_frameCount % FPS_SAMPLE_COUNT == 0) {
            LOGI("Performance: FPS=%.1f, AvgFrameTime=%.2f ms, DroppedFrames=%u",
                 g_currentFPS, g_averageFrameTime, g_droppedFrames);
        }
    }
    
    g_lastFrameTime = currentTime;
    g_frameCount++;
    *predictedDisplayTime = currentTime + *predictedDisplayPeriod;
    
    return true;
}

// Frame rendering state (for time warp)
static XrTime g_lastHeadPoseTime = 0;
static XrTime g_currentHeadPoseTime = 0;
static XrPosef g_lastHeadPose = {{0, 0, 0, 1}, {0, 0, 0}};
static XrPosef g_currentHeadPose = {{0, 0, 0, 1}, {0, 0, 0}};
static XrPosef g_smoothedHeadPose = {{0, 0, 0, 1}, {0, 0, 0}};

// Pose prediction coefficients (from QVR tracking data)
// Note: These are already defined at the top of the file, removing duplicate definitions
static float g_predictionCoeffBdt2[3] = {0, 0, 0};

// Smoothing factor for pose (0.0 = no smoothing, 1.0 = full smoothing)
static const float POSE_SMOOTHING_FACTOR = 0.1f;

bool BeginXR2FrameRendering() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_renderingActive) {
        return false;
    }
    
    // Adaptive quality and power management
    if (g_averageFrameTime > 0.0f) {
        // Adjust performance level based on FPS
        if (g_currentFPS < XR2_REFRESH_RATE * 0.7f && g_currentPerfLevel < XR2_MAX_PERF_LEVEL) {
            // Increase performance level if FPS is low
            g_currentPerfLevel++;
            SetXR2PerformanceLevel(g_currentPerfLevel);
            LOGI("Increased performance level to %u (FPS: %.1f)", g_currentPerfLevel, g_currentFPS);
        } else if (g_currentFPS > XR2_REFRESH_RATE * 0.95f && g_currentPerfLevel > 0) {
            // Decrease performance level if FPS is stable (save power)
            g_currentPerfLevel--;
            SetXR2PerformanceLevel(g_currentPerfLevel);
            LOGI("Decreased performance level to %u (FPS: %.1f) - power saving", 
                 g_currentPerfLevel, g_currentFPS);
        }
        
        // Warn if FPS is still low after adjustment
        if (g_currentFPS < XR2_REFRESH_RATE * 0.7f) {
            LOGW("Low FPS detected: %.1f (target: %u), consider reducing quality", 
                 g_currentFPS, XR2_REFRESH_RATE);
        }
    }
    
    // Get current head pose for time warp prediction
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (qvrClient) {
        qvrservice_head_tracking_data_t* trackingData = nullptr;
        int result = QVRServiceClient_GetHeadTrackingDataWrapper(qvrClient, &trackingData);
        if (result == QVR_SUCCESS && trackingData) {
            g_lastHeadPose = g_currentHeadPose;
            QVRPoseToXrPose(trackingData, &g_currentHeadPose);
            g_lastHeadPoseTime = g_currentHeadPoseTime;
            g_currentHeadPoseTime = QVRTimeToXrTime(trackingData->ts);
        }
    }
    
    return true;
}

bool EndXR2FrameRendering() {
    // Frame rendering ended, ready for submission
    return true;
}

// Calculate time warp matrix for a given eye and predicted display time
static void CalculateTimeWarpMatrix(const XrPosef& renderPose, const XrPosef& displayPose,
                                    const XrFovf& fov, float* warpMatrix) {
    if (!warpMatrix) {
        return;
    }
    
    // Calculate rotation difference between render pose and display pose
    // Time warp corrects for head movement between render time and display time
    
    // Get quaternion inverse of render pose
    XrQuaternionf renderInv = {
        -renderPose.orientation.x,
        -renderPose.orientation.y,
        -renderPose.orientation.z,
        renderPose.orientation.w
    };
    
    // Calculate relative rotation: displayPose * renderPose^-1
    // This gives the rotation needed to correct from render pose to display pose
    XrQuaternionf relativeRot;
    relativeRot.x = displayPose.orientation.w * renderInv.x + displayPose.orientation.x * renderInv.w +
                    displayPose.orientation.y * renderInv.z - displayPose.orientation.z * renderInv.y;
    relativeRot.y = displayPose.orientation.w * renderInv.y - displayPose.orientation.x * renderInv.z +
                    displayPose.orientation.y * renderInv.w + displayPose.orientation.z * renderInv.x;
    relativeRot.z = displayPose.orientation.w * renderInv.z + displayPose.orientation.x * renderInv.y -
                    displayPose.orientation.y * renderInv.x + displayPose.orientation.z * renderInv.w;
    relativeRot.w = displayPose.orientation.w * renderInv.w - displayPose.orientation.x * renderInv.x -
                    displayPose.orientation.y * renderInv.y - displayPose.orientation.z * renderInv.z;
    
    // Normalize quaternion
    float len = sqrtf(relativeRot.x * relativeRot.x + relativeRot.y * relativeRot.y + 
                     relativeRot.z * relativeRot.z + relativeRot.w * relativeRot.w);
    if (len > 0.0001f) {
        relativeRot.x /= len;
        relativeRot.y /= len;
        relativeRot.z /= len;
        relativeRot.w /= len;
    }
    
    // Convert quaternion to rotation matrix (column-major for OpenGL)
    float x = relativeRot.x, y = relativeRot.y, z = relativeRot.z, w = relativeRot.w;
    
    // Rotation matrix from quaternion
    warpMatrix[0] = 1.0f - 2.0f * (y * y + z * z);
    warpMatrix[1] = 2.0f * (x * y + z * w);
    warpMatrix[2] = 2.0f * (x * z - y * w);
    warpMatrix[3] = 0.0f;
    
    warpMatrix[4] = 2.0f * (x * y - z * w);
    warpMatrix[5] = 1.0f - 2.0f * (x * x + z * z);
    warpMatrix[6] = 2.0f * (y * z + x * w);
    warpMatrix[7] = 0.0f;
    
    warpMatrix[8] = 2.0f * (x * z + y * w);
    warpMatrix[9] = 2.0f * (y * z - x * w);
    warpMatrix[10] = 1.0f - 2.0f * (x * x + y * y);
    warpMatrix[11] = 0.0f;
    
    warpMatrix[12] = 0.0f;
    warpMatrix[13] = 0.0f;
    warpMatrix[14] = 0.0f;
    warpMatrix[15] = 1.0f;
}

bool SubmitXR2FrameLayers(const XrCompositionLayerBaseHeader* const* layers, uint32_t layerCount) {
    if (!layers || layerCount == 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_renderingActive) {
        LOGE("Cannot submit layers: rendering not active");
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        LOGE("Cannot submit layers: QVR client not available");
        return false;
    }
    
    // Get predicted display time for time warp
    // Use late-latching: predict pose as close to VSYNC as possible
    XrTime predictedDisplayTime = 0;
    XrDuration predictedDisplayPeriod = 0;
    WaitForXR2NextFrame(&predictedDisplayTime, &predictedDisplayPeriod);
    
    // Optimize motion-to-photon latency by predicting pose at display time
    // This reduces the time between pose capture and display
    
    // Get current head pose for time warp
    qvrservice_head_tracking_data_t* trackingData = nullptr;
    int result = QVRServiceClient_GetHeadTrackingDataWrapper(qvrClient, &trackingData);
    if (result != QVR_SUCCESS || !trackingData) {
        LOGW("Failed to get head tracking data for time warp");
    }
    
    XrPosef displayPose = g_currentHeadPose;
    if (trackingData) {
        QVRPoseToXrPose(trackingData, &displayPose);
    }
    
    // Process each layer
    for (uint32_t i = 0; i < layerCount; ++i) {
        const XrCompositionLayerBaseHeader* layer = layers[i];
        if (!layer) {
            continue;
        }
        
        switch (layer->type) {
            case XR_TYPE_COMPOSITION_LAYER_PROJECTION: {
                const XrCompositionLayerProjection* projLayer = 
                    reinterpret_cast<const XrCompositionLayerProjection*>(layer);
                
                // Validate layer
                if (!projLayer->views || projLayer->viewCount == 0) {
                    LOGW("Invalid projection layer: no views");
                    continue;
                }
                
                // Process each view (eye)
                for (uint32_t viewIdx = 0; viewIdx < projLayer->viewCount && viewIdx < 2; ++viewIdx) {
                    const XrCompositionLayerProjectionView* view = &projLayer->views[viewIdx];
                    
                    // Get FOV for this eye
                    XrFovf fov = view->fov;
                    
                    // Calculate time warp matrix
                    float warpMatrix[16];
                    CalculateTimeWarpMatrix(g_currentHeadPose, displayPose, fov, warpMatrix);
                    
                    // In a real implementation, we would:
                    // 1. Submit the swapchain image to the compositor
                    // 2. Apply time warp using the calculated matrix
                    // 3. Apply lens distortion correction
                    // 4. Render to the display
                    
                    LOGI("Submitting layer %u, view %u: image=%llu, pose=(%.3f,%.3f,%.3f)",
                         i, viewIdx, 
                         reinterpret_cast<uint64_t>(view->subImage.swapchain),
                         view->pose.position.x, view->pose.position.y, view->pose.position.z);
                }
                break;
            }
            
            case XR_TYPE_COMPOSITION_LAYER_QUAD: {
                const XrCompositionLayerQuad* quadLayer = 
                    reinterpret_cast<const XrCompositionLayerQuad*>(layer);
                
                // Quad layers don't need time warp (they're head-locked)
                LOGI("Submitting quad layer %u: image=%llu", i,
                     reinterpret_cast<uint64_t>(quadLayer->subImage.swapchain));
                break;
            }
            
            default:
                LOGW("Unsupported layer type: %u", layer->type);
                break;
        }
    }
    
    // In a real implementation, we would:
    // - Submit all layers to the compositor
    // - Trigger VSYNC
    // - Apply asynchronous time warp just before display refresh
    
    LOGI("Submitted %u layers to compositor", layerCount);
    return true;
}

// Controller state
struct ControllerState {
    bool connected;
    uint32_t buttonState;
    float analog1D[8];
    XrVector2f analog2D[4];
    XrPosef pose;
    uint64_t timestamp;
    uint32_t lastButtonState;
    float lastAnalog1D[8];
    XrVector2f lastAnalog2D[4];
    int controllerHandle;  // QVR controller handle
};

static ControllerState g_controllers[2] = {};
static bool g_controllersInitialized = false;
static std::mutex g_controllerMutex;

// Initialize controllers
static bool InitializeControllers() {
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    if (g_controllersInitialized) {
        return true;
    }
    
    LOGI("Initializing XR2 controllers");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        LOGE("QVR client not available for controller initialization");
        return false;
    }
    
    // Initialize controller states
    for (int i = 0; i < 2; ++i) {
        g_controllers[i].connected = false;
        g_controllers[i].buttonState = 0;
        g_controllers[i].timestamp = 0;
        g_controllers[i].lastButtonState = 0;
        g_controllers[i].controllerHandle = -1;
        memset(g_controllers[i].analog1D, 0, sizeof(g_controllers[i].analog1D));
        memset(g_controllers[i].analog2D, 0, sizeof(g_controllers[i].analog2D));
        memset(g_controllers[i].lastAnalog1D, 0, sizeof(g_controllers[i].lastAnalog1D));
        memset(g_controllers[i].lastAnalog2D, 0, sizeof(g_controllers[i].lastAnalog2D));
        g_controllers[i].pose = XrPosef{{0, 0, 0, 1}, {0, 0, 0}};
    }
    
    // Start tracking controllers using QVR API
    // Note: QVRServiceClient_StartTrackingWrapper doesn't exist in the API
    // Controllers are tracked automatically when tracking mode is set
    // We'll mark controllers as connected when they're detected
    // In a real implementation, you'd use QVRServiceClient_GetControllerState or similar
    for (int i = 0; i < 2; ++i) {
        // Placeholder: Controllers will be detected when they're actually connected
        g_controllers[i].controllerHandle = -1; // No handle available
        g_controllers[i].connected = false; // Will be set to true when detected
    }
    
    g_controllersInitialized = true;
    LOGI("XR2 controllers initialized");
    return true;
}

// Get controller index from subaction path (0 = left, 1 = right)
static uint32_t GetControllerIndex(XrPath subactionPath) {
    if (subactionPath == XR_NULL_PATH) {
        return 0; // Default to left if no subaction specified
    }
    
    // Parse subaction path: /user/hand/{left|right}
    // Use ParseInputPath helper but with a simplified path
    // For subaction paths, we can check if it's a hand path
    
    // Try to get path string
    XrInstance instance = GetCurrentInstance();
    if (instance != XR_NULL_HANDLE) {
        std::string pathString = GetPathString(instance, subactionPath);
        
        if (!pathString.empty()) {
            // Check if path contains "right"
            if (pathString.find("/right") != std::string::npos || 
                pathString.find("right") != std::string::npos) {
                return 1; // Right controller
            } else if (pathString.find("/left") != std::string::npos || 
                      pathString.find("left") != std::string::npos) {
                return 0; // Left controller
            }
        }
    }
    
    // Default to left (0)
    return 0;
}

// Map input type and component to controller input index
static uint32_t MapInputToControllerIndex(const std::string& inputType, const std::string& component) {
    // Map OpenXR input paths to controller input indices
    // This maps to sxrControllerState structure indices
    
    if (inputType == "trigger" && component == "value") {
        return 0; // PrimaryIndexTrigger
    } else if (inputType == "trigger" && component == "click") {
        return 0; // Same as value for boolean
    } else if (inputType == "squeeze" && component == "value") {
        return 2; // PrimaryHandTrigger
    } else if (inputType == "thumbstick" && component == "x") {
        return 0; // PrimaryThumbstick X
    } else if (inputType == "thumbstick" && component == "y") {
        return 0; // PrimaryThumbstick Y
    } else if (inputType == "trackpad" && component == "x") {
        return 0; // Trackpad X
    } else if (inputType == "trackpad" && component == "y") {
        return 0; // Trackpad Y
    }
    
    return 0; // Default
}

// Map input type to button bitmask
static uint32_t MapInputToButtonBit(const std::string& inputType, const std::string& component) {
    // Map to sxrControllerButton enum values
    if (inputType == "a" || inputType == "button_a") {
        return 0x00000001; // Button One
    } else if (inputType == "b" || inputType == "button_b") {
        return 0x00000002; // Button Two
    } else if (inputType == "x" || inputType == "button_x") {
        return 0x00000004; // Button Three
    } else if (inputType == "y" || inputType == "button_y") {
        return 0x00000008; // Button Four
    } else if (inputType == "thumbstick" && component == "click") {
        return 0x00010000; // Thumbstick click
    } else if (inputType == "trackpad" && component == "click") {
        return 0x00020000; // Trackpad click
    } else if (inputType == "trigger" && component == "click") {
        return 0x00040000; // Trigger click
    } else if (inputType == "squeeze" && component == "click") {
        return 0x00080000; // Squeeze click
    }
    
    return 0;
}

bool GetXR2BooleanInput(XrAction action, XrPath subactionPath, bool* state, bool* changed) {
    if (!state || !changed) {
        return false;
    }
    
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    // Get binding path for this action
    XrPath bindingPath = GetActionBindingPath(action);
    
    uint32_t controllerIdx = 0;
    uint32_t buttonBit = 0;
    
    if (bindingPath != XR_NULL_PATH) {
        // Parse binding path to get controller and input info
        ParsedInputPath parsed = ParseInputPath(bindingPath);
        if (parsed.valid) {
            controllerIdx = parsed.controllerIndex;
            buttonBit = MapInputToButtonBit(parsed.inputType, parsed.component);
        } else {
            // Fallback to subaction path
            controllerIdx = GetControllerIndex(subactionPath);
        }
    } else {
        // No binding, use subaction path
        controllerIdx = GetControllerIndex(subactionPath);
    }
    
    if (controllerIdx >= 2) {
        return false;
    }
    
    ControllerState& controller = g_controllers[controllerIdx];
    
    // Read button state
    if (buttonBit != 0) {
        *state = (controller.buttonState & buttonBit) != 0;
        *changed = ((controller.buttonState ^ controller.lastButtonState) & buttonBit) != 0;
    } else {
        // Default: check if any button is pressed
        *state = (controller.buttonState != 0);
        *changed = (controller.buttonState != controller.lastButtonState);
    }
    
    return true;
}

bool GetXR2FloatInput(XrAction action, XrPath subactionPath, float* state, bool* changed) {
    if (!state || !changed) {
        return false;
    }
    
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    // Get binding path for this action
    XrPath bindingPath = GetActionBindingPath(action);
    
    uint32_t controllerIdx = 0;
    uint32_t analogIndex = 0;
    
    if (bindingPath != XR_NULL_PATH) {
        ParsedInputPath parsed = ParseInputPath(bindingPath);
        if (parsed.valid) {
            controllerIdx = parsed.controllerIndex;
            analogIndex = MapInputToControllerIndex(parsed.inputType, parsed.component);
        } else {
            controllerIdx = GetControllerIndex(subactionPath);
        }
    } else {
        controllerIdx = GetControllerIndex(subactionPath);
    }
    
    if (controllerIdx >= 2) {
        return false;
    }
    
    ControllerState& controller = g_controllers[controllerIdx];
    
    // Read analog input (trigger, squeeze, etc.)
    if (analogIndex < 8) {
        *state = controller.analog1D[analogIndex];
        *changed = (controller.analog1D[analogIndex] != controller.lastAnalog1D[analogIndex]);
    } else {
        *state = 0.0f;
        *changed = false;
    }
    
    return true;
}

bool GetXR2Vector2fInput(XrAction action, XrPath subactionPath, XrVector2f* state, bool* changed) {
    if (!state || !changed) {
        return false;
    }
    
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    // Get binding path for this action
    XrPath bindingPath = GetActionBindingPath(action);
    
    uint32_t controllerIdx = 0;
    uint32_t analog2DIndex = 0;
    
    if (bindingPath != XR_NULL_PATH) {
        ParsedInputPath parsed = ParseInputPath(bindingPath);
        if (parsed.valid) {
            controllerIdx = parsed.controllerIndex;
            // Map thumbstick/trackpad to analog2D index
            if (parsed.inputType == "thumbstick") {
                analog2DIndex = 0; // PrimaryThumbstick
            } else if (parsed.inputType == "trackpad") {
                analog2DIndex = 1; // Trackpad
            }
        } else {
            controllerIdx = GetControllerIndex(subactionPath);
        }
    } else {
        controllerIdx = GetControllerIndex(subactionPath);
    }
    
    if (controllerIdx >= 2) {
        return false;
    }
    
    ControllerState& controller = g_controllers[controllerIdx];
    
    // Read 2D analog input (thumbstick, trackpad)
    if (analog2DIndex < 4) {
        *state = controller.analog2D[analog2DIndex];
        *changed = (controller.analog2D[analog2DIndex].x != controller.lastAnalog2D[analog2DIndex].x ||
                   controller.analog2D[analog2DIndex].y != controller.lastAnalog2D[analog2DIndex].y);
    } else {
        *state = {0.0f, 0.0f};
        *changed = false;
    }
    
    return true;
}

bool GetXR2PoseInput(XrAction action, XrPath subactionPath, bool* isActive) {
    if (!isActive) {
        return false;
    }
    
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    uint32_t controllerIdx = GetControllerIndex(subactionPath);
    if (controllerIdx >= 2) {
        return false;
    }
    
    ControllerState& controller = g_controllers[controllerIdx];
    *isActive = controller.connected;
    
    return true;
}

bool GetXR2ActionPose(XrAction action, XrPath subactionPath, XrTime time, 
                      XrPosef* pose, XrSpaceLocationFlags* locationFlags) {
    if (!pose || !locationFlags) {
        return false;
    }
    
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    uint32_t controllerIdx = GetControllerIndex(subactionPath);
    if (controllerIdx >= 2) {
        return false;
    }
    
    ControllerState& controller = g_controllers[controllerIdx];
    
    if (!controller.connected) {
        *locationFlags = 0;
        return false;
    }
    
    // Return controller pose
    *pose = controller.pose;
    *locationFlags = XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | 
                     XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                     XR_SPACE_LOCATION_POSITION_VALID_BIT;
    
    return true;
}

bool GetXR2CurrentInteractionProfile(XrPath topLevelUserPath, XrPath* interactionProfile) {
    if (!interactionProfile) {
        return false;
    }
    
    // In a real implementation, this would:
    // 1. Determine which controller is connected
    // 2. Map controller type to OpenXR interaction profile path
    // 3. Return the appropriate profile
    
    // For now, return null (no profile)
    *interactionProfile = XR_NULL_PATH;
    return false;
}

bool SyncXR2InputActions() {
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        return false;
    }
    
    // Update controller states from QVR API
    for (int i = 0; i < 2; ++i) {
        ControllerState& controller = g_controllers[i];
        
        if (!controller.connected || controller.controllerHandle < 0) {
            continue;
        }
        
        // Save previous state for change detection
        controller.lastButtonState = controller.buttonState;
        memcpy(controller.lastAnalog1D, controller.analog1D, sizeof(controller.lastAnalog1D));
        memcpy(controller.lastAnalog2D, controller.analog2D, sizeof(controller.lastAnalog2D));
        
        // Get controller state from QVR
        // Note: QVR controller API may use different structure
        // For now, we'll use a simplified approach that queries controller state
        // In a full implementation, we'd use QVRServiceClient_GetControllerStateWrapper
        
        // Check if controller is still connected by querying its state
        // This is a placeholder - actual implementation would read full controller state
        // For now, we'll assume controller remains connected if handle is valid
        if (controller.controllerHandle >= 0) {
            controller.connected = true;
            // TODO: Read actual controller state from QVR API when available
            // The controller state structure depends on QVR SDK version
            // For now, state remains unchanged (will be updated when API is integrated)
        } else {
            controller.connected = false;
        }
    }
    
    return true;
}

bool TriggerXR2HapticFeedback(XrAction action, XrPath subactionPath, 
                              float amplitude, XrDuration duration) {
    if (!g_controllersInitialized) {
        InitializeControllers();
    }
    
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    
    uint32_t controllerIdx = GetControllerIndex(subactionPath);
    if (controllerIdx >= 2) {
        return false;
    }
    
    ControllerState& controller = g_controllers[controllerIdx];
    if (!controller.connected) {
        return false;
    }
    
    // In a real implementation, this would:
    // 1. Call sxrControllerSendMessage with kControllerMessageVibration
    // 2. Convert amplitude (0.0-1.0) and duration to controller format
    // 3. Send vibration command to controller
    
    LOGI("Triggering haptic feedback: controller=%u, amplitude=%.2f, duration=%llu ns",
         controllerIdx, amplitude, duration);
    
    return true;
}

bool SetXR2PerformanceLevel(uint32_t level) {
    if (level > XR2_MAX_PERF_LEVEL) {
        level = XR2_MAX_PERF_LEVEL;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        return false;
    }
    
    // Map performance level to QVR performance levels
    qvrservice_perf_level_t perfLevels[2];
    perfLevels[0].hw_type = HW_TYPE_CPU;
    perfLevels[0].perf_level = (QVRSERVICE_PERF_LEVEL)(level + 1); // Map 0-3 to PERF_LEVEL_1-3
    perfLevels[1].hw_type = HW_TYPE_GPU;
    perfLevels[1].perf_level = (QVRSERVICE_PERF_LEVEL)(level + 1); // Map 0-3 to PERF_LEVEL_1-3
    
    int result = QVRServiceClient_SetOperatingLevelWrapper(qvrClient, perfLevels, 2);
    if (result != QVR_SUCCESS) {
        LOGW("Failed to set performance level: %d", result);
        return false;
    }
    
    g_currentPerfLevel = level;
    LOGI("Set performance level to %u (CPU=%u, GPU=%u)", level, level, level);
    return true;
}

bool EnableXR2PowerOptimization(bool enable) {
    g_powerOptimizationEnabled = enable;
    
    if (enable) {
        // Enable power-saving mode
        // This could reduce refresh rate when idle, lower CPU/GPU frequencies, etc.
        LOGI("Power optimization enabled");
    } else {
        // Disable power-saving mode
        LOGI("Power optimization disabled");
    }
    
    return true;
}

XrTime GetXR2CurrentTime() {
    auto now = std::chrono::steady_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    return static_cast<XrTime>(nanos);
}

// Hand tracking state
// Note: g_handTrackingInitialized is already defined at the top of the file, removing duplicate definition

bool InitializeXR2HandTracking() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (g_handTrackingInitialized) {
        return true;
    }
    
    LOGI("Initializing XR2 hand tracking");
    
    // Initialize Snapdragon Spaces SDK for hand tracking
    if (!InitializeSpacesSDK()) {
        LOGE("Failed to initialize Spaces SDK for hand tracking");
        return false;
    }
    
    g_handTrackingInitialized = true;
    LOGI("XR2 hand tracking initialized");
    return true;
}

void ShutdownXR2HandTracking() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_handTrackingInitialized) {
        return;
    }
    
    LOGI("Shutting down XR2 hand tracking");
    
    // Shutdown Spaces SDK hand tracking
    ShutdownSpacesSDK();
    
    g_handTrackingInitialized = false;
    LOGI("XR2 hand tracking shut down");
}

bool GetXR2HandPose(uint32_t handIndex, XrTime time, XrPosef* pose, 
                   XrSpaceLocationFlags* locationFlags) {
    if (!pose || !locationFlags || handIndex >= 2) {
        return false;
    }
    
    if (!g_handTrackingInitialized) {
        return false;
    }
    
    // Get hand pose from Snapdragon Spaces SDK
    float position[3];
    float rotation[4];
    
    if (GetSpacesHandPose(handIndex, position, rotation)) {
        pose->position.x = position[0];
        pose->position.y = position[1];
        pose->position.z = position[2];
        pose->orientation.x = rotation[0];
        pose->orientation.y = rotation[1];
        pose->orientation.z = rotation[2];
        pose->orientation.w = rotation[3];
        
        *locationFlags = XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | 
                         XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                         XR_SPACE_LOCATION_POSITION_VALID_BIT;
        
        return true;
    }
    
    *locationFlags = 0;
    *pose = XrPosef{{0, 0, 0, 1}, {0, 0, 0}};
    
    return false; // Not tracked
}

// Eye tracking state
static bool g_eyeTrackingInitialized = false;

bool InitializeXR2EyeTracking() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (g_eyeTrackingInitialized) {
        return true;
    }
    
    LOGI("Initializing XR2 eye tracking");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        LOGE("QVR client not available for eye tracking");
        return false;
    }
    
    // Check eye tracking capabilities
    // This would use QVRServiceClient_GetEyeTrackingCapabilities if available
    
    // Set eye tracking mode to DUAL (both eyes)
    // int result = QVRServiceClient_SetEyeTrackingModeWrapper(qvrClient, QVRSERVICE_EYE_TRACKING_MODE_DUAL);
    // if (result != QVR_SUCCESS) {
    //     LOGW("Failed to set eye tracking mode: %d", result);
    //     return false;
    // }
    
    g_eyeTrackingInitialized = true;
    LOGI("XR2 eye tracking initialized");
    return true;
}

void ShutdownXR2EyeTracking() {
    std::lock_guard<std::mutex> lock(g_xr2Mutex);
    
    if (!g_eyeTrackingInitialized) {
        return;
    }
    
    LOGI("Shutting down XR2 eye tracking");
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (qvrClient) {
        // Disable eye tracking
        // QVRServiceClient_SetEyeTrackingModeWrapper(qvrClient, QVRSERVICE_EYE_TRACKING_MODE_NONE);
    }
    
    g_eyeTrackingInitialized = false;
    LOGI("XR2 eye tracking shut down");
}

bool GetXR2EyeGaze(XrTime time, XrVector3f* gazeOrigin, XrVector3f* gazeDirection) {
    if (!gazeOrigin || !gazeDirection) {
        return false;
    }
    
    if (!g_eyeTrackingInitialized) {
        return false;
    }
    
    QVRServiceClientHandle qvrClient = GetQVRClient();
    if (!qvrClient) {
        return false;
    }
    
    // Get eye tracking data from QVR
    // qvrservice_eye_tracking_data_t* eyeData = nullptr;
    // int result = QVRServiceClient_GetEyeTrackingDataWrapper(qvrClient, &eyeData);
    // if (result != QVR_SUCCESS || !eyeData) {
    //     return false;
    // }
    
    // Extract gaze origin and direction from eye tracking data
    // For now, return default values
    gazeOrigin->x = 0.0f;
    gazeOrigin->y = 0.0f;
    gazeOrigin->z = 0.0f;
    
    gazeDirection->x = 0.0f;
    gazeDirection->y = 0.0f;
    gazeDirection->z = -1.0f; // Forward
    
    return false; // Not implemented yet
}

