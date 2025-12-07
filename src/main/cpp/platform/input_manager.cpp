#include "input_manager.h"
#include "qualcomm/xr2_platform.h"
#include "utils/logger.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <cstring>

struct InteractionProfileBinding {
    XrPath interactionProfile;
    std::unordered_map<XrPath, XrPath> bindings; // action -> input source
};

static std::mutex g_inputMutex;
static std::vector<InteractionProfileBinding> g_profileBindings;
static std::unordered_map<XrSession, std::vector<XrActionSet>> g_sessionActionSets;

bool RegisterInteractionProfileBindings(XrPath interactionProfile, 
                                        const XrActionSuggestedBinding* bindings, 
                                        uint32_t bindingCount) {
    std::lock_guard<std::mutex> lock(g_inputMutex);
    
    InteractionProfileBinding profileBinding;
    profileBinding.interactionProfile = interactionProfile;
    
    for (uint32_t i = 0; i < bindingCount; ++i) {
        profileBinding.bindings[bindings[i].action] = bindings[i].binding;
    }
    
    g_profileBindings.push_back(profileBinding);
    
    LOGI("Registered interaction profile bindings: %llu bindings", bindingCount);
    return true;
}

bool AttachActionSetsToSession(XrSession session, XrActionSet* actionSets, uint32_t count) {
    std::lock_guard<std::mutex> lock(g_inputMutex);
    
    std::vector<XrActionSet> sets;
    for (uint32_t i = 0; i < count; ++i) {
        sets.push_back(actionSets[i]);
    }
    
    g_sessionActionSets[session] = sets;
    
    LOGI("Attached %u action sets to session", count);
    return true;
}

bool GetBooleanActionState(XrAction action, XrPath subactionPath, 
                          bool* currentState, bool* changedSinceLastSync) {
    if (!currentState || !changedSinceLastSync) {
        return false;
    }
    
    // Get state from XR2 input system
    return GetXR2BooleanInput(action, subactionPath, currentState, changedSinceLastSync);
}

bool GetFloatActionState(XrAction action, XrPath subactionPath, 
                        float* currentState, bool* changedSinceLastSync) {
    if (!currentState || !changedSinceLastSync) {
        return false;
    }
    
    // Get state from XR2 input system
    return GetXR2FloatInput(action, subactionPath, currentState, changedSinceLastSync);
}

bool GetVector2fActionState(XrAction action, XrPath subactionPath, 
                           XrVector2f* currentState, bool* changedSinceLastSync) {
    if (!currentState || !changedSinceLastSync) {
        return false;
    }
    
    // Get state from XR2 input system
    return GetXR2Vector2fInput(action, subactionPath, currentState, changedSinceLastSync);
}

bool GetPoseActionState(XrAction action, XrPath subactionPath, bool* isActive) {
    if (!isActive) {
        return false;
    }
    
    // Get pose state from XR2 input system
    return GetXR2PoseInput(action, subactionPath, isActive);
}

bool GetActionPose(XrAction action, XrPath subactionPath, XrTime time, 
                  XrPosef* pose, XrSpaceLocationFlags* locationFlags) {
    if (!pose || !locationFlags) {
        return false;
    }
    
    // Get pose from XR2 tracking system
    return GetXR2ActionPose(action, subactionPath, time, pose, locationFlags);
}

bool GetCurrentInteractionProfile(XrPath topLevelUserPath, XrPath* interactionProfile) {
    if (!interactionProfile) {
        return false;
    }
    
    // Get current interaction profile from XR2
    return GetXR2CurrentInteractionProfile(topLevelUserPath, interactionProfile);
}

bool SyncInputActions() {
    // Sync input from XR2 platform
    return SyncXR2InputActions();
}

XrTime GetCurrentXrTime() {
    // Get current time from XR2 platform
    return GetXR2CurrentTime();
}

