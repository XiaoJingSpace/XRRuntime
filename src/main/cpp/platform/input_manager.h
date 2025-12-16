#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <openxr/openxr.h>
#include <string>

// Forward declaration
typedef struct XrInstance_T* XrInstance;

// Interaction profile bindings
bool RegisterInteractionProfileBindings(XrPath interactionProfile, 
                                        const XrActionSuggestedBinding* bindings, 
                                        uint32_t bindingCount);

bool AttachActionSetsToSession(XrSession session, const XrActionSet* actionSets, uint32_t count);

// Get binding path for an action
XrPath GetActionBindingPath(XrAction action);

// Parsed input path structure
struct ParsedInputPath {
    uint32_t controllerIndex;  // 0 = left, 1 = right
    std::string inputType;     // "trigger", "thumbstick", "button", etc.
    std::string component;      // "value", "click", "touch", etc.
    bool valid;
};

// Parse input path from binding path
ParsedInputPath ParseInputPath(XrPath bindingPath);

// Get current instance (for path conversion)
XrInstance GetCurrentInstance();

// Get path string from path
std::string GetPathString(XrInstance instance, XrPath path);

// Action state queries
bool GetBooleanActionState(XrAction action, XrPath subactionPath, 
                          bool* currentState, bool* changedSinceLastSync);

bool GetFloatActionState(XrAction action, XrPath subactionPath, 
                        float* currentState, bool* changedSinceLastSync);

bool GetVector2fActionState(XrAction action, XrPath subactionPath, 
                           XrVector2f* currentState, bool* changedSinceLastSync);

bool GetPoseActionState(XrAction action, XrPath subactionPath, bool* isActive);

// Action pose queries
bool GetActionPose(XrAction action, XrPath subactionPath, XrTime time, 
                  XrPosef* pose, XrSpaceLocationFlags* locationFlags);

// Interaction profile
bool GetCurrentInteractionProfile(XrPath topLevelUserPath, XrPath* interactionProfile);

// Sync actions
bool SyncInputActions();

// Current time
XrTime GetCurrentXrTime();

#endif // INPUT_MANAGER_H

