#include "openxr_api.h"
#include "platform/input_manager.h"
#include "utils/logger.h"
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <cstring>

// External declarations
extern std::mutex g_instanceMutex;
extern std::unordered_map<XrInstance, std::shared_ptr<XRInstance>> g_instances;
extern std::mutex g_sessionMutex;
extern std::unordered_map<XrSession, std::shared_ptr<XRSession>> g_sessions;

struct XRActionSet {
    XrInstance instance;
    std::string actionSetName;
    std::string localizedActionSetName;
    uint32_t priority;
};

struct XRAction {
    XrActionSet actionSet;
    XrActionType actionType;
    std::string actionName;
    std::string localizedActionName;
    std::unordered_map<XrPath, XrPath> subactionPaths;
};

std::mutex g_actionSetMutex;
std::unordered_map<XrActionSet, std::shared_ptr<XRActionSet>> g_actionSets;
static XrActionSet g_nextActionSetHandle = reinterpret_cast<XrActionSet>(0x4000);

std::mutex g_actionMutex;
std::unordered_map<XrAction, std::shared_ptr<XRAction>> g_actions;
static XrAction g_nextActionHandle = reinterpret_cast<XrAction>(0x5000);

XrResult xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet) {
    if (!createInfo || !actionSet) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (createInfo->type != XR_TYPE_ACTION_SET_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Create action set
    auto xrActionSet = std::make_shared<XRActionSet>();
    xrActionSet->instance = instance;
    xrActionSet->actionSetName = createInfo->actionSetName;
    xrActionSet->localizedActionSetName = createInfo->localizedActionSetName;
    xrActionSet->priority = createInfo->priority;
    
    // Register action set
    std::lock_guard<std::mutex> setLock(g_actionSetMutex);
    // Increment handle using integer arithmetic (handles are pointers in 64-bit)
    uintptr_t handleValue = reinterpret_cast<uintptr_t>(g_nextActionSetHandle);
    handleValue++;
    XrActionSet handle = reinterpret_cast<XrActionSet>(handleValue);
    g_nextActionSetHandle = handle;
    g_actionSets[handle] = xrActionSet;
    *actionSet = handle;
    
    LOGI("Action set created: %p, name: %s", handle, createInfo->actionSetName);
    return XR_SUCCESS;
}

XrResult xrDestroyActionSet(XrActionSet actionSet) {
    if (!actionSet) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> lock(g_actionSetMutex);
    auto it = g_actionSets.find(actionSet);
    if (it == g_actionSets.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    g_actionSets.erase(it);
    
    LOGI("Action set destroyed: %p", actionSet);
    return XR_SUCCESS;
}

XrResult xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action) {
    if (!createInfo || !action) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (createInfo->type != XR_TYPE_ACTION_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate action set
    std::lock_guard<std::mutex> setLock(g_actionSetMutex);
    auto setIt = g_actionSets.find(actionSet);
    if (setIt == g_actionSets.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Create action
    auto xrAction = std::make_shared<XRAction>();
    xrAction->actionSet = actionSet;
    xrAction->actionType = createInfo->actionType;
    xrAction->actionName = createInfo->actionName;
    xrAction->localizedActionName = createInfo->localizedActionName;
    
    // Register action
    std::lock_guard<std::mutex> actLock(g_actionMutex);
    // Increment handle using integer arithmetic (handles are pointers in 64-bit)
    uintptr_t handleValue = reinterpret_cast<uintptr_t>(g_nextActionHandle);
    handleValue++;
    XrAction handle = reinterpret_cast<XrAction>(handleValue);
    g_nextActionHandle = handle;
    g_actions[handle] = xrAction;
    *action = handle;
    
    LOGI("Action created: %p, name: %s, type: %d", handle, createInfo->actionName, createInfo->actionType);
    return XR_SUCCESS;
}

XrResult xrDestroyAction(XrAction action) {
    if (!action) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> lock(g_actionMutex);
    auto it = g_actions.find(action);
    if (it == g_actions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    g_actions.erase(it);
    
    LOGI("Action destroyed: %p", action);
    return XR_SUCCESS;
}

XrResult xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) {
    if (!suggestedBindings) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (suggestedBindings->type != XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Register bindings
    if (!RegisterInteractionProfileBindings(suggestedBindings->interactionProfile, 
                                            suggestedBindings->suggestedBindings, 
                                            suggestedBindings->countSuggestedBindings)) {
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    LOGI("Interaction profile bindings registered");
    return XR_SUCCESS;
}

XrResult xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo) {
    if (!attachInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (attachInfo->type != XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Attach action sets
    if (!AttachActionSetsToSession(session, attachInfo->actionSets, attachInfo->countActionSets)) {
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    LOGI("Action sets attached to session");
    return XR_SUCCESS;
}

XrResult xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateBoolean* state) {
    if (!getInfo || !state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (state->type != XR_TYPE_ACTION_STATE_BOOLEAN) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session and action
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> actLock(g_actionMutex);
    auto actIt = g_actions.find(getInfo->action);
    if (actIt == g_actions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get action state from input manager
    bool currentState;
    bool changedSinceLastSync;
    
    if (!GetBooleanActionState(getInfo->action, getInfo->subactionPath, 
                               &currentState, &changedSinceLastSync)) {
        state->currentState = XR_FALSE;
        state->changedSinceLastSync = XR_FALSE;
        return XR_SUCCESS;
    }
    
    state->currentState = currentState ? XR_TRUE : XR_FALSE;
    state->changedSinceLastSync = changedSinceLastSync ? XR_TRUE : XR_FALSE;
    state->lastChangeTime = GetCurrentXrTime();
    
    return XR_SUCCESS;
}

XrResult xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state) {
    if (!getInfo || !state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (state->type != XR_TYPE_ACTION_STATE_FLOAT) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session and action
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> actLock(g_actionMutex);
    auto actIt = g_actions.find(getInfo->action);
    if (actIt == g_actions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get action state from input manager
    float currentState;
    bool changedSinceLastSync;
    
    if (!GetFloatActionState(getInfo->action, getInfo->subactionPath, 
                             &currentState, &changedSinceLastSync)) {
        state->currentState = 0.0f;
        state->changedSinceLastSync = XR_FALSE;
        return XR_SUCCESS;
    }
    
    state->currentState = currentState;
    state->changedSinceLastSync = changedSinceLastSync ? XR_TRUE : XR_FALSE;
    state->lastChangeTime = GetCurrentXrTime();
    
    return XR_SUCCESS;
}

XrResult xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state) {
    if (!getInfo || !state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (state->type != XR_TYPE_ACTION_STATE_VECTOR2F) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session and action
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> actLock(g_actionMutex);
    auto actIt = g_actions.find(getInfo->action);
    if (actIt == g_actions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get action state from input manager
    XrVector2f currentState;
    bool changedSinceLastSync;
    
    if (!GetVector2fActionState(getInfo->action, getInfo->subactionPath, 
                                 &currentState, &changedSinceLastSync)) {
        state->currentState = {0.0f, 0.0f};
        state->changedSinceLastSync = XR_FALSE;
        return XR_SUCCESS;
    }
    
    state->currentState = currentState;
    state->changedSinceLastSync = changedSinceLastSync ? XR_TRUE : XR_FALSE;
    state->lastChangeTime = GetCurrentXrTime();
    
    return XR_SUCCESS;
}

XrResult xrGetActionStatePose(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStatePose* state) {
    if (!getInfo || !state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (state->type != XR_TYPE_ACTION_STATE_POSE) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session and action
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    std::lock_guard<std::mutex> actLock(g_actionMutex);
    auto actIt = g_actions.find(getInfo->action);
    if (actIt == g_actions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get pose state from input manager
    bool isActive;
    
    if (!GetPoseActionState(getInfo->action, getInfo->subactionPath, &isActive)) {
        state->isActive = XR_FALSE;
        return XR_SUCCESS;
    }
    
    state->isActive = isActive ? XR_TRUE : XR_FALSE;
    
    return XR_SUCCESS;
}

XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
    if (!syncInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (syncInfo->type != XR_TYPE_ACTIONS_SYNC_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Sync actions from input devices
    if (!SyncInputActions()) {
        return XR_ERROR_RUNTIME_FAILURE;
    }
    
    return XR_SUCCESS;
}

XrResult xrGetCurrentInteractionProfile(XrSession session, XrPath topLevelUserPath, XrInteractionProfileState* interactionProfile) {
    if (!interactionProfile) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    if (interactionProfile->type != XR_TYPE_INTERACTION_PROFILE_STATE) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate session
    std::lock_guard<std::mutex> sessLock(g_sessionMutex);
    auto sessIt = g_sessions.find(session);
    if (sessIt == g_sessions.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Get current interaction profile
    XrPath profile;
    if (!GetCurrentInteractionProfile(topLevelUserPath, &profile)) {
        interactionProfile->interactionProfile = XR_NULL_PATH;
        return XR_SUCCESS;
    }
    
    interactionProfile->interactionProfile = profile;
    
    return XR_SUCCESS;
}

