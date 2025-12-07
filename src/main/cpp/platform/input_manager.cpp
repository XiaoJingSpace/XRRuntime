#include "input_manager.h"
#include "qualcomm/xr2_platform.h"
#include "openxr/openxr_api.h"
#include "utils/logger.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>

struct InteractionProfileBinding {
    XrPath interactionProfile;
    std::unordered_map<XrPath, XrPath> bindings; // action -> input source (binding path)
};

// Action to binding path mapping
static std::unordered_map<XrAction, XrPath> g_actionToBindingPath;

// Forward declarations
extern "C" XrResult xrPathToString(XrInstance instance, XrPath path, uint32_t bufferCapacityInput, 
                                   uint32_t* bufferCountOutput, char* buffer);

// Get current instance (needed for path conversion)
extern std::mutex g_instanceMutex;
extern std::unordered_map<XrInstance, std::shared_ptr<struct XRInstance>> g_instances;

XrInstance GetCurrentInstance() {
    // Try to get any instance from the registry
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    if (!g_instances.empty()) {
        return g_instances.begin()->first;
    }
    return XR_NULL_HANDLE;
}

static std::mutex g_inputMutex;
static std::vector<InteractionProfileBinding> g_profileBindings;
static std::unordered_map<XrSession, std::vector<XrActionSet>> g_sessionActionSets;
static XrInstance g_currentInstance = XR_NULL_HANDLE;

bool RegisterInteractionProfileBindings(XrPath interactionProfile, 
                                        const XrActionSuggestedBinding* bindings, 
                                        uint32_t bindingCount) {
    std::lock_guard<std::mutex> lock(g_inputMutex);
    
    InteractionProfileBinding profileBinding;
    profileBinding.interactionProfile = interactionProfile;
    
    for (uint32_t i = 0; i < bindingCount; ++i) {
        profileBinding.bindings[bindings[i].action] = bindings[i].binding;
        // Store action to binding path mapping
        g_actionToBindingPath[bindings[i].action] = bindings[i].binding;
    }
    
    g_profileBindings.push_back(profileBinding);
    
    LOGI("Registered interaction profile bindings: %llu bindings", bindingCount);
    return true;
}

// Parse binding path to extract controller index and input information
// Example: "/user/hand/left/input/trigger/value" -> controller=0 (left), input="trigger", component="value"
struct ParsedInputPath {
    uint32_t controllerIndex;  // 0 = left, 1 = right
    std::string inputType;     // "trigger", "thumbstick", "button", etc.
    std::string component;      // "value", "click", "touch", etc.
    bool valid;
};

// Get path string by converting path to string using xrPathToString
std::string GetPathString(XrInstance instance, XrPath path) {
    if (instance == XR_NULL_HANDLE || path == XR_NULL_PATH) {
        return "";
    }
    
    char buffer[256] = {0};
    uint32_t bufferCount = 0;
    
    XrResult result = xrPathToString(instance, path, sizeof(buffer), &bufferCount, buffer);
    if (result == XR_SUCCESS && bufferCount > 0 && bufferCount <= sizeof(buffer)) {
        return std::string(buffer);
    }
    
    return "";
}

// Parse input path - exported for use in xr2_platform
ParsedInputPath ParseInputPath(XrPath bindingPath) {
    ParsedInputPath result = {0, "", "", false};
    
    if (bindingPath == XR_NULL_PATH) {
        return result;
    }
    
    // Get instance for path conversion
    XrInstance instance = GetCurrentInstance();
    if (instance == XR_NULL_HANDLE) {
        return result;
    }
    
    // Get path string
    std::string pathString = GetPathString(instance, bindingPath);
    if (pathString.empty()) {
        return result;
    }
    
    // Parse path: /user/hand/{left|right}/input/{type}/{component}
    // Example: "/user/hand/left/input/trigger/value"
    std::istringstream iss(pathString);
    std::string segment;
    std::vector<std::string> segments;
    
    while (std::getline(iss, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    
    if (segments.size() >= 5) {
        // segments[0] = "user"
        // segments[1] = "hand"
        // segments[2] = "left" or "right"
        // segments[3] = "input"
        // segments[4] = input type
        // segments[5] = component (if exists)
        
        if (segments[0] == "user" && segments[1] == "hand" && segments[3] == "input") {
            // Determine controller index
            if (segments[2] == "left") {
                result.controllerIndex = 0;
            } else if (segments[2] == "right") {
                result.controllerIndex = 1;
            } else {
                return result; // Invalid hand
            }
            
            result.inputType = segments[4];
            if (segments.size() > 5) {
                result.component = segments[5];
            } else {
                result.component = "value"; // Default component
            }
            
            result.valid = true;
        }
    }
    
    return result;
}

// Helper function to register path string (called from xrStringToPath)
void RegisterPathString(XrPath path, const char* pathString) {
    static std::mutex pathMutex;
    static std::unordered_map<XrPath, std::string> pathToStringMap;
    
    std::lock_guard<std::mutex> lock(pathMutex);
    pathToStringMap[path] = pathString;
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

XrPath GetActionBindingPath(XrAction action) {
    std::lock_guard<std::mutex> lock(g_inputMutex);
    auto it = g_actionToBindingPath.find(action);
    if (it != g_actionToBindingPath.end()) {
        return it->second;
    }
    return XR_NULL_PATH;
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

