#ifndef XR2_PLATFORM_H
#define XR2_PLATFORM_H

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "qvr_api_wrapper.h"

// Custom structure for XR2 graphics properties
// Note: This is not part of standard OpenXR, but used internally for XR2 platform
typedef struct XrGraphicsPropertiesOpenGLESKHR {
    uint32_t maxSwapchainImageWidth;
    uint32_t maxSwapchainImageHeight;
    uint32_t maxSwapchainImageLayers;
} XrGraphicsPropertiesOpenGLESKHR;

// Platform initialization
bool InitializeXR2Platform();
void ShutdownXR2Platform();

// Display management
bool InitializeXR2Display();
void ShutdownXR2Display();
bool GetXR2DisplayProperties(uint32_t* recommendedWidth, uint32_t* recommendedHeight,
                             uint32_t* maxWidth, uint32_t* maxHeight);
bool GetXR2GraphicsProperties(XrGraphicsPropertiesOpenGLESKHR* properties);
bool GetXR2TrackingProperties(XrSystemTrackingProperties* properties);
bool GetXR2ViewFOV(XrFovf* leftEyeFov, XrFovf* rightEyeFov);
bool GetXR2EyeOffsets(XrVector3f* leftEyeOffset, XrVector3f* rightEyeOffset);
bool StartXR2Rendering();
void StopXR2Rendering();

// Tracking
bool InitializeXR2Tracking();
void ShutdownXR2Tracking();
bool GetXR2ViewPoses(XrTime time, XrSpace space, XrView* views, uint32_t count, 
                     XrViewStateFlags* viewStateFlags);
bool LocateXR2ReferenceSpace(XrReferenceSpaceType space, XrReferenceSpaceType baseSpace,
                             XrTime time, XrPosef* pose, XrSpaceLocationFlags* locationFlags);
bool GetXR2StageBounds(XrExtent2Df* bounds);

// Hand tracking
bool InitializeXR2HandTracking();
void ShutdownXR2HandTracking();
bool GetXR2HandPose(uint32_t handIndex, XrTime time, XrPosef* pose, 
                   XrSpaceLocationFlags* locationFlags);

// Eye tracking
bool InitializeXR2EyeTracking();
void ShutdownXR2EyeTracking();
bool GetXR2EyeGaze(XrTime time, XrVector3f* gazeOrigin, XrVector3f* gazeDirection);

// Frame management
bool WaitForXR2NextFrame(XrTime* predictedDisplayTime, XrDuration* predictedDisplayPeriod);
bool BeginXR2FrameRendering();
bool EndXR2FrameRendering();
bool SubmitXR2FrameLayers(const XrCompositionLayerBaseHeader* const* layers, uint32_t layerCount);

// Input
bool GetXR2BooleanInput(XrAction action, XrPath subactionPath, bool* state, bool* changed);
bool GetXR2FloatInput(XrAction action, XrPath subactionPath, float* state, bool* changed);
bool GetXR2Vector2fInput(XrAction action, XrPath subactionPath, XrVector2f* state, bool* changed);
bool GetXR2PoseInput(XrAction action, XrPath subactionPath, bool* isActive);
bool GetXR2ActionPose(XrAction action, XrPath subactionPath, XrTime time, 
                      XrPosef* pose, XrSpaceLocationFlags* locationFlags);
bool GetXR2CurrentInteractionProfile(XrPath topLevelUserPath, XrPath* interactionProfile);
bool SyncXR2InputActions();

// Haptic feedback
bool TriggerXR2HapticFeedback(XrAction action, XrPath subactionPath, 
                              float amplitude, XrDuration duration);

// Time
XrTime GetXR2CurrentTime();

// Power management
bool SetXR2PerformanceLevel(uint32_t level);
bool EnableXR2PowerOptimization(bool enable);

#endif // XR2_PLATFORM_H

