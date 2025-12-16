#include "openxr_api.h"
#include "qualcomm/xr2_platform.h"
#include "platform/input_manager.h"
#include "utils/logger.h"
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <cstring>

// External declarations
extern std::mutex g_sessionMutex;
extern std::unordered_map<XrSession, std::shared_ptr<XRSession>> g_sessions;

struct XRSpace {
    XrSession session;
    XrReferenceSpaceType referenceSpaceType;
    XrPosef poseInReferenceSpace;
    bool isActionSpace;
    XrAction action;
    XrPath subactionPath;
    
    XRSpace(XrSession sess, XrReferenceSpaceType type) 
        : session(sess), referenceSpaceType(type), isActionSpace(false) {
        poseInReferenceSpace = XrPosef{{0, 0, 0, 1}, {0, 0, 0}};
    }
    
    XRSpace(XrSession sess, XrAction act, XrPath subaction) 
        : session(sess), isActionSpace(true), action(act), subactionPath(subaction) {
        referenceSpaceType = XR_REFERENCE_SPACE_TYPE_MAX_ENUM;
        poseInReferenceSpace = XrPosef{{0, 0, 0, 1}, {0, 0, 0}};
    }
};

std::mutex g_spaceMutex;
std::unordered_map<XrSpace, std::shared_ptr<XRSpace>> g_spaces;
static XrSpace g_nextSpaceHandle = reinterpret_cast<XrSpace>(0x2000);

XrResult xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space) {
    if (!createInfo || !space) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (createInfo->type != XR_TYPE_REFERENCE_SPACE_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Validate reference space type
    XrReferenceSpaceType spaceType = createInfo->referenceSpaceType;
    if (spaceType != XR_REFERENCE_SPACE_TYPE_VIEW &&
        spaceType != XR_REFERENCE_SPACE_TYPE_LOCAL &&
        spaceType != XR_REFERENCE_SPACE_TYPE_STAGE) {
        return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED;
    }
    
    // Create space
    auto xrSpace = std::make_shared<XRSpace>(session, spaceType);
    xrSpace->poseInReferenceSpace = createInfo->poseInReferenceSpace;
    
    // Register space
    std::lock_guard<std::mutex> spaceLock(g_spaceMutex);
    // Increment handle using integer arithmetic (handles are pointers in 64-bit)
    uintptr_t handleValue = reinterpret_cast<uintptr_t>(g_nextSpaceHandle);
    handleValue++;
    XrSpace handle = reinterpret_cast<XrSpace>(handleValue);
    g_nextSpaceHandle = handle;
    g_spaces[handle] = xrSpace;
    *space = handle;
    
    LOGI("Reference space created: %p, type: %d", handle, spaceType);
    return XR_SUCCESS;
}

XrResult xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo* createInfo, XrSpace* space) {
    if (!createInfo || !space) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (createInfo->type != XR_TYPE_ACTION_SPACE_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Create action space
    auto xrSpace = std::make_shared<XRSpace>(session, createInfo->action, createInfo->subactionPath);
    xrSpace->poseInReferenceSpace = createInfo->poseInActionSpace;
    
    // Register space
    std::lock_guard<std::mutex> spaceLock(g_spaceMutex);
    // Increment handle using integer arithmetic (handles are pointers in 64-bit)
    uintptr_t handleValue = reinterpret_cast<uintptr_t>(g_nextSpaceHandle);
    handleValue++;
    XrSpace handle = reinterpret_cast<XrSpace>(handleValue);
    g_nextSpaceHandle = handle;
    g_spaces[handle] = xrSpace;
    *space = handle;
    
    LOGI("Action space created: %p", handle);
    return XR_SUCCESS;
}

XrResult xrDestroySpace(XrSpace space) {
    if (!space) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> lock(g_spaceMutex);
    auto it = g_spaces.find(space);
    if (it == g_spaces.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    g_spaces.erase(it);
    
    LOGI("Space destroyed: %p", space);
    return XR_SUCCESS;
}

XrResult xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) {
    if (!location) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (location->type != XR_TYPE_SPACE_LOCATION) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(g_spaceMutex);
    
    // Get space
    auto spaceIt = g_spaces.find(space);
    if (spaceIt == g_spaces.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    auto& xrSpace = spaceIt->second;
    
    // Get base space
    auto baseIt = g_spaces.find(baseSpace);
    if (baseIt == g_spaces.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    auto& baseXrSpace = baseIt->second;
    
    // Get tracking data from platform
    XrPosef pose;
    XrSpaceLocationFlags locationFlags = 0;
    
    if (xrSpace->isActionSpace) {
        // Get pose from action
        if (!GetActionPose(xrSpace->action, xrSpace->subactionPath, time, &pose, &locationFlags)) {
            location->locationFlags = 0;
            return XR_SUCCESS; // Valid but not tracked
        }
    } else {
        // Get pose from reference space
        if (!LocateXR2ReferenceSpace(xrSpace->referenceSpaceType, baseXrSpace->referenceSpaceType, 
                                      time, &pose, &locationFlags)) {
            location->locationFlags = 0;
            return XR_SUCCESS; // Valid but not tracked
        }
    }
    
    location->pose = pose;
    location->locationFlags = locationFlags;
    
    return XR_SUCCESS;
}

XrResult xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, 
                       uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views) {
    if (!viewLocateInfo || !viewState || !viewCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (viewLocateInfo->type != XR_TYPE_VIEW_LOCATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (viewState->type != XR_TYPE_VIEW_STATE) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get view configuration
    if (viewLocateInfo->viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
        return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
    }
    
    // Get view poses from platform
    XrView localViews[2];
    XrViewStateFlags viewStateFlags;
    
    if (!GetXR2ViewPoses(viewLocateInfo->displayTime, viewLocateInfo->space, 
                         localViews, 2, &viewStateFlags)) {
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    *viewCountOutput = 2;
    
    if (views && viewCapacityInput >= 2) {
        views[0] = localViews[0];
        views[1] = localViews[1];
    }
    
    viewState->viewStateFlags = viewStateFlags;
    
    return XR_SUCCESS;
}

