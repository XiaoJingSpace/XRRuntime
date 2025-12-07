#ifndef QVR_API_WRAPPER_H
#define QVR_API_WRAPPER_H

#include <openxr/openxr.h>
#include "QVRServiceClient.h"
#include "QVRTypes.h"
#include <stdint.h>
#include <stdbool.h>

// QVR Service Client handle (using QVR API's helper type)
typedef qvrservice_client_helper_t* QVRServiceClientHandle;

// QVR API wrapper functions
bool InitializeQVRAPI();
void ShutdownQVRAPI();

// QVR Service Client management
QVRServiceClientHandle QVRServiceClient_CreateWrapper();
void QVRServiceClient_DestroyWrapper(QVRServiceClientHandle handle);

// VR Mode management
QVRSERVICE_VRMODE_STATE QVRServiceClient_GetVRModeWrapper(QVRServiceClientHandle handle);
int QVRServiceClient_StartVRModeWrapper(QVRServiceClientHandle handle);
int QVRServiceClient_StopVRModeWrapper(QVRServiceClientHandle handle);

// Tracking configuration
int QVRServiceClient_GetTrackingModeWrapper(QVRServiceClientHandle handle, 
                                            QVRSERVICE_TRACKING_MODE* mode, 
                                            uint32_t* supportedModes);
int QVRServiceClient_SetTrackingModeWrapper(QVRServiceClientHandle handle, 
                                            QVRSERVICE_TRACKING_MODE mode);

// Head tracking data
int QVRServiceClient_GetHeadTrackingDataWrapper(QVRServiceClientHandle handle, 
                                                 qvrservice_head_tracking_data_t** data);

// Display interrupt configuration
int QVRServiceClient_SetDisplayInterruptConfigWrapper(QVRServiceClientHandle handle, 
                                                       QVRSERVICE_DISP_INTERRUPT_ID interruptId, 
                                                       void* config, uint32_t configSize);

// Display interrupt timestamp
int QVRServiceClient_GetDisplayInterruptTimestampWrapper(QVRServiceClientHandle handle,
                                                          QVRSERVICE_DISP_INTERRUPT_ID interruptId,
                                                          qvrservice_ts_t** ts);

// Time conversion
XrTime QVRTimeToXrTime(uint64_t qvrTime);
uint64_t XrTimeToQVRTime(XrTime xrTime);

// Pose conversion
void QVRPoseToXrPose(const qvrservice_head_tracking_data_t* qvrData, XrPosef* xrPose);

// Get QVR client handle
QVRServiceClientHandle GetQVRClient();

#endif // QVR_API_WRAPPER_H

