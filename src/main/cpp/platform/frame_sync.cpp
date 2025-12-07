#include "frame_sync.h"
#include "qualcomm/xr2_platform.h"
#include "utils/logger.h"
#include <chrono>

bool WaitForNextFrame(XrTime* predictedDisplayTime, XrDuration* predictedDisplayPeriod) {
    if (!predictedDisplayTime || !predictedDisplayPeriod) {
        return false;
    }
    
    // Wait for next frame from XR2 display
    return WaitForXR2NextFrame(predictedDisplayTime, predictedDisplayPeriod);
}

bool BeginFrameRendering() {
    // Begin frame rendering on XR2
    return BeginXR2FrameRendering();
}

bool EndFrameRendering() {
    // End frame rendering on XR2
    return EndXR2FrameRendering();
}

bool SubmitFrameLayers(const XrCompositionLayerBaseHeader* const* layers, uint32_t layerCount) {
    if (!layers || layerCount == 0) {
        return false;
    }
    
    // Submit layers to XR2 compositor
    return SubmitXR2FrameLayers(layers, layerCount);
}


