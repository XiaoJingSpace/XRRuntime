#ifndef FRAME_SYNC_H
#define FRAME_SYNC_H

#include <openxr/openxr.h>

// Frame synchronization
bool WaitForNextFrame(XrTime* predictedDisplayTime, XrDuration* predictedDisplayPeriod);

bool BeginFrameRendering();
bool EndFrameRendering();

bool SubmitFrameLayers(const XrCompositionLayerBaseHeader* const* layers, uint32_t layerCount);

#endif // FRAME_SYNC_H

