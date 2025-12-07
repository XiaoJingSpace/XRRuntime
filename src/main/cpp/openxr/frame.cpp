#include "openxr_api.h"
#include "qualcomm/xr2_platform.h"
#include "platform/frame_sync.h"
#include "utils/logger.h"
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <memory>

// External declarations
extern std::mutex g_sessionMutex;
extern std::unordered_map<XrSession, std::shared_ptr<XRSession>> g_sessions;

static std::mutex g_frameMutex;

XrResult xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState) {
    if (!frameWaitInfo || !frameState) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (frameWaitInfo->type != XR_TYPE_FRAME_WAIT_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (frameState->type != XR_TYPE_FRAME_STATE) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    auto& sess = sessIt->second;
    if (!sess->active) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }
    
    // Wait for next frame from display
    XrTime predictedDisplayTime;
    XrDuration predictedDisplayPeriod;
    
    if (!WaitForNextFrame(&predictedDisplayTime, &predictedDisplayPeriod)) {
        LOGE("Failed to wait for next frame");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    // Check if should render
    bool shouldRender = true; // Platform determines this
    
    frameState->predictedDisplayTime = predictedDisplayTime;
    frameState->predictedDisplayPeriod = predictedDisplayPeriod;
    frameState->shouldRender = shouldRender ? XR_TRUE : XR_FALSE;
    
    return XR_SUCCESS;
}

XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
    if (!frameBeginInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (frameBeginInfo->type != XR_TYPE_FRAME_BEGIN_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    auto& sess = sessIt->second;
    if (!sess->active) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }
    
    // Begin frame rendering
    if (!BeginFrameRendering()) {
        LOGE("Failed to begin frame rendering");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    return XR_SUCCESS;
}

XrResult xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
    if (!frameEndInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (frameEndInfo->type != XR_TYPE_FRAME_END_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    auto& sess = sessIt->second;
    if (!sess->active) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }
    
    // Validate layer count
    if (frameEndInfo->layerCount > 0 && !frameEndInfo->layers) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Submit layers to compositor
    if (frameEndInfo->layerCount > 0) {
        if (!SubmitFrameLayers(frameEndInfo->layers, frameEndInfo->layerCount)) {
            LOGE("Failed to submit frame layers");
            return XR_ERROR_RUNTIME_FAILURE;
        }
    }
    
    // End frame rendering
    if (!EndFrameRendering()) {
        LOGE("Failed to end frame rendering");
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    return XR_SUCCESS;
}

