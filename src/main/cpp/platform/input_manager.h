#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <openxr/openxr.h>

// Interaction profile bindings
bool RegisterInteractionProfileBindings(XrPath interactionProfile, 
                                        const XrActionSuggestedBinding* bindings, 
                                        uint32_t bindingCount);

bool AttachActionSetsToSession(XrSession session, XrActionSet* actionSets, uint32_t count);

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

