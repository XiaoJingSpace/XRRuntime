//=============================================================================
// FILE: svrApiCore.cpp
//
//                  Copyright (c) 2016 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================
#include <errno.h>
#include <fcntl.h>
#include <jni.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <mutex>

#include <jni.h>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "sxrApi.h"
#include "svrCpuTimer.h"
#include "svrProfile.h"
#include "svrRenderExt.h"
#include "svrUtil.h"

#include "svrConfig.h"

#include "private/svrApiCore.h"
#include "private/svrApiDebugServer.h"
#include "private/svrApiHelper.h"
#include "private/svrApiPredictiveSensor.h"
#include "private/svrApiTimeWarp.h"

#ifdef ENABLE_CAMERA
#include "svrCameraManager.h"
#endif

#ifdef ENABLE_REMOTE_XR_RENDERING
#include "SxrServiceClientManager.h"
#endif

#define QVRSERVICE_SDK_CONFIG_FILE  "sdk-config-file"

using namespace Svr;

// Surface Properties
VAR(int, gEyeBufferWidth, 1024, kVariableNonpersistent);            //Value returned as recommended eye buffer width (pixels) in sxrDeviceInfo
VAR(int, gEyeBufferHeight, 1024, kVariableNonpersistent);           //Value returned as recommended eye buffer height (pixels) in sxrDeviceInfo

VAR(float, gFrustumDisplayWidth, 0, kVariableNonpersistent);          //Value returned as recommended frustum display width (millimeters)
VAR(float, gFrustumDisplayHeight, 0, kVariableNonpersistent);         //Value returned as recommended frustum display height (millimeters)

VAR(float, gDisplayWidth, 0, kVariableNonpersistent);                 //Value returned as recommended display width (millimeters)
VAR(float, gDisplayHeight, 0, kVariableNonpersistent);                //Value returned as recommended display height (millimeters)

// Field-of-view: Calculated from frustum if not set in config
VAR(float, gEyeBufferFovX, 0.0f, kVariableNonpersistent);          //Value returned as recommended FOV X in sxrDeviceInfo
VAR(float, gEyeBufferFovY, 0.0f, kVariableNonpersistent);          //Value returned as recommended FOV Y in sxrDeviceInfo

// Projection Properties
VAR(float, gFrustum_Convergence, 0.0f, kVariableNonpersistent);     //Value returned as recommended eye convergence in sxrDeviceInfo
VAR(float, gFrustum_Pitch, 0.0f, kVariableNonpersistent);           //Value returned as recommended eye pitch in sxrDeviceInfo

VAR(float, gLeftFrustum_Near, 0.0508, kVariableNonpersistent);      //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gLeftFrustum_Far, 100.0, kVariableNonpersistent);        //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gLeftFrustum_Left, -0.031, kVariableNonpersistent);      //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gLeftFrustum_Right, 0.031, kVariableNonpersistent);      //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gLeftFrustum_Top, 0.031, kVariableNonpersistent);        //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gLeftFrustum_Bottom, -0.031, kVariableNonpersistent);    //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gLeftFrustum_PositionX, -0.032f, kVariableNonpersistent);    //Value returned as recommended view frustum position in sxrDeviceInfo
VAR(float, gLeftFrustum_PositionY, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum position in sxrDeviceInfo
VAR(float, gLeftFrustum_PositionZ, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum position in sxrDeviceInfo
VAR(float, gLeftFrustum_RotationX, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo
VAR(float, gLeftFrustum_RotationY, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo
VAR(float, gLeftFrustum_RotationZ, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo
VAR(float, gLeftFrustum_RotationW, 1.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo

VAR(float, gRightFrustum_Near, 0.0508, kVariableNonpersistent);     //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gRightFrustum_Far, 100.0, kVariableNonpersistent);       //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gRightFrustum_Left, -0.031, kVariableNonpersistent);     //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gRightFrustum_Right, 0.031, kVariableNonpersistent);     //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gRightFrustum_Top, 0.031, kVariableNonpersistent);       //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gRightFrustum_Bottom, -0.031, kVariableNonpersistent);   //Value returned as recommended view frustum size in sxrDeviceInfo
VAR(float, gRightFrustum_PositionX, 0.032f, kVariableNonpersistent);    //Value returned as recommended view frustum position in sxrDeviceInfo
VAR(float, gRightFrustum_PositionY, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum position in sxrDeviceInfo
VAR(float, gRightFrustum_PositionZ, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum position in sxrDeviceInfo
VAR(float, gRightFrustum_RotationX, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo
VAR(float, gRightFrustum_RotationY, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo
VAR(float, gRightFrustum_RotationZ, 0.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo
VAR(float, gRightFrustum_RotationW, 1.0f, kVariableNonpersistent);    //Value returned as recommended view frustum rotation in sxrDeviceInfo

// Foveation Properties
VAR(float, gLowFoveationArea, 0.0f, kVariableNonpersistent);        //Value returned as recommended low foveation area in sxrDeviceInfo
VAR(float, gLowFoveationGainX, 2.0f, kVariableNonpersistent);       //Value returned as recommended low foveation gain x in sxrDeviceInfo
VAR(float, gLowFoveationGainY, 2.0f, kVariableNonpersistent);       //Value returned as recommended low foveation gain y in sxrDeviceInfo
VAR(float, gLowFoveationMinimum, 0.125f, kVariableNonpersistent);   //Value returned as recommended low foveation minimum in sxrDeviceInfo

VAR(float, gMedFoveationArea, 1.0f, kVariableNonpersistent);        //Value returned as recommended med foveation area in sxrDeviceInfo
VAR(float, gMedFoveationGainX, 3.0f, kVariableNonpersistent);       //Value returned as recommended med foveation gain x in sxrDeviceInfo
VAR(float, gMedFoveationGainY, 3.0f, kVariableNonpersistent);       //Value returned as recommended med foveation gain y in sxrDeviceInfo
VAR(float, gMedFoveationMinimum, 0.125f, kVariableNonpersistent);   //Value returned as recommended med foveation minimum in sxrDeviceInfo

VAR(float, gHighFoveationArea, 2.0f, kVariableNonpersistent);       //Value returned as recommended high foveation area in sxrDeviceInfo
VAR(float, gHighFoveationGainX, 4.0f, kVariableNonpersistent);      //Value returned as recommended high foveation gain x in sxrDeviceInfo
VAR(float, gHighFoveationGainY, 4.0f, kVariableNonpersistent);      //Value returned as recommended high foveation gain y in sxrDeviceInfo
VAR(float, gHighFoveationMinimum, 0.125f, kVariableNonpersistent);  //Value returned as recommended high foveation minimum in sxrDeviceInfo

// Tracking Camera. For now they are configuration items.  Eventually they
// will be returned from QVR.  Until that time, use the configuration settings
VAR(float, gCalibration_X_0, 0.00505392f, kVariableNonpersistent);
VAR(float, gCalibration_X_1, -0.926172f, kVariableNonpersistent);
VAR(float, gCalibration_X_2, -0.377068f, kVariableNonpersistent);
VAR(float, gCalibration_X_3, -0.00667208f, kVariableNonpersistent);

VAR(float, gCalibration_Y_0, -0.903837f, kVariableNonpersistent);
VAR(float, gCalibration_Y_1, -0.165562f, kVariableNonpersistent);
VAR(float, gCalibration_Y_2, 0.394548f, kVariableNonpersistent);
VAR(float, gCalibration_Y_3, 0.0630108f, kVariableNonpersistent);

VAR(float, gCalibration_Z_0, -0.427847f, kVariableNonpersistent);
VAR(float, gCalibration_Z_1, 0.338814f, kVariableNonpersistent);
VAR(float, gCalibration_Z_2, -0.837945f, kVariableNonpersistent);
VAR(float, gCalibration_Z_3, -0.0338611f, kVariableNonpersistent);

VAR(float, gPrincipalPoint_0, 331.91989f, kVariableNonpersistent);
VAR(float, gPrincipalPoint_1, 241.84999f, kVariableNonpersistent);

VAR(float, gFocalLength_0, 268.08765f, kVariableNonpersistent);
VAR(float, gFocalLength_1, 268.08765f, kVariableNonpersistent);

VAR(float, gRadialDistortion_0, 0.084730856f, kVariableNonpersistent);
VAR(float, gRadialDistortion_1, -0.058135655f, kVariableNonpersistent);
VAR(float, gRadialDistortion_2, 0.23780872f, kVariableNonpersistent);
VAR(float, gRadialDistortion_3, -0.12837055f, kVariableNonpersistent);
VAR(float, gRadialDistortion_4, 0.0f, kVariableNonpersistent);
VAR(float, gRadialDistortion_5, 0.0f, kVariableNonpersistent);
VAR(float, gRadialDistortion_6, 0.0f, kVariableNonpersistent);
VAR(float, gRadialDistortion_7, 0.0f, kVariableNonpersistent);

// TimeWarp Properties
VAR(bool, gEnableTimeWarp, true, kVariableNonpersistent);           //Override to disable TimeWarp
VAR(bool, gDisableReprojection, false, kVariableNonpersistent);     //Override to disable reprojection
VAR(bool, gDisablePredictedTime, false, kVariableNonpersistent);    //Forces sxrGetPredictedDisplayTime to return 0.0
VAR(int,  gRenderThreadCore, 3, kVariableNonpersistent);            //Core id to set render thread affinity for (-1 disables affinity), ignored if the QVR Perf module is active.
VAR(bool, gEnableRenderThreadFifo, false, kVariableNonpersistent);  //Enable/disable setting SCHED_FIFO scheduling policy on the render thread thread, ignored if the QVR Perf module is active.
VAR(int,  gForceMinVsync, 0, kVariableNonpersistent);               //Override for sxrFrameParams minVsync option (0:override disabled, 1 or 2 forced value)
VAR(bool, gUseVSyncCallback, false, kVariableNonpersistent);        //Override for using the vsync callback, if set to false then check gUseLinePtr
VAR(bool, gUseLinePtr, true, kVariableNonpersistent);               //Override for using the linePtr interrupt, if set to false Choreographer will be used instead
VAR(int,  gDisplayRefreshRateHz, 0, kVariableNonpersistent);        //Override for android display refresh rate (Hz)

// Display Setting
VAR(float, gTimeToHalfExposure, 8.33f, kVariableNonpersistent);     // Time (milliseconds) to get to Half Exposure on the display. This usually T/2, but this property can be used to adjust for OLED vs LCD, getting to 3T/4, additional display delay time, etc.
VAR(float, gTimeToMidEyeWarp, 4.16f, kVariableNonpersistent);       // Time (milliseconds) between warping each eye.

// Heuristic Predicted Time
VAR(bool, gHeuristicPredictedTime, false, kVariableNonpersistent);  // Whether to use a heuristic predicted time
VAR(int, gNumHeuristicEntries, 25, kVariableNonpersistent);         // How many entries to average to get heuristic predicted time
VAR(float, gHeuristicOffset, 0.0, kVariableNonpersistent);         // Offset added to the heuristic predicted time

//Power brackets for performance levels
VAR(bool, gUseQvrPerfModule, true, kVariableNonpersistent);			//Enable/disable the QVR Performance module.  If active all thread affinities, priorities and HW clocks will be based on the QVR perf module configuration rather than the SDK configuration

VAR(int, gForceCpuLevel, -1, kVariableNonpersistent);               //Override to force CPU performance level (-1: app defined, 0:system defined/off, 1/2/3 for min,medium,max)
VAR(int, gCpuLvl1Min, 30, kVariableNonpersistent);                  //Lower CPU frequency (percentage) bound for min performance level, ignored if the QVR Perf module is active.
VAR(int, gCpuLvl1Max, 50, kVariableNonpersistent);                  //Upper CPU frequency (percentage) bound for min performance level, ignored if the QVR Perf module is active.
VAR(int, gCpuLvl2Min, 51, kVariableNonpersistent);                  //Lower CPU frequency (percentage) bound for medium performance level, ignored if the QVR Perf module is active.
VAR(int, gCpuLvl2Max, 80, kVariableNonpersistent);                  //Upper CPU frequency (percentage) bound for medium performance level, ignored if the QVR Perf module is active.
VAR(int, gCpuLvl3Min, 81, kVariableNonpersistent);                  //Lower CPU frequency (percentage) bound for max performance level, ignored if the QVR Perf module is active.
VAR(int, gCpuLvl3Max, 100, kVariableNonpersistent);                 //Upper CPU frequency (percentage) bound for max performance level  , ignored if the QVR Perf module is active.

VAR(int, gForceGpuLevel, -1, kVariableNonpersistent);               //Override to force GPU performance level (-1: app defined, 0:system defined/off, 1/2/3 for min,medium,max)
VAR(int, gGpuLvl1Min, 30, kVariableNonpersistent);                  //Lower GPU frequency (percentage) bound for min performance level, ignored if the QVR Perf module is active.
VAR(int, gGpuLvl1Max, 50, kVariableNonpersistent);                  //Upper GPU frequency (percentage) bound for min performance level, ignored if the QVR Perf module is active.
VAR(int, gGpuLvl2Min, 51, kVariableNonpersistent);                  //Lower GPU frequency (percentage) bound for medium performance level, ignored if the QVR Perf module is active.
VAR(int, gGpuLvl2Max, 80, kVariableNonpersistent);                  //Upper GPU frequency (percentage) bound for medium performance level, ignored if the QVR Perf module is active.
VAR(int, gGpuLvl3Min, 81, kVariableNonpersistent);                  //Lower GPU frequency (percentage) bound for max performance level, ignored if the QVR Perf module is active.
VAR(int, gGpuLvl3Max, 100, kVariableNonpersistent);                 //Upper GPU frequency (percentage) bound for max performance level, ignored if the QVR Perf module is active.

//Tracking overrides
VAR(int, gForceTrackingMode, 0, kVariableNonpersistent);            //Force a specific tracking mode 1 = rotational 3 = rotational & positional
VAR(bool, gDisableTrackingRecenter, false, kVariableNonpersistent); //Disable tracking recenter
VAR(bool, gUseMagneticRotationFlag, true, kVariableNonpersistent);  //If using roational data only, use version that is magnetically corrected

VAR(bool, gLifecycleSuspendEnabled, false, kVariableNonpersistent); //Enable Begin/EndXr to control XR suspend/resume
VAR(bool, gProximitySuspendEnabled, true, kVariableNonpersistent);  //Enable proximity sensor to control XR suspend/resume

//Log options
VAR(float, gLogLinePtrDelay, 0.0, kVariableNonpersistent);          //Log line ptr delays longer greater than this value (0.0 = disabled)
VAR(bool, gLogSubmitFps, false, kVariableNonpersistent);            //Enables output of submit FPS to LogCat

VAR(bool, gLogPoseVelocity, false, kVariableNonpersistent);         //Log out the tracking velocity
VAR(float, gMaxAngVel, 450.0f, kVariableNonpersistent);             //Detected angular velocity larger than this value will be considered an error in the tracking system (degrees / sec)
VAR(float, gMaxLinearVel, 6.0f, kVariableNonpersistent);            //Detected linear/translational velocity larger than this value will be considered an error in the tracking system (meters / sec)

VAR(bool, gLogSubmitFrame, false, kVariableNonpersistent);          //Log svrSubmitFrame() parameters

//Debug Server options
VAR(bool, gEnableDebugServer, false, kVariableNonpersistent);       //Enables a very basic json-rpc server for interacting with vr while running

//Debug toggles
VAR(bool, gDisableFrameSubmit, false, kVariableNonpersistent);      //Debug flag that will prevent the eye buffer render thread from submitted frames to time warp

//Controller options
VAR(char*, gControllerService, " ", kVariableNonpersistent);
VAR(int, gControllerRingBufferSize, 80, kVariableNonpersistent);

EXTERN_VAR(float, gWarpMeshMinX);
EXTERN_VAR(float, gWarpMeshMaxX);
EXTERN_VAR(float, gWarpMeshMinY);
EXTERN_VAR(float, gWarpMeshMaxY);

EXTERN_VAR(float, gLensScale); // Depricated
EXTERN_VAR(float, gLensScaleX);
EXTERN_VAR(float, gLensScaleY);

EXTERN_VAR(float, gMeshDiscardUV);

EXTERN_VAR(float, gScreenWidthMM);

EXTERN_VAR(int, gWarpMeshType);
EXTERN_VAR(bool, gEnableWarpThreadFifo);
EXTERN_VAR(bool, gLogRawSensorData);
EXTERN_VAR(bool, gLogVSyncData);
EXTERN_VAR(bool, gLogPrediction);

EXTERN_VAR(float, gMaxPredictedTime);
EXTERN_VAR(bool, gLogMaxPredictedTime);

EXTERN_VAR(float, gSensorOrientationCorrectX);   //Adjustment if sensors are physically rotated (degrees)
EXTERN_VAR(float, gSensorOrientationCorrectY);   //Adjustment if sensors are physically rotated (degrees)
EXTERN_VAR(float, gSensorOrientationCorrectZ);   //Adjustment if sensors are physically rotated (degrees)
EXTERN_VAR(int, gSensorHomePosition);   // Base device configuration. 0 = Landscape Left; 1 = Landscape Right

EXTERN_VAR(float, gSensorHeadOffsetX);       //Adjustment for device physical distance from head (meters)
EXTERN_VAR(float, gSensorHeadOffsetY);       //Adjustment for device physical distance from head (meters)
EXTERN_VAR(float, gSensorHeadOffsetZ);     //Adjustment for device physical distance from head (meters)


#ifdef ENABLE_MOTION_VECTORS
EXTERN_VAR(bool, gEnableMotionVectors);
EXTERN_VAR(bool, gForceAppEnableMotionVectors);
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_CAMERA
bool gEnableCameraLayer = false;
extern SvrCameraManager gSvrCameraManager; // grabs & rectifies camera frames
#endif

/* RVR Specific properites*/
VAR(bool, gEnableRVR, false, kVariableNonpersistent);                   // Override to run in remote render mode
VAR(bool, gEnableRVRLagacy, true, kVariableNonpersistent);                   // Override to run in remote render mode

#ifdef ENABLE_REMOTE_XR_RENDERING
/* Streaming */
VAR(int, gRVRMode, 2, kVariableNonpersistent);                          // 1: UDP, 2: USB
VAR(bool, gRVRPayloadInRTP, false, kVariableNonpersistent);             // Override to enable network/legacy mode
VAR(int, gRVRMTU, 51200, kVariableNonpersistent);                       // Maximum transmission unit size in bytes (eg. usb: 51200 bytes, n/w : 1500 bytes)

/* render control*/
VAR(float, gRVRRenderFov, 80.0f, kVariableNonpersistent);               // Render fov. Set 0 to use display fov. Over render is recommended due to addional latency
VAR(float, gRVRRenderFovX, 0.0f, kVariableNonpersistent);               // Render horizontal fov. Set 0 to use display fov. Over render is recommended due to addional latency
VAR(float, gRVRRenderFovY, 0.0f, kVariableNonpersistent);               // Render vertical fov. Set 0 to use display fov. Over render is recommended due to addional latency
VAR(float, gRVRFPS, 75.0f, kVariableNonpersistent);                     // Display refersh rate of Hmd display
VAR(int, gRVRSynchronizeToHMDVSync, 1, kVariableNonpersistent);         // Debug setting. PLL to synchronize render rate to Hmd display refresh rate. 0: disable

/* video encoder */
VAR(char*, gRVRCodec, "hevc", kVariableNonpersistent);                  // Video encoder (supported types h264, hevc, nv12, rle(exerimental), rle2(exerimental))
VAR(int, gRVRAvgBitrateMbPS, 30, kVariableNonpersistent);               // Average video encoder bitrate in MbitsPerSec
VAR(int, gRVRPeakBitrateMbPS, 60, kVariableNonpersistent);              // Peak video encoder bitrate in MbitsPerSec
VAR(int, gRVRGOPLength, 75, kVariableNonpersistent);                    // GOP size in frames for video encoder
VAR(int, gRVRSlicesPerFrame, 1, kVariableNonpersistent);                // Slices per frame in encoded picture
VAR(int, gRVRYCoCg, 0, kVariableNonpersistent);                         // Override to enable YCoCg input to video encoder

/* video encoder performance */
VAR(int, gRVRPerfMode, 1, kVariableNonpersistent);                      // Encoder performance 0:default, 1:static fps based, 2:dynamic per frame fps, 4:turbo
VAR(float, gRVRF0, 1.5f, kVariableNonpersistent);                       // Performce multiplier (tuned as per platform)
VAR(float, gRVRF1, 1.5f, kVariableNonpersistent);                       // Performce multiplier (tuned as per platform)

/* fix foveation*/
VAR(int, gRVRStreamingWidth, 0, kVariableNonpersistent);                // Fix foveation downscaled width, 0 to disable
VAR(int, gRVRStreamingHeight, 0, kVariableNonpersistent);               // Fix foveation downscaled height, 0 to disable
VAR(float, gRVRFoveaAngle, 40.0f, kVariableNonpersistent);              // Fix foveation fovea region area

/* audio */
VAR(bool, gRVRAudio, true, kVariableNonpersistent);                     // Override to disable audio

/* local display */
VAR(int, gRVRDisplayFlags, 0, kVariableNonpersistent);                  // mask to show eye texture on host display 0: none , 1: left eye, 2: right eye

/* DFS for APR */
VAR(int, gRVRDefaultAtwMode, 0, kVariableNonpersistent);                // ATW mode 0:3DOF, 1: HarmonicMean, 2: APAPR
VAR(bool, gRVRCalculateDepthDFS, false, kVariableNonpersistent);        // Override to enable calculate depth using dfs
VAR(float, gRVRIPD, 0.064f, kVariableNonpersistent);                    // Value of IPD in meters
VAR(int, gRVRKernelSize, 5, kVariableNonpersistent);                    // Don't override
VAR(int, gRVRMaxDisparity, 24, kVariableNonpersistent);                 // Don't override
VAR(int, gRVRLocalSizeX, 32, kVariableNonpersistent);                   // Don't override
VAR(int, gRVRLocalSizeY, 32, kVariableNonpersistent);                   // Don't override
VAR(int, gRVRDownScaleFactor, 5, kVariableNonpersistent);               // Don't override
VAR(float, gRVRPositionScaleZ, 1.0f, kVariableNonpersistent);           // Don't override
VAR(float, gRVRFarClipPlane, -1.0f, kVariableNonpersistent);            // Don't override

/* reserved for future use*/
VAR(bool, gRVREnableHeartbeat, false, kVariableNonpersistent);          // Don't override
VAR(bool, gRVRWaitForHMD, true, kVariableNonpersistent);                // Don't override. Wait for USB connection
VAR(int, gRVRUsePoseSocket, 0, kVariableNonpersistent);                 // Don't override. Reserved for future use
VAR(int, gRVRFlags, 0, kVariableNonpersistent);                    // Don't override. Reserved for future use

SxrServiceClientManager *gRVRManager = nullptr;
static bool gRVRManagerInit = false;
static std::mutex gRVRManagerInitMtx;
static bool gRVRUseQVRServicePose = false;
#endif //ENABLE_REMOTE_XR_RENDERING

// BEGIN SxrPresentation region
#ifdef ENABLE_XR_CASTING
VAR(bool, gEnableVRCasting, false, kVariableNonpersistent); // Debug flag to force enable VR Casting feature
VAR(int,  gPresentationThreadCore, 6, kVariableNonpersistent);            //Core id to set presentation thread affinity for (-1 disables affinity), ignored if the QVR Perf module is active.
VAR(bool, gEnablePresentationThreadFifo, true, kVariableNonpersistent);   //Enable/disable setting SCHED_FIFO scheduling policy on the presentation thread (must be true if gBusyWait is false to avoid tearing), ignored if the QVR Perf module is active.
#endif//ENABLE_XR_CASTING
// END SxrPresentation region

/* 3DR Specific properites*/
VAR(bool, gEnable3DR, false, kVariableNonpersistent);                   // Override to initialize 3DR subsystem

/* Anchors Specific properites*/
VAR(bool, gEnableAnchors, false, kVariableNonpersistent);               // Override to initialize Anchors subsystem

int gFifoPriorityRender = 96;
int gNormalPriorityRender = 0;

int gRecenterTransition = 1000;

// Offset from the time Android has to what QVR has
int64_t gQTimeToAndroidBoot = 0LL;

// Want to timeline things
uint64_t gVrStartTimeNano = 0;

// Variables needed for heuristic predicted time
float *gpHeuristicPredictData = NULL;
int gHeuristicWriteIndx = 0;

qvrsync_ctrl_t *m_eyeSyncCtrl = NULL;

qvrservice_ring_buffer_desc_t m_poseBufferDesc;

XrPointCloudQTI *m_SlamPointCloud = NULL;

// Need this common file from svrApiTimeWarp.cpp
double L_MilliSecondsToNextVSync();
float L_MilliSecondsSinceVrStart();

void L_SetQvrTransform();

namespace Svr
{
    SvrAppContext* gAppContext = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

extern "C"
{
    void Java_com_qualcomm_sxrapi_SxrApi_nativeVsync(JNIEnv *jni, jclass clazz, jlong frameTimeNanos)
    {
        if (gAppContext != NULL && gAppContext->modeContext != NULL)
        {
            const double periodNano = 1e9 / gAppContext->deviceInfo.displayRefreshRateHz;

            pthread_mutex_lock(&gAppContext->modeContext->vsyncMutex);

            uint64_t vsyncTimeStamp = frameTimeNanos;

            if (gAppContext->modeContext->vsyncTimeNano == 0)
            {
                //Don't count the first time through
                gAppContext->modeContext->vsyncTimeNano = vsyncTimeStamp;
                gAppContext->modeContext->vsyncCount = 1;
            }
            else
            {
                unsigned int nVsync = floor(0.5 + ((double)(vsyncTimeStamp - gAppContext->modeContext->vsyncTimeNano) / periodNano));
                gAppContext->modeContext->vsyncCount += nVsync;
                gAppContext->modeContext->vsyncTimeNano = vsyncTimeStamp;
            }

            pthread_mutex_unlock(&gAppContext->modeContext->vsyncMutex);

        }   // gAppContext != NULL
    }
}

//-----------------------------------------------------------------------------
void L_LogQvrError(const char *pSource, int32_t qvrReturn)
//-----------------------------------------------------------------------------
{
	switch (qvrReturn)
	{
	case QVR_ERROR:
		LOGE("Error from %s: QVR_ERROR", pSource);
		break;
	case QVR_CALLBACK_NOT_SUPPORTED:
		LOGE("Error from %s: QVR_CALLBACK_NOT_SUPPORTED", pSource);
		break;
	case QVR_API_NOT_SUPPORTED:
		LOGE("Error from %s: QVR_API_NOT_SUPPORTED", pSource);
		break;
	case QVR_INVALID_PARAM:
		LOGE("Error from %s: QVR_INVALID_PARAM", pSource);
		break;
	default:
		LOGE("Error from %s: Unknown = %d", pSource, qvrReturn);
		break;
	}
}

//-----------------------------------------------------------------------------
static inline sxrThermalLevel QvrThermalToSvrThermal(uint32_t qvrThermal)
//-----------------------------------------------------------------------------
{
	switch (qvrThermal)
	{
	case TEMP_SAFE:
		return kSafe;
	case TEMP_LEVEL_1:
		return kLevel1;
	case TEMP_LEVEL_2:
		return kLevel2;
	case TEMP_LEVEL_3:
		return kLevel3;
	case TEMP_CRITICAL:
		return kCritical;
	default:
		return kSafe;
	}
}

//-----------------------------------------------------------------------------
static inline QVRSERVICE_PERF_LEVEL SvrPerfLevelToQvrPerfLevel(sxrPerfLevel svrLevel)
//-----------------------------------------------------------------------------
{
	switch (svrLevel)
	{
	case kPerfSystem:
		return PERF_LEVEL_DEFAULT;
	case kPerfMinimum:
		return PERF_LEVEL_1;
	case kPerfMedium:
		return PERF_LEVEL_2;
	case kPerfMaximum:
		return PERF_LEVEL_3;
	default:
		return PERF_LEVEL_DEFAULT;
	}
}

//-----------------------------------------------------------------------------
static void qvrClientStatusCallback(void *pCtx, QVRSERVICE_CLIENT_STATUS status, uint32_t arg1, uint32_t arg2)
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->modeContext == NULL)
    {
        return;
    }

    if (gAppContext->modeContext->eventManager != NULL)
    {
        svrEventQueue* pQueue = gAppContext->modeContext->eventManager->AcquireEventQueue();
        if (pQueue != NULL)
        {
            sxrEventData eventData;

            switch (status)
            {
            case STATUS_DISCONNECTED:
                break;
            case STATUS_STATE_CHANGED:
                //uint32_t newState = arg1;
                //uint32_t prevState = arg2;
                break;
            case STATUS_SENSOR_ERROR:
                pQueue->SubmitEvent(kEventSensorError, eventData);
                break;
            default:
                break;
            }
        }
        else
        {
            LOGE("qvrClientStatusCallback, failed to acquire event queue");
        }
    }
}

//-----------------------------------------------------------------------------
void qvrClientThermalNotificationCallback(void *pCtx, QVRSERVICE_CLIENT_NOTIFICATION notification, void *payload, uint32_t payload_length)
//-----------------------------------------------------------------------------
{
	if (payload_length != sizeof(qvrservice_therm_notify_payload_t))
	{
		LOGE("ThermalNotifcation payload length %d doesn't match expected payload length of %d", payload_length, (int)sizeof(qvrservice_therm_notify_payload_t));
		return;
	}

	if (gAppContext == NULL || gAppContext->modeContext == NULL)
	{
		return;
	}

	if (gAppContext->modeContext->eventManager != NULL)
	{
		sxrEventData eventData;

		svrEventQueue* pQueue = gAppContext->modeContext->eventManager->AcquireEventQueue();
		if (pQueue != NULL)
		{
			qvrservice_therm_notify_payload_t* pThermalPayload = (qvrservice_therm_notify_payload_t*)payload;
			switch (pThermalPayload->hw_type)
			{
			case HW_TYPE_CPU:
				eventData.thermal.zone = kCpu;
				break;
			case HW_TYPE_GPU:
				eventData.thermal.zone = kGpu;
				break;
			case HW_TYPE_SKIN:
				eventData.thermal.zone = kSkin;
                break;
			default:
				LOGE("qvrClientThermalNotificationCallback: Unknown zone %d", pThermalPayload->hw_type);
				eventData.thermal.zone = kNumThermalZones;
			}

			switch (pThermalPayload->temp_level)
			{
			case TEMP_SAFE:
				eventData.thermal.level = kSafe;
				break;
			case TEMP_LEVEL_1:
				eventData.thermal.level = kLevel1;
				break;
			case TEMP_LEVEL_2:
				eventData.thermal.level = kLevel2;
				break;
			case TEMP_LEVEL_3:
				eventData.thermal.level = kLevel3;
				break;
			case TEMP_CRITICAL:
				eventData.thermal.level = kCritical;
				break;
			}

			pQueue->SubmitEvent(kEventThermal, eventData);
		}
	}
}

//-----------------------------------------------------------------------------
void qvrClientProximityNotificationCallback(void *pCtx, QVRSERVICE_CLIENT_NOTIFICATION notification, void *payload, uint32_t payload_length)
//-----------------------------------------------------------------------------
{
    if (payload_length != sizeof(qvrservice_proximity_notify_payload_t))
    {
        LOGE("ProximityNotifcation payload length %d doesn't match expected payload length of %d", payload_length, (int)sizeof(qvrservice_proximity_notify_payload_t));
        return;
    }

    if (gAppContext == NULL || gAppContext->modeContext == NULL)
    {
        return;
    }

    if (gAppContext->modeContext->eventManager != NULL)
    {
        sxrEventData eventData;
        qvrservice_proximity_notify_payload_t proximityData;

        svrEventQueue* pQueue = gAppContext->modeContext->eventManager->AcquireEventQueue();
        if (pQueue != NULL)
        {
            //qvrservice_proximity_notify_payload_t* pProximityPayload = (qvrservice_proximity_notify_payload_t*)payload;
            //eventData.proximity.distance = pProximityPayload->scalar;

            // Fix for payload memory alignment
            memcpy(&proximityData, payload, payload_length);
            eventData.proximity.distance = proximityData.scalar;

            LOGI("Proximity Distance: %f", eventData.proximity.distance);

            pQueue->SubmitEvent(kEventProximity, eventData);
        }
    }
}

//-----------------------------------------------------------------------------
void svrNotifyHeadless()
//-----------------------------------------------------------------------------
{
    // Find the method ...
	JNIEnv* pThreadJEnv;
	if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, NULL) != JNI_OK)
	{
		LOGE("sxrInitialize AttachCurrentThread failed.");
    }

    jclass ParentClass = gAppContext->javaSvrApiClass;
    jmethodID MethodId = pThreadJEnv->GetStaticMethodID(ParentClass, "NotifyHeadless", "(Landroid/app/Activity;)V");
    if (MethodId == NULL)
    {
        LOGE("Unable to find Method: SvrNativeActivity::NotifyHeadless()");
        return;
    }

    // ... call the method
    LOGI("Method Called: SvrNativeActivity::NotifyHeadless()...");
    pThreadJEnv->CallStaticVoidMethod(ParentClass, MethodId, gAppContext->javaActivityObject);
}

//-----------------------------------------------------------------------------
void qvrClientStateNotificationCallback(void *pCtx, QVRSERVICE_CLIENT_NOTIFICATION notification, void *payload, uint32_t payload_length)
//-----------------------------------------------------------------------------
{

    if (payload_length != sizeof(qvrservice_state_notify_payload_t))
    {
        LOGE("StateNotifcation payload length %d doesn't match expected payload length of %d", payload_length, (int)sizeof(qvrservice_state_notify_payload_t));
        return;
    }

    if (gAppContext == NULL || gAppContext->modeContext == NULL)
    {
        return;
    }

    if (gAppContext->modeContext->eventManager != NULL)
    {
        sxrEventData eventData;

        svrEventQueue *pQueue = gAppContext->modeContext->eventManager->AcquireEventQueue();
        if (pQueue != NULL)
        {
            qvrservice_state_notify_payload_t pStatePayload;
            memcpy(&pStatePayload, payload, payload_length);

            eventData.state.previous_state = (uint32_t)pStatePayload.previous_state;
            eventData.state.new_state = (uint32_t)pStatePayload.new_state;
            LOGI("qvrClientStateNotificationCallback: previous state %s", QVRServiceClient_StateToName((QVRSERVICE_VRMODE_STATE)eventData.state.previous_state));
            LOGI("qvrClientStateNotificationCallback: new state %s", QVRServiceClient_StateToName((QVRSERVICE_VRMODE_STATE)eventData.state.new_state));

            // VRMODE_STOPPED -> VRMODE_HEADLESS
            // VRMODE_STOPPING -> VRMODE_HEADLESS
            if ((eventData.state.previous_state == 4 && eventData.state.new_state == 5) ||
                (eventData.state.previous_state == 3 && eventData.state.new_state == 5))
            {
                svrNotifyHeadless(); //Pop a notification
                pQueue->SubmitEvent(kEventVrModeHeadless, eventData);
            }

            // VRMODE_HEADLESS -> VRMODE_STOPPED
            if (eventData.state.previous_state == 5 && eventData.state.new_state == 4)
            {
                LOGI("Headless: prepare to restart VR Mode!");
                pQueue->SubmitEvent(kEventVrModeHeadless, eventData);
                // Add ProcessEvents() here so the queue is updated. Otherwise the event will not
                // be processed in svrApplication.cpp
                gAppContext->modeContext->eventManager->ProcessEvents();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void svrNotifyFailedQvrService()
//-----------------------------------------------------------------------------
{
    // Find the method ...
	JNIEnv* pThreadJEnv;
	if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, NULL) != JNI_OK)
	{
		LOGE("sxrInitialize AttachCurrentThread failed.");
    }

    jclass ParentClass = gAppContext->javaSvrApiClass;
    jmethodID MethodId = pThreadJEnv->GetStaticMethodID(ParentClass, "NotifyNoVr", "(Landroid/app/Activity;)V");
    if (MethodId == NULL)
    {
        LOGE("Unable to find Method: SvrNativeActivity::NotifyNoVr()");
        return;
    }

    // ... call the method
    LOGI("Method Called: SvrNativeActivity::NotifyNoVr()...");
    pThreadJEnv->CallStaticVoidMethod(ParentClass, MethodId, gAppContext->javaActivityObject);
}

//-----------------------------------------------------------------------------
int svrGetAndroidOSVersion()
//-----------------------------------------------------------------------------
{
    char fileBuf[256];

    FILE* pFile = popen("getprop ro.build.version.sdk", "r");
    if (!pFile)
    {
        LOGE("Failed to getprop on 'ro.build.version.sdk'");
        return -1;
    }
    fgets(fileBuf, 256, pFile);

    int version;
    sscanf(fileBuf, "%d", &version);
    pclose(pFile);

    return version;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetPerformanceLevelsInternal(sxrPerfLevel cpuPerfLevel, sxrPerfLevel gpuPerfLevel)
//-----------------------------------------------------------------------------
{
    if (gAppContext && gAppContext->qvrHelper)
    {
        qvrservice_perf_level_t qvrLevels[2];

        qvrLevels[0].hw_type = HW_TYPE_CPU;
        qvrLevels[0].perf_level = SvrPerfLevelToQvrPerfLevel(cpuPerfLevel);

        qvrLevels[1].hw_type = HW_TYPE_GPU;
        qvrLevels[1].perf_level = SvrPerfLevelToQvrPerfLevel(gpuPerfLevel);

        LOGI("Attempting to set perf to (%d, %d)", qvrLevels[0].perf_level, qvrLevels[1].perf_level);
        int res = QVRServiceClient_SetOperatingLevel(gAppContext->qvrHelper, &qvrLevels[0], 2, NULL, NULL);
        if (res != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_SetOperatingLevel", res);
        }
        else
        {
            LOGI("sxrSetPerformanceLevels CPU Level : %d", (unsigned int)qvrLevels[0].perf_level);
            LOGI("sxrSetPerformanceLevels GPU Level : %d", (unsigned int)qvrLevels[1].perf_level);
        }

        if (gAppContext->modeContext)
        {
            int32_t qRes;

            if (gAppContext->modeContext->renderThreadId > 0)
            {
                LOGI("Calling SetThreadAttributesByType for RENDER thread...");
                QVRSERVICE_THREAD_TYPE threadType = gEnableRenderThreadFifo ? RENDER : NORMAL;
                qRes = QVRServiceClient_SetThreadAttributesByType(gAppContext->qvrHelper, gAppContext->modeContext->renderThreadId, threadType);
                if (qRes != QVR_SUCCESS)
                {
                    L_LogQvrError("QVRServiceClient_SetThreadAttributesByType", qRes);
                }
            }
            if (gAppContext->modeContext->warpThreadId > 0)
            {
                LOGI("Calling SetThreadAttributesByType for WARP thread...");
                QVRSERVICE_THREAD_TYPE threadType = gEnableWarpThreadFifo ? WARP : NORMAL;
                qRes = QVRServiceClient_SetThreadAttributesByType(gAppContext->qvrHelper, gAppContext->modeContext->warpThreadId, threadType);
                if (qRes != QVR_SUCCESS)
                {
                    L_LogQvrError("QVRServiceClient_SetThreadAttributesByType", qRes);
                }
            }
        }
    }
    else
    {
        LOGE("QVRServiceClient unavailable, sxrSetPerformanceLevels failed.");
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

	return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
void L_SetThreadPriority(const char *pName, int policy, int priority)
//-----------------------------------------------------------------------------
{

    // What is the current thread policy?
    int oldPolicy = sched_getscheduler(gettid());
    switch (oldPolicy)
    {
    case SCHED_NORMAL:      // 0
        LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_NORMAL", pName, (int)gettid(), (int)gettid());
        break;

    case SCHED_FIFO:        // 1
        LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_FIFO", pName, (int)gettid(), (int)gettid());
        break;

    case SCHED_FIFO | SCHED_RESET_ON_FORK:        // 1
        LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_FIFO | SCHED_RESET_ON_FORK", pName, (int)gettid(), (int)gettid());
        break;

    case SCHED_RR:          // 2
        LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_RR", pName, (int)gettid(), (int)gettid());
        break;

    case SCHED_BATCH:       // 3
        LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_BATCH", pName, (int)gettid(), (int)gettid());
        break;

        // case SCHED_ISO:         // 4
        //     LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_ISO", pName, (int)gettid(), (int)gettid());
        //     break;

    case SCHED_IDLE:        // 5
        LOGI("Current %s (0x%x = %d) Scheduling policy: SCHED_IDLE", pName, (int)gettid(), (int)gettid());
        break;

    default:
        LOGI("Current %s (0x%x = %d) Scheduling policy: %d", pName, (int)gettid(), (int)gettid(), oldPolicy);
        break;
    }

    // Where is it going?
    switch (policy)
    {
    case SCHED_NORMAL:      // 0
        LOGI("    Setting => SCHED_NORMAL");
        break;

    case SCHED_FIFO:        // 1
        LOGI("    Setting => SCHED_FIFO");
        break;

    case SCHED_FIFO | SCHED_RESET_ON_FORK:        // 1
        LOGI("    Setting => SCHED_FIFO | SCHED_RESET_ON_FORK");
        break;

    case SCHED_RR:          // 2
        LOGI("    Setting => SCHED_RR");
        break;

    case SCHED_BATCH:       // 3
        LOGI("    Setting => SCHED_BATCH");
        break;

        // case SCHED_ISO:         // 4
        //     LOGI("    Setting => SCHED_ISO");
        //     break;

    case SCHED_IDLE:        // 5
        LOGI("    Setting => SCHED_IDLE");
        break;

    default:
        LOGI("    Setting => UNKNOWN! (%d)", policy);
        break;
    }

	int qRes = QVRServiceClient_SetThreadPriority(gAppContext->qvrHelper, gettid(), policy, priority);
	if (qRes != QVR_SUCCESS)
	{
		L_LogQvrError("QVRServiceClient_SetThreadPriority", qRes);
	}

    // What was the result?
    int newPolicy = sched_getscheduler(gettid());
    switch (newPolicy)
    {
    case SCHED_NORMAL:      // 0
        LOGI("    Result => SCHED_NORMAL");
        break;

    case SCHED_FIFO:        // 1
        LOGI("    Result => SCHED_FIFO");
        break;

    case SCHED_FIFO | SCHED_RESET_ON_FORK:        // 1
        LOGI("    Result => SCHED_FIFO | SCHED_RESET_ON_FORK");
        break;

    case SCHED_RR:          // 2
        LOGI("    Result => SCHED_RR");
        break;

    case SCHED_BATCH:       // 3
        LOGI("    Result => SCHED_BATCH");
        break;

        // case SCHED_ISO:         // 4
        //     LOGI("    Result => SCHED_ISO");
        //     break;

    case SCHED_IDLE:        // 5
        LOGI("    Result => SCHED_IDLE");
        break;

    default:
        LOGI("    Result => UNKNOWN! (%d)", newPolicy);
        break;
    }
}

//-----------------------------------------------------------------------------
extern "C" SXRP_EXPORT SxrResult svrInitializeOptArgs(const sxrInitParams* pInitParams, void* pTmAPI)
//-----------------------------------------------------------------------------
{
	#if defined(SVR_PROFILING_ENABLED) && defined(SVR_PROFILE_TELEMETRY)
		gTelemetryAPI = (tm_api*)pTmAPI;
	#endif

	return sxrInitialize(pInitParams);
}

//-----------------------------------------------------------------------------
SxrResult sxrCreateQvr()
//-----------------------------------------------------------------------------
{
    LOGI("sxrCreateQvr: Calling QVRServiceClient_Create()...");
    gAppContext->qvrHelper = QVRServiceClient_Create();
    if (gAppContext->qvrHelper == NULL ||
        QVRServiceClient_GetVRMode(gAppContext->qvrHelper) == VRMODE_UNSUPPORTED )
    {
        // Need to destroy this so later functions will not return
        // an incomplete device info.
        if (gAppContext->qvrHelper != NULL)
        {
            LOGI("sxrCreateQvr: DestroyVRMode ...");
            QVRServiceClient_Destroy(gAppContext->qvrHelper);
        }
        gAppContext->qvrHelper = NULL;

        LOGE("sxrCreateQvr: QVR Service reported VR not supported; gAppContext->qvrHelper=%p, QVRServiceClient_GetVRMode(gAppContext->qvrHelper)=%i, where VRMODE_UNSUPPORTED=0", gAppContext->qvrHelper, QVRServiceClient_GetVRMode(gAppContext->qvrHelper));
        svrNotifyFailedQvrService();
        return SXR_ERROR_UNSUPPORTED;
    }
    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrReadSdkConfig()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrReadSdkConfig: failed: QVR not initialized!");
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    //Load SVR configuration options
    unsigned int len = 0;
    int qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_CONFIG_FILE, &len, NULL);
    if (qRes == QVR_SUCCESS)
    {
        LOGI("sxrReadSdkConfig: Loading variables from QVR Service [len=%d]", len);
        if (len > 0)
        {
            char *p = new char[len];
            qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_CONFIG_FILE, &len, p);
            if (qRes == QVR_SUCCESS)
            {
                LoadVariableBuffer(p);
            }
            else
            {
                L_LogQvrError("sxrReadSdkConfig: QVRServiceClient_SetClientStatusCallback(QVRSERVICE_SDK_CONFIG_FILE)", qRes);
            }
            delete[] p;
        }
    }
    else
    {
        L_LogQvrError("sxrReadSdkConfig: QVRServiceClient_GetParam(QVRSERVICE_SDK_CONFIG_FILE) Not Loaded", qRes);
        svrNotifyFailedQvrService();
        return SXR_ERROR_UNSUPPORTED;
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrGetDeviceMode(char *deviceMode)
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrGetDeviceMode failed: QVR not initialized!");
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    uint32_t paramValueLen = 0;
    int ret = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_DEVICE_MODE, &paramValueLen, NULL);
    if (ret != 0) {
        LOGE("sxrGetDeviceMode: [%s]: Getting length failed\n", QVRSERVICE_DEVICE_MODE);
        return SXR_ERROR_UNSUPPORTED;
    }
    LOGI("sxrGetDeviceMode: [%s] len %u",QVRSERVICE_DEVICE_MODE, paramValueLen);
    ret = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_DEVICE_MODE, &paramValueLen, deviceMode);
    if (ret != 0) {
        LOGE("sxrGetDeviceMode: [%s] failed: expected 0, got %d\n", QVRSERVICE_DEVICE_MODE, ret);
        return SXR_ERROR_UNSUPPORTED;
    }
    LOGI("sxrGetDeviceMode: [%s] = [%s], len %u",QVRSERVICE_DEVICE_MODE, deviceMode, paramValueLen);
    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrInitialize(const sxrInitParams* pInitParams)
//-----------------------------------------------------------------------------
{
    LOGI("sxrInitialize");

    LOGI("sxrApi Version : %s (%s)", sxrGetVersion(), ABI_STRING);

    gAppContext = new SvrAppContext();

    gAppContext->qvrHelper = NULL;

    gAppContext->inVrMode = false;

    gAppContext->looper = ALooper_forThread();
    gAppContext->javaVm = pInitParams->javaVm;
    gAppContext->javaEnv = pInitParams->javaEnv;
    gAppContext->javaActivityObject = pInitParams->javaActivityObject;
    char deviceMode[32] = {0, };
    // initialize keepNotification flag
    gAppContext->keepNotification = false;

	PROFILE_INITIALIZE();

    //Initialize render extensions
    if (!InitializeRenderExtensions())
    {
        LOGE("Failed to initialize required EGL/GL extensions");
        return SXR_ERROR_UNSUPPORTED;
    }
    else
    {
        LOGI("EGL/GL Extensions Initialized");
    }

    //Load the SvrApi Java class and cache necessary method references
    //Since we are utilizing a native activity we need to go through the activities class loader to find the SvrApi class

	JNIEnv* pThreadJEnv;
	if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, NULL) != JNI_OK)
	{
		LOGE("sxrInitialize AttachCurrentThread failed.");
        return SXR_ERROR_JAVA_ERROR;
    }

    jclass tmpActivityClass = pThreadJEnv->FindClass("android/app/NativeActivity");
    jclass activityClass = (jclass)pThreadJEnv->NewGlobalRef(tmpActivityClass);

    jmethodID getClassLoaderMethodId = pThreadJEnv->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject classLoaderObj = pThreadJEnv->CallObjectMethod(gAppContext->javaActivityObject, getClassLoaderMethodId);

    jclass tmpClassLoaderClass = pThreadJEnv->FindClass("java/lang/ClassLoader");
    jclass classLoader = (jclass)pThreadJEnv->NewGlobalRef(tmpClassLoaderClass);

    jmethodID findClassMethodId = pThreadJEnv->GetMethodID(classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    //Get a reference to the SvrApi class
    jstring strClassName = pThreadJEnv->NewStringUTF("com/qualcomm/sxrapi/SxrApi");
    jclass tmpjavaSvrApiClass = (jclass)pThreadJEnv->CallObjectMethod(classLoaderObj, findClassMethodId, strClassName);
    gAppContext->javaSvrApiClass = (jclass)pThreadJEnv->NewGlobalRef(tmpjavaSvrApiClass);

    if (gAppContext->javaSvrApiClass == NULL)
    {
        LOGE("Failed to initialzie SxrApi Java class");
        return SXR_ERROR_JAVA_ERROR;
    }

    //Register our native methods
    struct
    {
        jclass          clazz;
        JNINativeMethod nm;
    } nativeMethods[] =
    {
        { gAppContext->javaSvrApiClass, { "nativeVsync", "(J)V", (void*)Java_com_qualcomm_sxrapi_SxrApi_nativeVsync } }
    };

    const int count = sizeof(nativeMethods) / sizeof(nativeMethods[0]);

    for (int i = 0; i < count; i++)
    {
        if (pThreadJEnv->RegisterNatives(nativeMethods[i].clazz, &nativeMethods[i].nm, 1) != JNI_OK)
        {
            LOGE("Failed to register %s", nativeMethods[i].nm.name);
            return SXR_ERROR_JAVA_ERROR;
        }
    }

    //Cache method ids for the start/stop vsync callback methods
    gAppContext->javaSvrApiStartVsyncMethodId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "startVsync", "(Landroid/app/Activity;)V");
    if (gAppContext->javaSvrApiStartVsyncMethodId == NULL)
    {
        LOGE("Failed to locate startVsync method");
        return SXR_ERROR_JAVA_ERROR;
    }

    gAppContext->javaSvrApiStopVsyncMethodId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "stopVsync", "(Landroid/app/Activity;)V");
    if (gAppContext->javaSvrApiStopVsyncMethodId == NULL)
    {
        LOGE("Failed to locate stopVsync method");
        return SXR_ERROR_JAVA_ERROR;
    }

    SxrResult svrResult = sxrCreateQvr();
    if(svrResult != SXR_ERROR_NONE) return svrResult;

    svrResult = sxrReadSdkConfig();
    if(svrResult != SXR_ERROR_NONE) return svrResult;

    //Gather information about the device
    memset(&gAppContext->deviceInfo, 0, sizeof(sxrDeviceInfo));

    jmethodID refreshRateId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "getRefreshRate", "(Landroid/app/Activity;)F");
    if (refreshRateId == NULL)
    {
        LOGE("sxrGetDeviceInfo: Failed to locate getRefreshRate method");
        return SXR_ERROR_JAVA_ERROR;
    }
    else
    {
        jfloat refreshRate = pThreadJEnv->CallStaticFloatMethod(gAppContext->javaSvrApiClass, refreshRateId, gAppContext->javaActivityObject);
#ifdef ENABLE_REMOTE_XR_RENDERING
        if (gEnableRVR) {
            gRVRUseQVRServicePose = (gRVRFlags & 1) ? true : false;
            SxrResult svrResult = sxrGetDeviceMode(deviceMode);
            if (svrResult == SXR_ERROR_NONE && !strncmp(deviceMode, QVRSERVICE_DEVICE_MODE_SMARTVIEWER_LOCAL, strlen(QVRSERVICE_DEVICE_MODE_SMARTVIEWER_LOCAL))) {
                gEnableRVR = false;
            }

            if (gEnableRVR) {
                /*
                    Set framerate as per configuration file.
                */
                SxrServiceClientManager::UpdateRenderFrustum();
                refreshRate = gRVRFPS;
            }
            LOGI("[RVR] gRVRUseQVRServicePose %d [%s] = [%s]", gRVRUseQVRServicePose ? 1 : 0, QVRSERVICE_DEVICE_MODE, deviceMode);
        }
#endif
        gAppContext->deviceInfo.displayRefreshRateHz = refreshRate;
        LOGI("Android Display Refresh Rate: %f", refreshRate);
    }

    jmethodID getDisplayWidthId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "getDisplayWidth", "(Landroid/app/Activity;)I");
    if (getDisplayWidthId == NULL)
    {
        LOGE("Failed to locate getDisplayWidth method");
        return SXR_ERROR_JAVA_ERROR;
    }
    else
    {
        jint displayWidth = pThreadJEnv->CallStaticIntMethod(gAppContext->javaSvrApiClass, getDisplayWidthId, gAppContext->javaActivityObject);
        gAppContext->deviceInfo.displayWidthPixels = displayWidth;
        LOGI("Display Width : %d", displayWidth);
    }

    jmethodID getDisplayHeightId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "getDisplayHeight", "(Landroid/app/Activity;)I");
    if (getDisplayHeightId == NULL)
    {
        LOGE("Failed to locate getDisplayHeight method");
        return SXR_ERROR_JAVA_ERROR;
    }
    else
    {
        jint displayHeight = pThreadJEnv->CallStaticIntMethod(gAppContext->javaSvrApiClass, getDisplayHeightId, gAppContext->javaActivityObject);
        gAppContext->deviceInfo.displayHeightPixels = displayHeight;
        LOGI("Display Height : %d", displayHeight);
    }


    jmethodID getDisplayOrientationId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "getDisplayOrientation", "(Landroid/app/Activity;)I");
    if (getDisplayOrientationId == NULL)
    {
        LOGE("Failed to locate getDisplayOrientation method");
        return SXR_ERROR_JAVA_ERROR;
    }
    else
    {
        jint displayOrientation = 270;
        displayOrientation = pThreadJEnv->CallStaticIntMethod(gAppContext->javaSvrApiClass, getDisplayOrientationId, gAppContext->javaActivityObject);
        gAppContext->deviceInfo.displayOrientation = displayOrientation;
        LOGI("Display Orientation : %d", displayOrientation);
    }

    //Get the current OS version
    int osVersion = svrGetAndroidOSVersion();
    jmethodID getAndroidOsVersion = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "getAndroidOsVersion", "()I");
    {
        if (getAndroidOsVersion == NULL)
        {
            LOGE("Failed to locate getAndroidOsVersion method");
        }
        else
        {
            LOGI("Getting Android OS Version...");
            osVersion = pThreadJEnv->CallStaticIntMethod(gAppContext->javaSvrApiClass, getAndroidOsVersion);
        }
    }
    LOGI("    Android OS Version = %d", osVersion);

    // Svr Service
    LOGI("Allocating SvrServiceClient...");
    {
        //TODO: need to pass gAppContext or the eventlooper to add events
        gAppContext->svrServiceClient = new SvrServiceClient(gAppContext->javaVm, gAppContext->javaActivityObject);
        gAppContext->svrServiceClient->Connect();
    }
    LOGI("SvrServiceClient Connected");

#ifdef ENABLE_REMOTE_XR_RENDERING
    if(!gEnableRVR || gRVRUseQVRServicePose)
    {
#endif
    gAppContext->currentTrackingMode = 0;

	LOGI("Calling QVRServiceClient_Create()...");
	if(!gAppContext->qvrHelper) gAppContext->qvrHelper = QVRServiceClient_Create();
	if (gAppContext->qvrHelper == NULL ||
		QVRServiceClient_GetVRMode(gAppContext->qvrHelper) == VRMODE_UNSUPPORTED )
    {
        // Need to destroy this so later functions will not return
        // an incomplete device info.
        if (gAppContext->qvrHelper != NULL)
        {
            LOGI("sxrInitialize: DestroyVRMode ...");
            QVRServiceClient_Destroy(gAppContext->qvrHelper);
        }
        gAppContext->qvrHelper = NULL;

        LOGE("    QVR Service reported VR not supported; gAppContext->qvrHelper=%p, QVRServiceClient_GetVRMode(gAppContext->qvrHelper)=%i, where VRMODE_UNSUPPORTED=0", gAppContext->qvrHelper, QVRServiceClient_GetVRMode(gAppContext->qvrHelper));
        svrNotifyFailedQvrService();
        return SXR_ERROR_UNSUPPORTED;
    }

    LOGI("Setting SvrServiceClient Callback");
	int qRes = QVRServiceClient_SetClientStatusCallback(gAppContext->qvrHelper, qvrClientStatusCallback, 0);
	if (qRes != QVR_SUCCESS)
	{
		L_LogQvrError("QVRServiceClient_SetClientStatusCallback()", qRes);
	}

    //Get the supported tracking modes
    uint32_t supportedTrackingModes = sxrGetSupportedTrackingModes();
    if (supportedTrackingModes & kTrackingRotation)
    {
        LOGI("  QVR Service supports rotational tracking");
    }
    if (supportedTrackingModes & kTrackingPosition)
    {
        LOGI("  QVR Service supports positional tracking");
    }

    // Get and display eye tracking capabilities
    uint64_t eyeCaps = 0;
    SxrResult svrResult = sxrGetTrackingCapabilities(&eyeCaps);
    if (svrResult == SXR_ERROR_NONE)
    {
        LOGI("Service Tracking Capabilities:");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_COMBINED_GAZE) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_COMBINED_GAZE");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_CONVERGENCE_DISTANCE) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_CONVERGENCE_DISTANCE");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_FOVEATED_GAZE) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_FOVEATED_GAZE");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_GAZE_ORIGIN) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_GAZE_ORIGIN");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_GAZE_DIRECTION) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_GAZE_DIRECTION");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_2D_GAZE_POINT) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_2D_GAZE_POINT");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_EYE_OPENNESS) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_EYE_OPENNESS");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_PUPIL_DILATION) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_PUPIL_DILATION");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_POSITION_GUIDE) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_POSITION_GUIDE");
        if ((eyeCaps & QVR_CAPABILITY_GAZE_PER_EYE_BLINK) != 0)
            LOGI("\tQVR_CAPABILITY_GAZE_PER_EYE_BLINK");
    }
    gAppContext->deviceInfo.trackingCapabilities = eyeCaps;

#ifdef ENABLE_XR_CASTING
    if (gEnableVRCasting)
    {
	    LOGV("SxrPresentation GetMethodID(gAppContext->javaSvrApiClass, \"enablePresentation\", \"(Landroid/app/Activity;Z)V\");");
        gAppContext->javaSxrEnablePresentation = pThreadJEnv->GetStaticMethodID(
                gAppContext->javaSvrApiClass, "enablePresentation", "(Landroid/app/Activity;Z)V");
        if (gAppContext->javaSxrEnablePresentation == nullptr)
        {
            LOGE("Failed to get jmethodID for createTestPresentation");
            return SXR_ERROR_JAVA_ERROR;
        };

    }
#endif //ENABLE_XR_CASTING

    if (gUseVSyncCallback && gUseLinePtr)
    {
        LOGE("Both gUseVSyncCallback and gUseLinePtr are set! Using gUseVSyncCallback");
        gUseLinePtr = false;
    }

    if(gUseLinePtr)
    {
        //Make sure the linePtr support is actually available
        qvrservice_ts_t* qvrStamp;
        qRes = QVRServiceClient_GetDisplayInterruptTimestamp(gAppContext->qvrHelper, DISP_INTERRUPT_LINEPTR, &qvrStamp);
        if( qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_GetDisplayInterruptTimestamp", qRes);
            gUseLinePtr = false;
        }
    }

    if (gDisplayRefreshRateHz > 0)
    {
        gAppContext->deviceInfo.displayRefreshRateHz = (float)gDisplayRefreshRateHz;
        LOGI("Config Override Display Refresh Rate: %f", gAppContext->deviceInfo.displayRefreshRateHz);
    }

#ifdef ENABLE_REMOTE_XR_RENDERING
    }
#endif

    //Set the default tracking mode to rotational only
    // Need to be AFTER reading the config file so overrides will work
    sxrSetTrackingMode(kTrackingRotation);

    // OS Version
    gAppContext->deviceInfo.deviceOSVersion = osVersion;

    // Eye Buffer Size
    gAppContext->deviceInfo.targetEyeWidthPixels = gEyeBufferWidth;
    gAppContext->deviceInfo.targetEyeHeightPixels = gEyeBufferHeight;

    // Field-of-view calculated from frustum if not set in config
    if (gEyeBufferFovX < 0.000001f)
        gAppContext->deviceInfo.targetFovXRad = atan2(fabs(gLeftFrustum_Left), gLeftFrustum_Near) + atan2(fabs(gLeftFrustum_Right), gLeftFrustum_Near);
    else
        gAppContext->deviceInfo.targetFovXRad = gEyeBufferFovX * DEG_TO_RAD;

    if (gEyeBufferFovY < 0.000001f)
        gAppContext->deviceInfo.targetFovYRad = atan2(fabs(gLeftFrustum_Top), gLeftFrustum_Near) + atan2(fabs(gLeftFrustum_Bottom), gLeftFrustum_Near);
    else
        gAppContext->deviceInfo.targetFovYRad = gEyeBufferFovY * DEG_TO_RAD;
    LOGI("Config Field of View (degrees): %f vertical, %f horizontal", gAppContext->deviceInfo.targetFovXRad*RAD_TO_DEG, gAppContext->deviceInfo.targetFovYRad*RAD_TO_DEG);

    // Backward compatability for new gLensScaleX/Y
    if (gLensScaleX == 0.0f || gLensScaleY == 0.0f)
    {
        gLensScaleX = gLensScale;
        gLensScaleY = gLensScale;
    }

    // Calculate warp mesh scale given FrustumDisplay and Display dimensions (in meters)
    if (gFrustumDisplayWidth > 0.0f && gDisplayWidth > 0.0f && gFrustumDisplayHeight > 0.0f && gDisplayHeight > 0.0f)
    {
        float meshScaleX = gFrustumDisplayWidth / gDisplayWidth;
        gWarpMeshMinX = -meshScaleX;
        gWarpMeshMaxX = meshScaleX;
        gLensScaleX = meshScaleX;
        LOGI("Frustum Display Width Scale (%f / %f) => %f used to set gWarpMeshMinX, gWarpMeshMaxX, gLensScaleX",
            gFrustumDisplayWidth, gDisplayWidth, meshScaleX);

        float meshScaleY = gFrustumDisplayHeight / gDisplayHeight;
        gWarpMeshMinY = -meshScaleY;
        gWarpMeshMaxY = meshScaleY;
        gLensScaleY = meshScaleY;
        LOGI("Frustum Display Height Scale (%f / %f) => %f used to set gWarpMeshMinY, gWarpMeshMaxY, gLensScaleY",
            gFrustumDisplayHeight, gDisplayHeight, meshScaleY);

        gMeshDiscardUV = glm::max(meshScaleY, gMeshDiscardUV);
        LOGI("gMeshDiscardUV => %f", gMeshDiscardUV);

        gScreenWidthMM = gDisplayWidth * 2.0f;
        LOGI("gScreenWidthMM => %f", gScreenWidthMM);
    }


    // Frustum
    gAppContext->deviceInfo.targetEyeConvergence = gFrustum_Convergence;
    gAppContext->deviceInfo.targetEyePitch = gFrustum_Pitch;

    // Left Eye Frustum
    gAppContext->deviceInfo.leftEyeFrustum.near = gLeftFrustum_Near;
    gAppContext->deviceInfo.leftEyeFrustum.far = gLeftFrustum_Far;
    gAppContext->deviceInfo.leftEyeFrustum.left = gLeftFrustum_Left;
    gAppContext->deviceInfo.leftEyeFrustum.right = gLeftFrustum_Right;
    gAppContext->deviceInfo.leftEyeFrustum.top = gLeftFrustum_Top;
    gAppContext->deviceInfo.leftEyeFrustum.bottom = gLeftFrustum_Bottom;
    gAppContext->deviceInfo.leftEyeFrustum.position.x = gLeftFrustum_PositionX;
    gAppContext->deviceInfo.leftEyeFrustum.position.y = gLeftFrustum_PositionY;
    gAppContext->deviceInfo.leftEyeFrustum.position.z = gLeftFrustum_PositionZ;
    gAppContext->deviceInfo.leftEyeFrustum.rotation.x = gLeftFrustum_RotationX;
    gAppContext->deviceInfo.leftEyeFrustum.rotation.y = gLeftFrustum_RotationY;
    gAppContext->deviceInfo.leftEyeFrustum.rotation.z = gLeftFrustum_RotationZ;
    gAppContext->deviceInfo.leftEyeFrustum.rotation.w = gLeftFrustum_RotationW;

    // Right Eye Frustum
    gAppContext->deviceInfo.rightEyeFrustum.near = gRightFrustum_Near;
    gAppContext->deviceInfo.rightEyeFrustum.far = gRightFrustum_Far;
    gAppContext->deviceInfo.rightEyeFrustum.left = gRightFrustum_Left;
    gAppContext->deviceInfo.rightEyeFrustum.right = gRightFrustum_Right;
    gAppContext->deviceInfo.rightEyeFrustum.top = gRightFrustum_Top;
    gAppContext->deviceInfo.rightEyeFrustum.bottom = gRightFrustum_Bottom;
    gAppContext->deviceInfo.rightEyeFrustum.position.x = gRightFrustum_PositionX;
    gAppContext->deviceInfo.rightEyeFrustum.position.y = gRightFrustum_PositionY;
    gAppContext->deviceInfo.rightEyeFrustum.position.z = gRightFrustum_PositionZ;
    gAppContext->deviceInfo.rightEyeFrustum.rotation.x = gRightFrustum_RotationX;
    gAppContext->deviceInfo.rightEyeFrustum.rotation.y = gRightFrustum_RotationY;
    gAppContext->deviceInfo.rightEyeFrustum.rotation.z = gRightFrustum_RotationZ;
    gAppContext->deviceInfo.rightEyeFrustum.rotation.w = gRightFrustum_RotationW;

    // Foveation Parameters
    gAppContext->deviceInfo.lowFoveation.area = gLowFoveationArea;
    gAppContext->deviceInfo.lowFoveation.gain.x = gLowFoveationGainX;
    gAppContext->deviceInfo.lowFoveation.gain.y = gLowFoveationGainY;
    gAppContext->deviceInfo.lowFoveation.minimum = gLowFoveationMinimum;

    gAppContext->deviceInfo.medFoveation.area = gMedFoveationArea;
    gAppContext->deviceInfo.medFoveation.gain.x = gMedFoveationGainX;
    gAppContext->deviceInfo.medFoveation.gain.y = gMedFoveationGainY;
    gAppContext->deviceInfo.medFoveation.minimum = gMedFoveationMinimum;

    gAppContext->deviceInfo.highFoveation.area = gHighFoveationArea;
    gAppContext->deviceInfo.highFoveation.gain.x = gHighFoveationGainX;
    gAppContext->deviceInfo.highFoveation.gain.y = gHighFoveationGainY;
    gAppContext->deviceInfo.highFoveation.minimum = gHighFoveationMinimum;

    // Tracking Camera
    gAppContext->deviceInfo.trackingCalibration[ 0] = gCalibration_X_0;
    gAppContext->deviceInfo.trackingCalibration[ 1] = gCalibration_X_1;
    gAppContext->deviceInfo.trackingCalibration[ 2] = gCalibration_X_2;
    gAppContext->deviceInfo.trackingCalibration[ 3] = gCalibration_X_3;

    gAppContext->deviceInfo.trackingCalibration[ 4] = gCalibration_Y_0;
    gAppContext->deviceInfo.trackingCalibration[ 5] = gCalibration_Y_1;
    gAppContext->deviceInfo.trackingCalibration[ 6] = gCalibration_Y_2;
    gAppContext->deviceInfo.trackingCalibration[ 7] = gCalibration_Y_3;

    gAppContext->deviceInfo.trackingCalibration[ 8] = gCalibration_Z_0;
    gAppContext->deviceInfo.trackingCalibration[ 9] = gCalibration_Z_1;
    gAppContext->deviceInfo.trackingCalibration[10] = gCalibration_Z_2;
    gAppContext->deviceInfo.trackingCalibration[11] = gCalibration_Z_3;

    gAppContext->deviceInfo.trackingPrincipalPoint[0] = gPrincipalPoint_0;
    gAppContext->deviceInfo.trackingPrincipalPoint[1] = gPrincipalPoint_1;

    gAppContext->deviceInfo.trackingFocalLength[0] = gFocalLength_0;
    gAppContext->deviceInfo.trackingFocalLength[1] = gFocalLength_1;

    gAppContext->deviceInfo.trackingDistortion[0] = gRadialDistortion_0;
    gAppContext->deviceInfo.trackingDistortion[1] = gRadialDistortion_1;
    gAppContext->deviceInfo.trackingDistortion[2] = gRadialDistortion_2;
    gAppContext->deviceInfo.trackingDistortion[3] = gRadialDistortion_3;
    gAppContext->deviceInfo.trackingDistortion[4] = gRadialDistortion_4;
    gAppContext->deviceInfo.trackingDistortion[5] = gRadialDistortion_5;
    gAppContext->deviceInfo.trackingDistortion[6] = gRadialDistortion_6;
    gAppContext->deviceInfo.trackingDistortion[7] = gRadialDistortion_7;

    // Warp mesh type
    switch (gWarpMeshType)
    {
    case 0:     // 0 = Columns (Left To Right)
        gAppContext->deviceInfo.warpMeshType = kMeshTypeColumsLtoR;
        break;

    case 1:     // 1 = Columns (Right To Left)
        gAppContext->deviceInfo.warpMeshType = kMeshTypeColumsRtoL;
        break;

    case 2:     // 2 = Rows (Top To Bottom)
        gAppContext->deviceInfo.warpMeshType = kMeshTypeRowsTtoB;
        break;

    case 3:     // 3 = Rows (Bottom To Top)
        gAppContext->deviceInfo.warpMeshType = kMeshTypeRowsBtoT;
        break;

    default:
        LOGE("Unknown warp mesh type (gWarpMeshType) specified in configuration file: %d", gWarpMeshType);
        gAppContext->deviceInfo.warpMeshType = kMeshTypeColumsLtoR;
        break;
    }

    //Log out some useful information
    jmethodID vsyncOffetId = pThreadJEnv->GetStaticMethodID(gAppContext->javaSvrApiClass,
        "getVsyncOffsetNanos", "(Landroid/app/Activity;)J");
    if (vsyncOffetId == NULL)
    {
        LOGE("Failed to locate getVsyncOffsetNanos method");
    }
    else
    {
        jlong result = pThreadJEnv->CallStaticLongMethod(gAppContext->javaSvrApiClass, vsyncOffetId, gAppContext->javaActivityObject);
        LOGI("Vsync Offset : %d", (int)result);
    }

    if (gDisableReprojection)
    {
        LOGI("Timewarp disabled from configuration file");
    }

    if (gForceMinVsync > 0)
    {
        LOGI("Forcing minVsync = %d", gForceMinVsync);
    }

	LOGI("Using QVR Performance Module");

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrShutdown()
//-----------------------------------------------------------------------------
{
    LOGI("sxrShutdown");

	PROFILE_SHUTDOWN();

    if (gAppContext == NULL)
    {
        LOGE("sxrShutdown Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    if (gAppContext != NULL)
    {
        // Disconnect from SvrService
        if( gAppContext->svrServiceClient != 0 )
        {
            gAppContext->svrServiceClient->Disconnect();
            delete gAppContext->svrServiceClient;
            gAppContext->svrServiceClient = 0;
        }

        if (gAppContext->qvrHelper != NULL)
        {
            LOGI("sxrShutdown: DestroyVRMode ...");
            QVRServiceClient_Destroy(gAppContext->qvrHelper);
        }
        gAppContext->qvrHelper = NULL;

        delete gAppContext;
        gAppContext = NULL;
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
sxrDeviceInfo sxrGetDeviceInfo()
//-----------------------------------------------------------------------------
{
    if (gAppContext != NULL)
    {
        return gAppContext->deviceInfo;
    }
    else
    {
        LOGE("Called sxrGetDeviceInfo without initialzing sxr");
        sxrDeviceInfo tmp;
        memset(&tmp, 0, sizeof(sxrDeviceInfo));
        return tmp;
    }
}

//-----------------------------------------------------------------------------
SXRP_EXPORT bool sxrIsProximitySuspendEnabled()
//-----------------------------------------------------------------------------
{
    return gProximitySuspendEnabled == true;
}

//-----------------------------------------------------------------------------
void L_AddHeuristicPredictData(uint64_t newData)
//-----------------------------------------------------------------------------
{
    if (!gHeuristicPredictedTime || gNumHeuristicEntries <= 0 || gpHeuristicPredictData == NULL)
    {
        LOGE("Unable to add heuristic predicted time data. Heuristic predicted time is not enabled!");
        return;
    }

    // Put this new time in the latest slot...
    float predictTimeMs = (float)newData * NANOSECONDS_TO_MILLISECONDS;

    // During initialization the values put in heuristic list can be very large
    if (predictTimeMs < 0.0f || predictTimeMs > gMaxPredictedTime)
    {
        if (gLogMaxPredictedTime)
        {
            LOGI("Heuristic data of %0.3f clampled by gMaxPredictedTime of %0.3f", predictTimeMs, gMaxPredictedTime);
        }
        predictTimeMs = gMaxPredictedTime;
    }

    gpHeuristicPredictData[gHeuristicWriteIndx] = predictTimeMs;

    // ... and bump the write pointer
    gHeuristicWriteIndx++;
    if (gHeuristicWriteIndx >= gNumHeuristicEntries)
        gHeuristicWriteIndx = 0;

}

//-----------------------------------------------------------------------------
float L_GetHeuristicPredictTime()
//-----------------------------------------------------------------------------
{
    if (!gHeuristicPredictedTime || gNumHeuristicEntries <= 0 || gpHeuristicPredictData == NULL)
    {
        LOGE("Unable to get heuristic predicted time. Heuristic predicted time is not enabled!");
        return 0.0f;
    }

    // What is the average of the last gNumHeuristicEntries
    float averageEntry = 0.0f;
    for (int whichEntry = 0; whichEntry < gNumHeuristicEntries; whichEntry++)
    {
        averageEntry += gpHeuristicPredictData[whichEntry];
    }
    averageEntry /= (float)gNumHeuristicEntries;

    // Add the offset before logging
    averageEntry += gHeuristicOffset;

    if (gLogPrediction)
    {
        LOGI("Heuristic predicted display time: %0.3f", averageEntry);
    }

    return averageEntry;
}

//-----------------------------------------------------------------------------
void VSyncEventCallback(void *pCtx, uint64_t ts)
//-----------------------------------------------------------------------------
{
    // This callback is not longer supported
    // LOGI("VSyncEventCallback: %llu", (long long unsigned int)ts);

    // gAppContext->modeContext->vsyncTimeNano = ts;
    // gAppContext->modeContext->vsyncCount++;
}

#ifdef ENABLE_XR_CASTING
//-----------------------------------------------------------------------------
bool  svrCreatePresentationContextFromNativeWindow()
//-----------------------------------------------------------------------------
{
    LOGI("svrCreatePresentationContextFromNativeWindow()");

    gAppContext->modeContext->SxrPresentationWindowDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // initialize OpenGL ES and EGL if not already from the regular presentation display
    EGLint major, minor;
    const EGLBoolean eglInitializeResult = eglInitialize(
            gAppContext->modeContext->SxrPresentationWindowDisplay,
            &major, &minor);
    LOGI("EGL version %d.%d", major, minor);
    LOG_EGL_RESOURCE("%s:%i:threadid=%i, eglInitialize(display=%p)=%i", __FILE__, __LINE__,
                     gettid(), SxrPresentationDisplay, eglInitializeResult);

    EGLint      format;
    EGLSurface  surface;
    EGLContext  context;
    EGLConfig   config = 0;
    EGLint      isProtectedContent = EGL_FALSE;

    sxrDeviceInfo di = sxrGetDeviceInfo();
    EGLDisplay display = gAppContext->modeContext->SxrPresentationWindowDisplay;

    //Find an appropriate config for our presentation context
    EGLConfig configs[512];
    EGLint numConfigs = 0;

    eglGetConfigs(display, NULL, 0, &numConfigs);
    eglGetConfigs(display, configs, 512, &numConfigs);

    LOGI("Found %d EGL configs...", numConfigs);

    const EGLint attribs[] = {
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 0,
            EGL_NONE
    };

    //Find an appropriate surface configuration
    int configId;
    for (configId = 0; configId < numConfigs; configId++)
    {
        EGLint value = 0;

        eglGetConfigAttrib(display, configs[configId], EGL_RENDERABLE_TYPE, &value);
        if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
        {
            LOGI("    EGL config %d rejected: Not ES3 surface", configId);
            continue;
        }

        eglGetConfigAttrib(display, configs[configId], EGL_SURFACE_TYPE, &value);
        if ((value & (EGL_PBUFFER_BIT)) != (EGL_PBUFFER_BIT))
        {
            LOGI("    EGL config %d rejected: Not EGL Pbuffer", configId);
            continue;
        }

        int	j = 0;
        for (; attribs[j] != EGL_NONE; j += 2)
        {
            eglGetConfigAttrib(display, configs[configId], attribs[j], &value);
            if (value != attribs[j + 1])
            {
                LOGI("    EGL config %d rejected: Attrib %d (Need %d, got %d)", configId, j, attribs[j + 1], value);
                break;
            }
        }
        if (attribs[j] == EGL_NONE)
        {
            LOGI("    EGL config %d Accepted!", configId);
            config = configs[configId];
            break;
        }
    }

    if (config == 0)
    {
        LOGE("svrCreatePresentationContextFromNativeWindow: Failed to find suitable EGL config");
        return -1;
    }

    //Create the new surface
    EGLint renderBufferType = EGL_BACK_BUFFER;

    if (gAppContext->modeContext->isProtectedContent)
    {
        isProtectedContent = EGL_TRUE;
        LOGI("isProtectedContent is true");
    }

    EGLint colorSpace = gAppContext->modeContext->colorspace;

    const EGLint windowAttribs[] =
            {
                    EGL_RENDER_BUFFER, renderBufferType,
                    EGL_PROTECTED_CONTENT_EXT, isProtectedContent,
                    EGL_COLORSPACE, colorSpace,
                    EGL_NONE

            };

    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(gAppContext->modeContext->SxrPresentationWindowANativeWindow,
                                     gAppContext->modeContext->SxrPresentationWindowSurfaceWidth,
                                     gAppContext->modeContext->SxrPresentationWindowSurfaceHeight, format);

    surface = eglCreateWindowSurface(display,config, gAppContext->modeContext->SxrPresentationWindowANativeWindow ,windowAttribs);
    LOG_EGL_RESOURCE("%s:%i:threadid=%i, eglCreateWindowSurface(display=%p, config=%p, window=%p)", __FILE__, __LINE__, gettid(), display, config, gAppContext->modeContext->SxrPresentationWindowANativeWindow);

    if (surface == EGL_NO_SURFACE)
    {

        EGLint error = eglGetError();

        char *pError = NULL;
        switch (error)
        {
            case EGL_SUCCESS:				pError = (char *)"EGL_SUCCESS"; break;
            case EGL_NOT_INITIALIZED:		pError = (char *)"EGL_NOT_INITIALIZED"; break;
            case EGL_BAD_ACCESS:			pError = (char *)"EGL_BAD_ACCESS"; break;
            case EGL_BAD_ALLOC:				pError = (char *)"EGL_BAD_ALLOC"; break;
            case EGL_BAD_ATTRIBUTE:			pError = (char *)"EGL_BAD_ATTRIBUTE"; break;
            case EGL_BAD_CONTEXT:			pError = (char *)"EGL_BAD_CONTEXT"; break;
            case EGL_BAD_CONFIG:			pError = (char *)"EGL_BAD_CONFIG"; break;
            case EGL_BAD_CURRENT_SURFACE:	pError = (char *)"EGL_BAD_CURRENT_SURFACE"; break;
            case EGL_BAD_DISPLAY:			pError = (char *)"EGL_BAD_DISPLAY"; break;
            case EGL_BAD_SURFACE:			pError = (char *)"EGL_BAD_SURFACE"; break;
            case EGL_BAD_MATCH:				pError = (char *)"EGL_BAD_MATCH"; break;
            case EGL_BAD_PARAMETER:			pError = (char *)"EGL_BAD_PARAMETER"; break;
            case EGL_BAD_NATIVE_PIXMAP:		pError = (char *)"EGL_BAD_NATIVE_PIXMAP"; break;
            case EGL_BAD_NATIVE_WINDOW:		pError = (char *)"EGL_BAD_NATIVE_WINDOW"; break;
            case EGL_CONTEXT_LOST:			pError = (char *)"EGL_CONTEXT_LOST"; break;
            default:
                pError = (char *)"Unknown EGL Error!";
                LOGE("Unknown! (0x%x)", error);
                break;
        }

        LOGE("svrCreatePresentationContextFromNativeWindow: eglCreateWindowSurface failed (Egl Error = %s)", pError);

        return false;
    }



    //Create the presentation context
    EGLint contextAttribs[] =
            {
                    EGL_CONTEXT_CLIENT_VERSION, 3,
                    EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_LOW_IMG,
                    EGL_PROTECTED_CONTENT_EXT, isProtectedContent,
                    EGL_NONE
            };
    context = eglCreateContext(display, config, gAppContext->modeContext->eyeRenderContext, contextAttribs);
    LOG_EGL_RESOURCE("%s:%i:threadid=%i, eglCreateContext(display=%p, config=%p, share_context=%p)=warpRenderContext=%p", __FILE__, __LINE__, gettid(), display, config, gAppContext->modeContext->eyeRenderContext, context);

    if (context == EGL_NO_CONTEXT)
    {
        LOGE("svrCreatePresentationContextFromNativeWindow: Failed to create EGL context");
        return false;
    }

    //Check the context priority that was actually assigned to ensure we are high priority
    EGLint resultPriority;
    eglQueryContext(display, context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &resultPriority);
    switch (resultPriority)
    {
        case EGL_CONTEXT_PRIORITY_HIGH_IMG:
            LOGI("svrCreatePresentationContextFromNativeWindow: context has high priority");
            break;
        case EGL_CONTEXT_PRIORITY_MEDIUM_IMG:
            LOGE("svrCreatePresentationContextFromNativeWindow: context has medium priority");
            break;
        case EGL_CONTEXT_PRIORITY_LOW_IMG:
            LOGE("svrCreatePresentationContextFromNativeWindow: context has low priority");
            break;
        default:
            LOGE("svrCreatePresentationContextFromNativeWindow: unknown context priority");
            break;
    }

    LOG_EGL_RESOURCE("%s:%i:threadid=%i, eglGetCurrentSurface(EGL_DRAW)=%p, eglGetCurrentSurface(EGL_READ)=%p, eglGetCurrentContext()=%p, eglGetCurrentDisplay()=%p", __FILE__, __LINE__, gettid(), eglGetCurrentSurface(EGL_DRAW), eglGetCurrentSurface(EGL_READ), eglGetCurrentContext(), eglGetCurrentDisplay());
    LOG_EGL_RESOURCE("%s:%i:threadid=%i, eglMakeCurrent(display=%p, surface=%p, surface=%p, context=%p)=1", __FILE__, __LINE__, gettid(), display, surface, surface, context);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
    {
        LOGE("svrCreatePresentationContextFromNativeWindow: eglMakeCurrent failed");
        return false;
    }

    //Verify that we got what we actually asked for
    EGLint acutalRenderBuffer;
    eglQueryContext(display, context, EGL_RENDER_BUFFER, &acutalRenderBuffer);
    LOGI("svrCreatePresentationContextFromNativeWindow: Requested EGL_BACK_BUFFER");

    if (acutalRenderBuffer == EGL_SINGLE_BUFFER)
    {
        LOGI("    svrCreatePresentationContextFromNativeWindow: Got EGL_SINGLE_BUFFER");
    }
    else if (acutalRenderBuffer == EGL_BACK_BUFFER)
    {
        LOGI("    svrCreatePresentationContextFromNativeWindow: Got EGL_BACK_BUFFER");
    }
    else if (acutalRenderBuffer == EGL_NONE)
    {
        LOGE("    svrCreatePresentationContextFromNativeWindow: Got EGL_NONE");
    }
    else
    {
        LOGE("    svrCreatePresentationContextFromNativeWindow: Got Unknown");
    }

    //Grab some data from the surface and signal we've finished setting up the warp context

    gAppContext->modeContext->SxrPresentationWindowContext = context;
    gAppContext->modeContext->SxrPresentationWindowSurface = surface;
    LOGI("svrCreatePresentationContextFromNativeWindow: Presentation context successfully created");

    GL(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &(gAppContext->modeContext->SxrPresentationCachedReadFBO)));

    if (gAppContext->modeContext->SxrPresentationFBO == 0)
    {
        LOGV("SxrPresentationProcessFrame2 Creating SxrPresentationFBO");
        GL(glGenFramebuffers(1, &(gAppContext->modeContext->SxrPresentationFBO)));
        LOGV("SxrPresentationProcessFrame2 SxrPresentationFBO created: %u", gAppContext->modeContext->SxrPresentationFBO);
    }

    GL(glDisable(GL_DEPTH_TEST));
    GL(glDepthMask(GL_FALSE));
    GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    GL(glDisable(GL_SCISSOR_TEST));
    GL(glDisable(GL_STENCIL_TEST));

    return 0;
}

//-----------------------------------------------------------------------------
void sxrDestroyPresentationContextFromNativeWindow()
//-----------------------------------------------------------------------------
{
    LOGV("sxrDestroyPresentationContextFromNativeWindow");
    if (gAppContext != NULL &&
        gAppContext->modeContext != NULL)
    {
        if (gAppContext->modeContext->SxrPresentationFBO != 0)
        {
            LOGV("SxrPresentationProcessFrame Deleting SxrPresentationFBO");
            GL(glDeleteFramebuffers(1, &(gAppContext->modeContext->SxrPresentationFBO)));
            gAppContext->modeContext->SxrPresentationFBO = 0;
        }
        EGLDisplay display = eglGetCurrentDisplay();
        if (display != EGL_NO_DISPLAY) {
            EGLBoolean makeCurrentResult = eglMakeCurrent(
                    display, EGL_NO_SURFACE,
                    EGL_NO_SURFACE, EGL_NO_CONTEXT);
            LOG_EGL_RESOURCE(
                    "%s:%i:threadid=%i, eglMakeCurrent(display=%p, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)=%i",
                    __FILE__, __LINE__, gettid(), display, makeCurrentResult);
        }

        if (gAppContext->modeContext->SxrPresentationWindowSurface != EGL_NO_SURFACE)
        {
            const EGLBoolean destroySurfaceResult = eglDestroySurface(
                    display,
                    gAppContext->modeContext->SxrPresentationWindowSurface);
            LOG_EGL_RESOURCE(
                    "%s:%i:threadid=%i, eglDestroySurface(display=%p, SxrPresentationWindowSurface=%p)=%i",
                    __FILE__, __LINE__, gettid(), display,
                    gAppContext->modeContext->SxrPresentationWindowSurface, destroySurfaceResult);

            gAppContext->modeContext->SxrPresentationWindowSurface = EGL_NO_SURFACE;
        }

        if (gAppContext->modeContext->SxrPresentationWindowContext != EGL_NO_CONTEXT)
        {
            const EGLBoolean destroyContextResult = eglDestroyContext(
                    display,
                    gAppContext->modeContext->SxrPresentationWindowContext);
            LOG_EGL_RESOURCE(
                    "%s:%i:threadid=%i, eglDestroyContext(display=%p, SxrPresentationWindowContext=%p)=%i",
                    __FILE__, __LINE__, gettid(), display,
                    gAppContext->modeContext->SxrPresentationWindowContext, destroyContextResult);

            gAppContext->modeContext->SxrPresentationWindowContext = EGL_NO_CONTEXT;
        }
    }
    else
        {
        LOGE("svrDestroyPresentationContext called without valid application or VR mode context");
    }
}

//-----------------------------------------------------------------------------
void SxrPresentationProcessFrame(svrFrameParamsInternal* pWarpFrame)
//-----------------------------------------------------------------------------
{
    if (eglMakeCurrent(gAppContext->modeContext->SxrPresentationWindowDisplay,
            gAppContext->modeContext->SxrPresentationWindowSurface,
            gAppContext->modeContext->SxrPresentationWindowSurface,
            gAppContext->modeContext->SxrPresentationWindowContext) == EGL_FALSE)
    {
        LOGE("SxrPresentationProcessFrame: eglMakeCurrent failed");
        return;
    }

    GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, gAppContext->modeContext->SxrPresentationFBO));
    GL(glReadBuffer(GL_COLOR_ATTACHMENT0));

    uint32_t validLayerIndex = 0;
    int32_t texture_width = 0;
    int32_t texture_height = 0;
    glm::vec4 uvScaleoffset;

    for (uint32_t whichLayer = 0; whichLayer < SXR_MAX_RENDER_LAYERS; whichLayer++)
    {
        if ((kEyeMaskLeft & pWarpFrame->frameParams.renderLayers[whichLayer].eyeMask) &&
            pWarpFrame->frameParams.renderLayers[whichLayer].imageHandle != 0)
        {
            validLayerIndex = whichLayer;
            uint32_t imageHandle = 0;
            if (pWarpFrame->frameParams.renderLayers[validLayerIndex].imageType ==
                kTypeVulkan)
            {
                imageHandle = GetVulkanInteropHandle(
                        &(pWarpFrame->frameParams.renderLayers[validLayerIndex]));
            }
            else
            {
                imageHandle = pWarpFrame->frameParams.renderLayers[validLayerIndex].imageHandle;
            }

            LOGV("SxrPresentationProcessFrame Found a valid renderlayer for SxrPresentationFBO %u: handle=%u",
                 validLayerIndex, imageHandle);
            switch (pWarpFrame->frameParams.renderLayers[validLayerIndex].imageType)
            {
                case kTypeTextureArray:
                    LOGV("imageType=kTypeTextureArray");
                    GL(glBindTexture(GL_TEXTURE_2D_ARRAY, imageHandle));
                    GL(glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH,
                                                &texture_width));
                    GL(glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_HEIGHT,
                                                &texture_height));
                    GL(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
                    GL(glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 imageHandle,
                                                 0, 0));
                    break;
                case kTypeTexture:
                    LOGV("imageType=kTypeTexture");
                    GL(glBindTexture(GL_TEXTURE_2D, imageHandle));
                    GL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                                                &texture_width));
                    GL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT,
                                                &texture_height));
                    GL(glBindTexture(GL_TEXTURE_2D, 0));
                    GL(glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                              GL_TEXTURE_2D, imageHandle, 0));
                    break;
                default:
                    return;
            }

            // Scale is the difference (Assume right angles at all corners...
            uvScaleoffset.x = pWarpFrame->frameParams.renderLayers[validLayerIndex].imageCoords.LowerUVs[2] - pWarpFrame->frameParams.renderLayers[validLayerIndex].imageCoords.LowerUVs[0];
            uvScaleoffset.y = pWarpFrame->frameParams.renderLayers[validLayerIndex].imageCoords.UpperUVs[1] - pWarpFrame->frameParams.renderLayers[validLayerIndex].imageCoords.LowerUVs[1];

            // ...offset is the desired lowest value...
            uvScaleoffset.z = pWarpFrame->frameParams.renderLayers[validLayerIndex].imageCoords.LowerUVs[0];
            uvScaleoffset.w = pWarpFrame->frameParams.renderLayers[validLayerIndex].imageCoords.LowerUVs[1];

            LOGV("Setting Presentation uvScaleOffset: (%0.2f, %0.2f, %0.2f, %0.2f)", uvScaleoffset.x, uvScaleoffset.y, uvScaleoffset.z, uvScaleoffset.w);

            break;
        }
    }

    // Check results
    GLenum eResult = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if(eResult != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (eResult)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                LOGE("%s => Error (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) setting up FBO",
                     "SxrPresentationProcessFrame");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                LOGE("%s => Error (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) setting up FBO",
                     "SxrPresentationProcessFrame");
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                LOGE("%s => Error (GL_FRAMEBUFFER_UNSUPPORTED) setting up FBO",
                     "SxrPresentationProcessFrame");
                break;
            default:
                LOGE("%s => Error (0x%X) setting up FBO",
                     "SxrPresentationProcessFrame",
                     eResult);
                break;
        }
        GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, gAppContext->modeContext->SxrPresentationCachedReadFBO));
        return;
    }

    texture_width = texture_width * uvScaleoffset.x;
    texture_height = texture_height * uvScaleoffset.y;
    int32_t texture_x_origin = texture_width * uvScaleoffset.z;
    int32_t texture_y_origin = texture_height * uvScaleoffset.w;

    float heightRatio = float(texture_height) / float(gAppContext->modeContext->SxrPresentationWindowSurfaceHeight);
    float widthRatio = float(texture_width) / float(gAppContext->modeContext->SxrPresentationWindowSurfaceWidth);
    if (heightRatio > widthRatio)
    {
        float adjustment = widthRatio / heightRatio;
        heightRatio = 1.0f;
        widthRatio = heightRatio * adjustment;
    }
    else
    {
        float adjustment = heightRatio / widthRatio;
        widthRatio = 1.0f;
        heightRatio = widthRatio * adjustment;
    }

    int32_t blitWidth = widthRatio * float(gAppContext->modeContext->SxrPresentationWindowSurfaceWidth);
    int32_t blitHeight = heightRatio * float(gAppContext->modeContext->SxrPresentationWindowSurfaceHeight);

    int32_t blitDiffWidth = (gAppContext->modeContext->SxrPresentationWindowSurfaceWidth - blitWidth) / 2;
    int32_t blitDiffHeight = (gAppContext->modeContext->SxrPresentationWindowSurfaceHeight - blitHeight) / 2;

    LOGV("sw:%i sh:%i tw:%i th:%i wr:%0.2f hr%0.2f bw:%i bh:%i bdw:%i bdh:%i",
         gAppContext->modeContext->SxrPresentationWindowSurfaceWidth,
         gAppContext->modeContext->SxrPresentationWindowSurfaceHeight,
         texture_width,
         texture_height,
         widthRatio,
         heightRatio,
         blitWidth,
         blitHeight,
         blitDiffWidth,
         blitDiffHeight);
    GL(glBlitFramebuffer(texture_x_origin, texture_y_origin, texture_width, texture_height,
                         blitDiffWidth, blitDiffHeight, blitWidth + blitDiffWidth, blitHeight + blitDiffHeight,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR));

    GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, gAppContext->modeContext->SxrPresentationCachedReadFBO));

    if (!eglSwapBuffers(gAppContext->modeContext->SxrPresentationWindowDisplay, gAppContext->modeContext->SxrPresentationWindowSurface))
    {
        LOGE("SxrPresentationProcessFrame eglSwapBuffers() returned error %d", eglGetError());
    }
}

//-----------------------------------------------------------------------------
void* sxrPresentationThreadMain(void* arg)
//-----------------------------------------------------------------------------
{
    LOGI("sxrPresentationThreadMain Entered");
    gAppContext->modeContext->SxrPresentationThreadId = gettid();

    PROFILE_THREAD_NAME(0, 0, "SxrPresentation");

    if (gUseQvrPerfModule && gAppContext->qvrHelper)
    {
        LOGV("SxrPresentation Calling SetThreadAttributesByType for PRESENTATION thread...");
        QVRSERVICE_THREAD_TYPE threadType = NORMAL;
        int32_t qRes = QVRServiceClient_SetThreadAttributesByType(gAppContext->qvrHelper,
                gAppContext->modeContext->SxrPresentationThreadId, threadType);
        if (qRes != QVR_SUCCESS)
        {
            LOGE("SxrPresentation QVRServiceClient_SetThreadAttributesByType failed for PRESENTATION thread.");
        }
    }
    else
    {
        if (gEnablePresentationThreadFifo)
        {
            int lowestPriority = sched_get_priority_min(SCHED_FIFO);
            L_SetThreadPriority("Presentation Thread", SCHED_FIFO, lowestPriority);
        }

        if (gPresentationThreadCore >= 0)
        {
            LOGV("SxrPresentation Setting Presentation Affinity to %d", gPresentationThreadCore);
            svrSetThreadAffinity(gPresentationThreadCore);
        }
    }

    pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationReadyMutex);
    gAppContext->modeContext->SxrPresentationThreadReady = true;
    pthread_cond_signal(&gAppContext->modeContext->SxrPresentationInitializedCv);
    pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationReadyMutex);

    gAppContext->modeContext->SxrPresentationCount = 0;
    svrFrameParamsInternal* presentationParams = nullptr;
    while (gAppContext->modeContext->SxrPresentationThreadExit == false)
    {
        LOGV("SxrPresentation: PresentationThread pthread_mutex_lock iteration %"
                     PRIu64
                     "", gAppContext->modeContext->SxrPresentationCount);
        pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationFrameMutex);
        if (gAppContext->modeContext->SxrPresentationLastFrameWarpedTimeStamp >=
               gAppContext->modeContext->SxrPresentationPendingFrameTimeStamp)
        {
            LOGV("SxrPresentation: PresentationThread pthread_cond_wait iteration %"
                         PRIu64
                         "", gAppContext->modeContext->SxrPresentationCount);
            pthread_cond_wait(&gAppContext->modeContext->SxrPresentationFrameAvailableCV,
                              &gAppContext->modeContext->SxrPresentationFrameMutex);
        }

        //Check if the main thread has requested an exit
        if (gAppContext->modeContext->SxrPresentationThreadExit)
        {
            LOGV("SxrPresentation exit flag detected");
            pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationFrameMutex);
            break;
        }

        LOGV("SxrPresentation: PresentationThread start iteration %" PRIu64 ", frameTimeStamp: %" PRIu64 "",
                gAppContext->modeContext->SxrPresentationCount,
                gAppContext->modeContext->SxrPresentationPendingFrameTimeStamp);

        presentationParams = gAppContext->modeContext->SxrPresentationPendingFrameParams;

        LOGV("SxrPresentation: PresentationThread pthread_mutex_unlock iteration %"
                     PRIu64
                     ", frameTimeStamp: %"
                     PRIu64
                     "", gAppContext->modeContext->SxrPresentationCount,
             gAppContext->modeContext->SxrPresentationPendingFrameTimeStamp);
        pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationFrameMutex);

        //Check if the main thread has signaled a surface destroyed
        if (gAppContext->modeContext->SxrPresentationSurfaceDestroyed)
        {
            LOGI("SxrPresentation PresentationThread SxrPresentationSurfaceDestroyed");
            pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);

            sxrDestroyPresentationContextFromNativeWindow();

            gAppContext->modeContext->SxrPresentationSurfaceDestroyed = false;
            pthread_cond_signal(&gAppContext->modeContext->SxrPresentationSurfaceDestroyedCV);
            pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);
        }

        //Check if the main thread has signaled a surface Change
        if (gAppContext->modeContext->SxrPresentationSurfaceChanged)
        {
            LOGI("SxrPresentation PresentationThread SxrPresentationSurfaceChanged");
            pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);

            sxrDestroyPresentationContextFromNativeWindow();
            svrCreatePresentationContextFromNativeWindow();

            gAppContext->modeContext->SxrPresentationSurfaceChanged = false;

            pthread_cond_signal(&gAppContext->modeContext->SxrPresentationSurfaceChangedCV);
            pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);
        }

        if (gAppContext->modeContext->SxrPresentationWindowSurface != EGL_NO_SURFACE)
        {
            // Dummy render to new presentation Context
            SxrPresentationProcessFrame(presentationParams);

            gAppContext->modeContext->SxrPresentationLastFrameParams = presentationParams;
            gAppContext->modeContext->SxrPresentationLastFrameWarpedTimeStamp = presentationParams->frameSubmitTimeStamp;
            LOGV("SxrPresentation: PresentationThread finished iteration: %" PRIu64 ", frameSubmitTimeStamp: %" PRIu64 "",
            gAppContext->modeContext->SxrPresentationCount,
            presentationParams->frameSubmitTimeStamp);
        }
        gAppContext->modeContext->SxrPresentationCount++;
    }

    sxrDestroyPresentationContextFromNativeWindow();
    return 0;
}

//-----------------------------------------------------------------------------
bool sxrBeginPresentation()
//-----------------------------------------------------------------------------
{
    LOGI("sxrBeginPresentation");

    JNIEnv *pThreadJEnv;
    if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, 0) != JNI_OK)
    {
        LOGE("sxrBeginXr AttachCurrentThread failed.");
        return SXR_ERROR_JAVA_ERROR;
    }

    LOGV("SxrPresentation CallStaticVoidMethod(gAppContext->javaSvrApiClass, javaSxrEnablePresentation, gAppContext->javaActivityObject);");
    pThreadJEnv->CallStaticVoidMethod(gAppContext->javaSvrApiClass,
                                      gAppContext->javaSxrEnablePresentation,
                                      gAppContext->javaActivityObject, jboolean(true));
    if (pThreadJEnv->ExceptionCheck())
    {
        LOGE("CallStaticVoidMethod for enablePresentation triggered an exception");
        return SXR_ERROR_JAVA_ERROR;
    }

    gAppContext->modeContext->SxrPresentationCount = 0;
    gAppContext->modeContext->SxrPresentationWindowANativeWindow = nullptr;
    gAppContext->modeContext->SxrPresentationWindowDisplay = EGL_NO_DISPLAY;
    gAppContext->modeContext->SxrPresentationWindowContext = EGL_NO_CONTEXT;
    gAppContext->modeContext->SxrPresentationWindowSurface = EGL_NO_SURFACE;
    gAppContext->modeContext->SxrPresentationWindowSurfaceWidth = 0;
    gAppContext->modeContext->SxrPresentationWindowSurfaceHeight = 0;
    gAppContext->modeContext->SxrPresentationFBO = 0;
    gAppContext->modeContext->SxrPresentationCachedReadFBO = 0;
    gAppContext->modeContext->SxrPresentationThread = pthread_t();
    gAppContext->modeContext->SxrPresentationThreadId = pid_t();
    pthread_mutex_init(&gAppContext->modeContext->SxrPresentationReadyMutex, NULL);
    pthread_mutex_init(&gAppContext->modeContext->SxrPresentationFrameMutex, NULL);
    pthread_mutex_init(&gAppContext->modeContext->SxrPresentationSurfaceMutex, NULL);
    pthread_cond_init(&gAppContext->modeContext->SxrPresentationInitializedCv, NULL);
    pthread_cond_init(&gAppContext->modeContext->SxrPresentationFrameAvailableCV, NULL);
    pthread_cond_init(&gAppContext->modeContext->SxrPresentationSurfaceChangedCV, NULL);
    pthread_cond_init(&gAppContext->modeContext->SxrPresentationSurfaceDestroyedCV, NULL);
    gAppContext->modeContext->SxrPresentationThreadExit = false;
    gAppContext->modeContext->SxrPresentationSurfaceChanged = false;
    gAppContext->modeContext->SxrPresentationSurfaceDestroyed = false;
    gAppContext->modeContext->SxrPresentationThreadReady = false;
    gAppContext->modeContext->SxrPresentationPendingFrameParams = nullptr;
    gAppContext->modeContext->SxrPresentationPendingFrameTimeStamp = 0;
    gAppContext->modeContext->SxrPresentationLastFrameParams = nullptr;
    gAppContext->modeContext->SxrPresentationLastFrameWarpedTimeStamp = 0;

    int status = 0;

    // In case we need to change some attributes of the thread
    pthread_attr_t  threadAttribs;
    status = pthread_attr_init(&threadAttribs);
    if (status != 0)
    {
        LOGE("sxrBeginPresentation: Failed to initialize presentation thread attributes");
    }

    // For some reason, under 64-bit, we get this "warning" if we try to set the priority:
    //  "W libc    : pthread_create sched_setscheduler call failed: Operation not permitted"
    // It comes in as a warning, but the thread is NOT created!
#if defined(__ARM_ARCH_7A__)
    int lowestPriority = sched_get_priority_min(SCHED_FIFO);
    status = pthread_attr_setschedpolicy(&threadAttribs, SCHED_FIFO);
    if (status != 0)
    {
        LOGE("sxrBeginPresentation: Failed to set presentation thread attribute: SCHED_FIFO");
    }

    sched_param schedParam;
    memset(&schedParam, 0, sizeof(schedParam));
    schedParam.sched_priority = lowestPriority;
    status = pthread_attr_setschedparam(&threadAttribs, &schedParam);
    if (status != 0)
    {
        LOGE("sxrBeginPresentation: Failed to set presentation thread attribute: Priority");
    }
#endif  // defined(__ARM_ARCH_7A__)

    status = pthread_create(&gAppContext->modeContext->SxrPresentationThread, &threadAttribs, &sxrPresentationThreadMain, NULL);
    if (status != 0)
    {
        LOGE("sxrBeginPresentation: Failed to presentation warp thread");
    }

    pthread_setname_np(gAppContext->modeContext->SxrPresentationThread, "sxrPresentation");

    // No longer need the thread attributes
    status = pthread_attr_destroy(&threadAttribs);
    if (status != 0)
    {
        LOGE("sxrBeginPresentation: Failed to presentation warp thread attributes");
    }

    // Wait until the presentation thread is created and the context has been created before continuing
    LOGI("Waiting for presentation context creation...");
    pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationReadyMutex);
    while (!gAppContext->modeContext->SxrPresentationThreadReady)
    {
        pthread_cond_wait(&gAppContext->modeContext->SxrPresentationInitializedCv, &gAppContext->modeContext->SxrPresentationReadyMutex);
    }
    pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationReadyMutex);


    gAppContext->isSxrPresentationInitialized = true;

    return true;
}

//-----------------------------------------------------------------------------
bool sxrEndPresentation()
//-----------------------------------------------------------------------------
{
    LOGI("sxrEndPresentation");

    // Check global contexts
    if (gAppContext == NULL)
    {
        LOGE("Unable to end Presentation! Application context has been released!");
        return false;
    }

    if (gAppContext->modeContext == NULL)
    {
        LOGE("Unable to end Presentation: Called when not in VR mode!");
        return false;
    }

    JNIEnv *pThreadJEnv;
    if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, 0) != JNI_OK)
    {
        LOGE("sxrBeginXr AttachCurrentThread failed.");
        return SXR_ERROR_JAVA_ERROR;
    }
    LOGV("SxrPresentation CallStaticVoidMethod(gAppContext->javaSvrApiClass, javaSxrEnablePresentation, gAppContext->javaActivityObject, false);");
    pThreadJEnv->CallStaticVoidMethod(gAppContext->javaSvrApiClass,
                                      gAppContext->javaSxrEnablePresentation,
                                      gAppContext->javaActivityObject, jboolean(false));
    if (pThreadJEnv->ExceptionCheck())
    {
        LOGE("CallStaticVoidMethod for enablePresentation triggered an exception");
        return SXR_ERROR_JAVA_ERROR;
    }

    //Stop the warp thread
    LOGI("sxrEndPresentation - Setting SxrPresentationThreadExit flag to true");
    gAppContext->modeContext->SxrPresentationThreadExit = true;
    // Fire a dummy SxrPresentationFrameAvailableCV to allow the presentation thread to iterate
    // because it may be at a cond_wait for that signal
    LOGI("sxrEndPresentation - Fire dummy SxrPresentationFrameAvailableCV signal to unblock presentation thread");
    pthread_cond_signal(&gAppContext->modeContext->SxrPresentationFrameAvailableCV);
    LOGI("sxrEndPresentation - pthread_join on SxrPresentationThread");
    if (int32_t returnVal = pthread_join(gAppContext->modeContext->SxrPresentationThread, NULL) != 0)
    {
        LOGI("sxrEndPresentation - pthread_join ERROR %d", returnVal);
    }
    // Clean up any remaining presentation resources and leave variables in a sensible state.
    gAppContext->modeContext->SxrPresentationCount = 0;
    if (gAppContext->modeContext->SxrPresentationWindowANativeWindow != nullptr)
    {
        ANativeWindow_release(gAppContext->modeContext->SxrPresentationWindowANativeWindow);
        gAppContext->modeContext->SxrPresentationWindowANativeWindow = nullptr;
    }
    gAppContext->modeContext->SxrPresentationWindowDisplay = EGL_NO_DISPLAY;
    gAppContext->modeContext->SxrPresentationWindowContext = EGL_NO_CONTEXT;
    gAppContext->modeContext->SxrPresentationWindowSurface = EGL_NO_SURFACE;
    gAppContext->modeContext->SxrPresentationWindowSurfaceWidth = 0;
    gAppContext->modeContext->SxrPresentationWindowSurfaceHeight = 0;
    gAppContext->modeContext->SxrPresentationFBO = 0;
    gAppContext->modeContext->SxrPresentationCachedReadFBO = 0;
    gAppContext->modeContext->SxrPresentationThread = pthread_t();
    gAppContext->modeContext->SxrPresentationThreadId = pid_t();
    pthread_mutex_destroy(&gAppContext->modeContext->SxrPresentationReadyMutex);
    pthread_mutex_destroy(&gAppContext->modeContext->SxrPresentationFrameMutex);
    pthread_mutex_destroy(&gAppContext->modeContext->SxrPresentationSurfaceMutex);
    pthread_cond_destroy(&gAppContext->modeContext->SxrPresentationInitializedCv);
    pthread_cond_destroy(&gAppContext->modeContext->SxrPresentationFrameAvailableCV);
    pthread_cond_destroy(&gAppContext->modeContext->SxrPresentationSurfaceChangedCV);
    pthread_cond_destroy(&gAppContext->modeContext->SxrPresentationSurfaceDestroyedCV);
    gAppContext->modeContext->SxrPresentationThreadExit = false;
    gAppContext->modeContext->SxrPresentationSurfaceChanged = false;
    gAppContext->modeContext->SxrPresentationSurfaceDestroyed = false;
    gAppContext->modeContext->SxrPresentationThreadReady = false;
    gAppContext->modeContext->SxrPresentationPendingFrameParams = nullptr;
    gAppContext->modeContext->SxrPresentationPendingFrameTimeStamp = 0;
    gAppContext->modeContext->SxrPresentationLastFrameParams = nullptr;
    gAppContext->modeContext->SxrPresentationLastFrameWarpedTimeStamp = 0;

    gAppContext->isSxrPresentationInitialized = false;
    return true;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetPresentationSurfaceChanged(ANativeWindow* pWindow, int format, int width, int height)
//-----------------------------------------------------------------------------
{
    if ((gAppContext != nullptr) && (gAppContext->modeContext != nullptr) && (gAppContext->isSxrPresentationInitialized == true))
    {
        LOGI("sxrSetPresentationSurfaceChanged(%p, %i, %i, %i)", pWindow, format, width, height);
        pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);
        if (gAppContext->modeContext->SxrPresentationWindowANativeWindow != nullptr)
        {
            ANativeWindow_release(gAppContext->modeContext->SxrPresentationWindowANativeWindow);
        }
        gAppContext->modeContext->SxrPresentationWindowANativeWindow = pWindow;
        gAppContext->modeContext->SxrPresentationSurfaceChanged = true;
        gAppContext->modeContext->SxrPresentationWindowSurfaceWidth = width;
        gAppContext->modeContext->SxrPresentationWindowSurfaceHeight = height;
        pthread_cond_wait(&gAppContext->modeContext->SxrPresentationSurfaceChangedCV,
                &gAppContext->modeContext->SxrPresentationSurfaceMutex);
        pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);
    }
    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetPresentationSurfaceDestroyed()
//-----------------------------------------------------------------------------
{
    LOGI("sxrSetPresentationSurfaceDestroyed()");
    if ((gAppContext != nullptr) && (gAppContext->modeContext != nullptr) && (gAppContext->isSxrPresentationInitialized == true)) {
        LOGI("sxrSetPresentationSurfaceDestroyed - setting SxrPresentationSurfaceDestroyed flag to true");
        gAppContext->modeContext->SxrPresentationSurfaceDestroyed = true;
        LOGI("sxrSetPresentationSurfaceDestroyed - Locking SxrPresentationSurfaceMutex");
        if (pthread_mutex_lock(&gAppContext->modeContext->SxrPresentationSurfaceMutex) == 0)
        {
        LOGI("sxrSetPresentationSurfaceDestroyed - CV wait on SxrPresentationSurfaceDestroyedCV");
        int returnVal = pthread_cond_wait(&gAppContext->modeContext->SxrPresentationSurfaceDestroyedCV,
                          &gAppContext->modeContext->SxrPresentationSurfaceMutex);
        LOGI("sxrSetPresentationSurfaceDestroyed - CV wait on SxrPresentationSurfaceDestroyedCV returned %d", returnVal);
        LOGI("sxrSetPresentationSurfaceDestroyed - Unlocking SxrPresentationSurfaceMutex");
        pthread_mutex_unlock(&gAppContext->modeContext->SxrPresentationSurfaceMutex);
        }
    }
    return SXR_ERROR_NONE;
}
#endif // ENABLE_XR_CASTING

//-----------------------------------------------------------------------------
SxrResult sxrBeginXr(const sxrBeginParams* pBeginParams)
//-----------------------------------------------------------------------------
{
    LOGI("sxrBeginXr");

    gVrStartTimeNano = Svr::GetTimeNano();
    //m_poseBufferDesc.fd = -1;

#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gEnableRVR) {
        gRVRManager = new SxrServiceClientManager();
        if(gRVRManager) {
            SxrResult res = gRVRManager->Init();
            if(res != SXR_ERROR_NONE) {
                return res;
            }
        }
        sxrSetTrackingMode(kTrackingRotation | kTrackingPosition);
        if(gEnableRVRLagacy) {
            LOGI("[RVR] Lagacy mode : Disable time warp");
            gEnableTimeWarp = false;
        }
        gUseVSyncCallback = false;
        gUseLinePtr = false;
        gDisableReprojection = true;
        //gDisableTrackingRecenter = true;
        {
            std::lock_guard<std::mutex> lock (gRVRManagerInitMtx);
            gRVRManagerInit = true;
        }
    }
#endif

#ifdef ENABLE_CAMERA
    if(pBeginParams->optionFlags & kEnableCameraLayer)
    {
        LOGI("Camera layer enabled by application");
        gEnableCameraLayer = true;
    }
    else
    {
        gEnableCameraLayer = false;
    }

#endif

#ifdef ENABLE_MOTION_VECTORS
    // Motion vectors start out disabled
    gEnableMotionVectors = false;


    // Check if forced on globally...
    if (gForceAppEnableMotionVectors)
    {
        LOGI("Forcing motion vectors enabled by config file setting");
        gEnableMotionVectors = true;
    }


    // ... or enabled by the application
    if(pBeginParams->optionFlags & kMotionAwareFrames)
    {
        LOGI("Motion vectors enabled by application");
        gEnableMotionVectors = true;
    }

    // If motion vectors are enabled, make sure vsync is correct
    if (gEnableMotionVectors)
    {
        //gForceMinVsync = 2;
        //LOGI("Motion Vectors Enabled: Forcing minVsync = %d", gForceMinVsync);
    }
#endif

    if (gHeuristicPredictedTime)
    {
        if (gNumHeuristicEntries <= 0)
            gNumHeuristicEntries = 25;

        gHeuristicWriteIndx = 0;

        LOGI("Allocating %d entries for heuristic prediction time calculation...", gNumHeuristicEntries);
        gpHeuristicPredictData = new float[gNumHeuristicEntries];
        if (gpHeuristicPredictData == NULL)
        {
            LOGE("Unable to allocate %d entries for heuristic prediction time calculation!", gNumHeuristicEntries);
        }
        else
        {
            memset(gpHeuristicPredictData, 0, gNumHeuristicEntries * sizeof(float));
            for (int whichEntry = 0; whichEntry < gNumHeuristicEntries; whichEntry++)
            {
                gpHeuristicPredictData[whichEntry] = 0.0f;
            }
        }
    }

    if (gAppContext == NULL)
    {
        LOGE("svrBeginXr Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    PROFILE_THREAD_NAME(0, 0, "Eye Render Thread");

    //Set currently selected tracking mode. This is needed when the application resumes from suspension
    LOGI("Set tracking mode context...");
    sxrSetTrackingMode(gAppContext->currentTrackingMode);

    LOGI("Creating mode context...");
    gAppContext->modeContext = new SvrModeContext();

	gAppContext->modeContext->renderThreadId = gettid();
	gAppContext->modeContext->warpThreadId = -1;

    gAppContext->modeContext->nativeWindow = pBeginParams->nativeWindow;
    LOG_EGL_RESOURCE("%s:%i:threadid=%i, gAppContext->modeContext->nativeWindow=%p", __FILE__, __LINE__, gettid(), gAppContext->modeContext->nativeWindow);
    gAppContext->modeContext->isProtectedContent = (pBeginParams->optionFlags & kProtectedContent);

    switch(pBeginParams->colorSpace)
    {
        case kColorSpaceLinear:
            gAppContext->modeContext->colorspace = EGL_COLORSPACE_LINEAR;
            break;
        case kColorSpaceSRGB:
            gAppContext->modeContext->colorspace = EGL_COLORSPACE_sRGB;
            break;
        default:
            gAppContext->modeContext->colorspace = EGL_COLORSPACE_LINEAR;
            break;
    }

    gAppContext->modeContext->vsyncCount = 0;
    gAppContext->modeContext->vsyncTimeNano = 0;
    pthread_mutex_init(&gAppContext->modeContext->vsyncMutex, NULL);

    pthread_cond_init(&gAppContext->modeContext->warpThreadContextCv, NULL);
    pthread_mutex_init(&gAppContext->modeContext->warpThreadContextMutex, NULL);
    gAppContext->modeContext->warpContextCreated = false;

    gAppContext->modeContext->warpThreadExit = false;
    gAppContext->modeContext->vsyncThreadExit = false;

    pthread_cond_init(&gAppContext->modeContext->warpBufferConsumedCv, NULL);
    pthread_mutex_init(&gAppContext->modeContext->warpBufferConsumedMutex, NULL);

    gAppContext->modeContext->eyeRenderWarpSurface = EGL_NO_SURFACE;
    gAppContext->modeContext->eyeRenderOrigSurface = EGL_NO_SURFACE;
    gAppContext->modeContext->eyeRenderOrigConfigId = -1;
    gAppContext->modeContext->eyeRenderContext = EGL_NO_CONTEXT;
    gAppContext->modeContext->warpRenderContext = EGL_NO_CONTEXT;
    gAppContext->modeContext->warpRenderSurface = EGL_NO_SURFACE;
    gAppContext->modeContext->warpRenderSurfaceWidth = 0;
    gAppContext->modeContext->warpRenderSurfaceHeight = 0;

    //Initialize the warp frame param structures
    memset(&gAppContext->modeContext->frameParams[0], 0, sizeof(svrFrameParamsInternal) * NUM_SWAP_FRAMES);
    gAppContext->modeContext->submitFrameCount = 0;
    gAppContext->modeContext->warpFrameCount = 0;
    gAppContext->modeContext->prevSubmitVsyncCount = 0;

    // Recenter rotation
    gAppContext->modeContext->recenterRot = glm::fquat();
    gAppContext->modeContext->recenterPos = glm::vec3(0.0f, 0.0f, 0.0f);

    // Set the transform from Qvr to Sxr
    gAppContext->modeContext->qvrTransformMat = glm::mat4(1.0f);
    gAppContext->modeContext->qvrInverseMat = glm::mat4(1.0f);
    L_SetQvrTransform();

    // Create Event Manager
    gAppContext->modeContext->eventManager = new svrEventManager();

    // recover this flag if it is set to true before
    if (gAppContext->keepNotification)
    {
        gAppContext->keepNotification = false;
    }

    // Initialize frame counters
    gAppContext->modeContext->fpsPrevTimeMs = 0;
    gAppContext->modeContext->fpsFrameCounter = 0;

    memset(&gAppContext->modeContext->prevPoseState, 0, sizeof(sxrHeadPoseState));

#ifdef ENABLE_REMOTE_XR_RENDERING
    if(!gEnableRVR || gRVRUseQVRServicePose) {
#endif
    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrBeginXr Failed: SnapdragonXR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    QVRSERVICE_VRMODE_STATE serviceState;
    serviceState = QVRServiceClient_GetVRMode(gAppContext->qvrHelper);
    LOGI("sxrBeginXr: QvrServiceState is %s", QVRServiceClient_StateToName(serviceState));
    const int maxTries = 8;
    const int waitTime = 500000;
    int attempt = 0;
    int qRes;
    while ((serviceState != VRMODE_STARTED) && (attempt < maxTries))
    {
        if (attempt > 0)
            LOGE("sxrBeginXr called but VR service is in unexpected state, waiting... (attempt %d)", attempt);
        switch (serviceState)
        {
        case VRMODE_STOPPED:
            LOGI("sxrBeginXr: StartVRMode ...");
            qRes = QVRServiceClient_StartVRMode(gAppContext->qvrHelper);
            if (qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_StartVRMode", qRes);
            }
            break;
        case VRMODE_PAUSED:
            LOGI("sxrBeginXr: ResumeVRMode ...");
            qRes = QVRServiceClient_ResumeVRMode(gAppContext->qvrHelper);
            if (qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_ResumeVRMode", qRes);

                LOGI("sxrBeginXr: StopVRMode ...");
                qRes = QVRServiceClient_StopVRMode(gAppContext->qvrHelper);
                if (qRes != QVR_SUCCESS)
                {
                    L_LogQvrError("QVRServiceClient_StopVRMode", qRes);
                }
            }
            break;
        case VRMODE_UNSUPPORTED:
        case VRMODE_STARTING:
        case VRMODE_STARTED:
        case VRMODE_STOPPING:
        case VRMODE_HEADLESS:
        case VRMODE_PAUSING:
        case VRMODE_RESUMING:
        default:
            break;
        }
        usleep(waitTime);
        serviceState = QVRServiceClient_GetVRMode(gAppContext->qvrHelper);
        LOGI("sxrBeginXr: QvrServiceState is %s", QVRServiceClient_StateToName(serviceState));
        attempt++;
    }

    if (serviceState != VRMODE_STARTED)
    {
        LOGE("sxrBeginXr: VR service failed to start");
        delete gAppContext->modeContext;
        gAppContext->modeContext = 0;
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    // Check the current tracking mode
    QVRSERVICE_TRACKING_MODE currentMode;
    uint32_t supportedModes;
	qRes = QVRServiceClient_GetTrackingMode(gAppContext->qvrHelper, &currentMode, &supportedModes);
    if(qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_GetTrackingMode()", qRes);
    }
    else
    {
        switch(currentMode)
        {
        case TRACKING_MODE_NONE:
            LOGE("Current tracking mode: TRACKING_MODE_NONE");
            break;

        case TRACKING_MODE_ROTATIONAL:
            LOGI("Current tracking mode: TRACKING_MODE_ROTATIONAL");
            break;

        case TRACKING_MODE_POSITIONAL:
            LOGI("Current tracking mode: TRACKING_MODE_POSITIONAL");
            break;

        case TRACKING_MODE_ROTATIONAL_MAG:
            LOGI("Current tracking mode: TRACKING_MODE_ROTATIONAL_MAG");
            break;

        default:
            LOGE("Current tracking mode: Unknown = %d", currentMode);
            break;
        }

        // Says it is a mask, but the results are an enumeration!  This could be anything so just print it out.
        LOGI("Supported tracking mode mask: 0x%x", supportedModes);
    }

    // Check eye tracking support
    uint32_t currentEyeMode;
    uint32_t supportedEyeModes;
    qRes = QVRServiceClient_GetEyeTrackingMode(gAppContext->qvrHelper, &currentEyeMode, &supportedEyeModes);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_GetEyeTrackingMode", qRes);
    }
    else if (supportedEyeModes & QVRSERVICE_EYE_TRACKING_MODE_DUAL)
    {
        LOGE("sxrBeginXr: Eye Tracking Mode: enabled DUAL\n");
        m_eyeSyncCtrl = QVRServiceClient_GetSyncCtrl(gAppContext->qvrHelper, QVR_SYNC_SOURCE_EYE_POSE_CLIENT_READ);
        if (m_eyeSyncCtrl != NULL)
        {
            LOGE("sxrBeginXr: Eye sync: enabled\n");
        }
        else
        {
            LOGE("sxrBeginXr: Eye sync: disabled\n");
        }
    }

    // Ring Buffer Descriptor
    qRes = QVRServiceClient_GetRingBufferDescriptor(gAppContext->qvrHelper, RING_BUFFER_POSE, &m_poseBufferDesc);
    if(qRes != QVR_SUCCESS)
    {
        L_LogQvrError("GetRingBufferDescriptor()", qRes);
    }
    else
    {
        LOGI("Pose Ring Buffer:");
        LOGI("    fd: %d", m_poseBufferDesc.fd);
        LOGI("    size: %d", m_poseBufferDesc.size);
        LOGI("    index_offset: %d", m_poseBufferDesc.index_offset);
        LOGI("    ring_offset: %d", m_poseBufferDesc.ring_offset);
        LOGI("    element_size: %d", m_poseBufferDesc.element_size);
        LOGI("    num_elements: %d", m_poseBufferDesc.num_elements);
    }

	unsigned int retLen = 0;

    // **************************************
    LOGI("Reading Sample to Android time...");
    // **************************************
    qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_TRACKER_ANDROID_OFFSET_NS, &retLen, NULL);
    if(qRes != QVR_SUCCESS)
    {
		L_LogQvrError("QVRServiceClient_GetParam(QVRSERVICE_TRACKER_ANDROID_OFFSET_NS - Length)", qRes);
    }
    else
    {
        char *pRetValue = new char[retLen + 1];
        if(pRetValue == NULL)
        {
            LOGE("Unable to allocate %d bytes for return string!", retLen + 1);
        }
        else
        {
            memset(pRetValue, 0, retLen + 1);
			qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_TRACKER_ANDROID_OFFSET_NS, &retLen, pRetValue);
            if(qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_GetParam(QVRSERVICE_TRACKER_ANDROID_OFFSET_NS)", qRes);
            }
            else
            {
                gQTimeToAndroidBoot = strtoll(pRetValue, NULL, 0);

                LOGI("QTimer Android offset string: %s", pRetValue);
                LOGI("QTimer Android offset value: %lld", (long long int)gQTimeToAndroidBoot);
            }

            // No longer need the temporary buffer
            delete[] pRetValue;
        }
    }

    // **************************************
    LOGI("Reading QVR Service Version...");
    // **************************************
    qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SERVICE_VERSION, &retLen, NULL);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_GetParam(QVRSERVICE_SERVICE_VERSION - Length)", qRes);
    }
    else
    {
        char *pRetValue = new char[retLen + 1];
        if (pRetValue == NULL)
        {
            LOGE("Unable to allocate %d bytes for return string!", retLen + 1);
        }
        else
        {
            memset(pRetValue, 0, retLen + 1);
            qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SERVICE_VERSION, &retLen, pRetValue);
            if (qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_GetParam(QVRSERVICE_SERVICE_VERSION)", qRes);
            }
            else
            {
                LOGI("Qvr Service Version: %s", pRetValue);
            }

            // No longer need the temporary buffer
            delete[] pRetValue;
        }
    }

    // **************************************
    LOGI("Reading QVR Client Version...");
    // **************************************
    qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_CLIENT_VERSION, &retLen, NULL);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_GetParam(QVRSERVICE_CLIENT_VERSION - Length)", qRes);
    }
    else
    {
        char *pRetValue = new char[retLen + 1];
        if (pRetValue == NULL)
        {
            LOGE("Unable to allocate %d bytes for return string!", retLen + 1);
        }
        else
        {
            memset(pRetValue, 0, retLen + 1);
			qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_CLIENT_VERSION, &retLen, pRetValue);
            if (qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_GetParam(QVRSERVICE_CLIENT_VERSION)", qRes);
            }
            else
            {
                LOGI("Qvr Client Version: %s", pRetValue);
            }

            // No longer need the temporary buffer
            delete[] pRetValue;
        }
    }

    //Enable thermal notifications
    qRes = QVRServiceClient_RegisterForNotification(gAppContext->qvrHelper, NOTIFICATION_THERMAL_INFO, qvrClientThermalNotificationCallback, 0);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_RegisterForNotification Thermal:", qRes);
    }

    //Enable proximity notifications
    qRes = QVRServiceClient_RegisterForNotification(gAppContext->qvrHelper, NOTIFICATION_PROXIMITY_CHANGED, qvrClientProximityNotificationCallback, 0);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_RegisterForNotification Proximity:", qRes);
    }

    //Enable State notifications
    qRes = QVRServiceClient_RegisterForNotification(gAppContext->qvrHelper, NOTIFICATION_STATE_CHANGED, qvrClientStateNotificationCallback, 0);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_RegisterForNotification State:", qRes);
    }

    LOGI("QVRService: Service Initialized");


    if (gAppContext->qvrHelper)
    {
        LOGI("Calling SetThreadAttributesByType for RENDER thread...");
        QVRSERVICE_THREAD_TYPE threadType = gEnableRenderThreadFifo ? RENDER : NORMAL;
        qRes = QVRServiceClient_SetThreadAttributesByType(gAppContext->qvrHelper, gAppContext->modeContext->renderThreadId, threadType);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_SetThreadAttributesByType", qRes);
        }
    }
    else
    {
        LOGE("QVRServiceClient unavailable, SetThreadAttributesByType for RENDER thread failed.");
    }

	LOGI("Setting CPU/GPU Performance Levels...");
    sxrSetPerformanceLevels( pBeginParams->cpuPerfLevel, pBeginParams->gpuPerfLevel);

    //Start Vsync monitoring
    LOGI("Starting VSync Monitoring...");
    if (gUseVSyncCallback)
    {
        LOGI("Configuring QVR Display Interrupt Vsync ...");
        int qRes = QVRServiceClient_SetDisplayInterruptCapture(gAppContext->qvrHelper, DISP_INTERRUPT_VSYNC, 1);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_SetDisplayInterruptCapture(DISP_INTERRUPT_VSYNC)", qRes);
        }
        else
        {
            LOGI("    Started Display Interrupt VSync");
        }
    }
    else if(gUseLinePtr)
    {
        LOGI("Configuring QVR Display Interrupt LinePtr ...");
        int qRes = QVRServiceClient_SetDisplayInterruptCapture(gAppContext->qvrHelper, DISP_INTERRUPT_LINEPTR, 1);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_SetDisplayInterruptCapture(DISP_INTERRUPT_LINEPTR)", qRes);
        }
        else
        {
            LOGE("    Started Display Interrupt LinePtr");
        }
    }
    else
    {
        LOGI("Configuring Android Choreographer ...");
        //Utilize Choreographer for tracking HW Vsync
        JNIEnv* pThreadJEnv;
        if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, NULL) != JNI_OK)
        {
            LOGE("    sxrBeginXr AttachCurrentThread failed.");
        }

        LOGI("   Using Choreographer VSync Monitoring");
        pThreadJEnv->CallStaticVoidMethod(gAppContext->javaSvrApiClass, gAppContext->javaSvrApiStartVsyncMethodId, gAppContext->javaActivityObject);
    }

    if (gEnableTimeWarp)
    {
        LOGI("Starting Timewarp...");
        svrBeginTimeWarp();
    }
    else
    {
        LOGI("Timewarp not started.  Disabled by config file.");
    }

#ifdef ENABLE_XR_CASTING
    if (gEnableVRCasting)
    {
        sxrBeginPresentation();
    }
#endif // ENABLE_XR_CASTING

    if (gEnableDebugServer)
    {
        svrStartDebugServer();
    }

#ifdef ENABLE_REMOTE_XR_RENDERING
    }
    if(gEnableRVR) {
        if(gRVRManager) {
            if(gEnableRVRLagacy) {
                LOGI("[RVR] Initializing RVR context...");
                sxrDeviceInfo di = sxrGetDeviceInfo();
                SXRGLInfo info;
                info.w = di.displayWidthPixels;
                info.h = di.displayHeightPixels;
                info.nativeWindow = gAppContext->modeContext->nativeWindow;
                info.colorSpace = gAppContext->modeContext->colorspace;
                gRVRManager->SetNativeWindowInfo(info);
                LOGI("[RVR] Initializing RVR context done!");
            } else if(!gRVRUseQVRServicePose) {
                if (gEnableTimeWarp)
                {
                    LOGI("[RVR] Starting Timewarp...");
                    svrBeginTimeWarp();
                }
            }
        } else {
            LOGE("[RVR] RVRManager not initialize!");
        }
    }
#endif

    // Controller Manager
    gAppContext->controllerManager = new ControllerManager(gAppContext->javaVm,
        gAppContext->javaActivityObject,
        gAppContext->svrServiceClient,
        gAppContext->modeContext->eventManager,
        m_poseBufferDesc.fd,
        m_poseBufferDesc.size);
    gAppContext->controllerManager->Initialize(gControllerService, gControllerRingBufferSize);

    // Only now are we truly in VR mode
    gAppContext->inVrMode = true;

    //Signal VR Mode Active
    svrEventQueue* pQueue = gAppContext->modeContext->eventManager->AcquireEventQueue();
    sxrEventData data;
    pQueue->SubmitEvent(kEventVrModeStarted, data);

    data.thermal.zone = kCpu;
    data.thermal.level = kSafe;
    pQueue->SubmitEvent(kEventThermal, data);

    data.thermal.zone = kGpu;
    data.thermal.level = kSafe;
    pQueue->SubmitEvent(kEventThermal, data);

    data.thermal.zone = kSkin;
    data.thermal.level = kSafe;
    pQueue->SubmitEvent(kEventThermal, data);

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrEndXr()
//-----------------------------------------------------------------------------
{
    LOGI("sxrEndXr");
#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gRVRManager) {
        gRVRManager->Release();
        delete gRVRManager;
        gRVRManager = nullptr;
        gRVRManagerInit = false;
    }
    if (gAppContext == NULL || (!gEnableRVR && gAppContext->qvrHelper == NULL))
    {
        LOGE("Unable to end VR! Application context has been released!");
        return  SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }
#else
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("Unable to end VR! Application context has been released!");
        return  SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }
#endif

#ifdef ENABLE_XR_CASTING
    if (gEnableVRCasting && gAppContext->isSxrPresentationInitialized)
    {
        sxrEndPresentation();
    }
#endif

    svrEventQueue* pQueue = NULL;
    sxrEventData data;
    if (gAppContext->modeContext != NULL && gAppContext->modeContext->eventManager != NULL)
    {
        pQueue = gAppContext->modeContext->eventManager->AcquireEventQueue();
    }

    if (pQueue != NULL)
        pQueue->SubmitEvent(kEventVrModeStopping, data);

    // No longer in VR mode
    gAppContext->inVrMode = false;

    // Controller Manager Shutdown
    if (gAppContext->controllerManager != 0)
    {
        delete gAppContext->controllerManager;
        gAppContext->controllerManager = 0;
    }

    if (gpHeuristicPredictData != NULL)
    {
        gHeuristicWriteIndx = 0;

        LOGI("Deleting %d entries for heuristic prediction time calculation...", gNumHeuristicEntries);
        delete [] gpHeuristicPredictData;
        gpHeuristicPredictData = NULL;
    }


    if (gEnableTimeWarp)
    {
        LOGI("Stopping Timewarp...");
        svrEndTimeWarp();
    }
    else
    {
        LOGI("Not stopping Timewarp.  Was not started since disabled by config file.");
    }

    if (gUseVSyncCallback)
    {
        int qRes = QVRServiceClient_SetDisplayInterruptCapture(gAppContext->qvrHelper, DISP_INTERRUPT_VSYNC, 0);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_SetDisplayInterruptCapture(DISP_INTERRUPT_VSYNC) Off", qRes);
        }
        else
        {
            LOGI("Stopped Display Interrupt VSync");
        }
    }
    else if (gUseLinePtr)
    {
        int qRes = QVRServiceClient_SetDisplayInterruptCapture(gAppContext->qvrHelper, DISP_INTERRUPT_LINEPTR, 0);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_SetDisplayInterruptCapture(DISP_INTERRUPT_LINEPTR) Off", qRes);
        }
        else
        {
            LOGE("Stopped Display Interrupt LinePtr");
        }
    }
    else
    {
        LOGI("Stopping Choreographer VSync Monitoring...");
        JNIEnv* pThreadJEnv;
        if (gAppContext->javaVm->AttachCurrentThread(&pThreadJEnv, NULL) != JNI_OK)
        {
            LOGE("    sxrBeginXr AttachCurrentThread failed.");
        }
        pThreadJEnv->CallStaticVoidMethod(gAppContext->javaSvrApiClass, gAppContext->javaSvrApiStopVsyncMethodId, gAppContext->javaActivityObject);
    }

    int qRes;
    if (!gAppContext->keepNotification)
    {
        //Disable thermal notifications
        qRes = QVRServiceClient_RegisterForNotification(gAppContext->qvrHelper, NOTIFICATION_THERMAL_INFO, 0, 0);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_RegisterForNotification Thermal:", qRes);
        }

        //Disable proximity notifications
        qRes = QVRServiceClient_RegisterForNotification(gAppContext->qvrHelper, NOTIFICATION_PROXIMITY_CHANGED, 0, 0);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_RegisterForNotification Proximity:", qRes);
        }

        //Disable state change notifications
        qRes = QVRServiceClient_RegisterForNotification(gAppContext->qvrHelper, NOTIFICATION_STATE_CHANGED, 0, 0);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_RegisterForNotification StateChange:", qRes);
        }
    }

    if (!gUseQvrPerfModule)
	{
		if (gRenderThreadCore >= 0)
		{
			LOGI("Clearing Eye Render Affinity");
			svrClearThreadAffinity();
		}

		if (gEnableRenderThreadFifo)
		{
			L_SetThreadPriority("Render Thread", SCHED_NORMAL, gNormalPriorityRender);
		}
	}

    LOGI("Resetting CPU/GPU Performance Levels...");
    sxrSetPerformanceLevelsInternal(kPerfSystem, kPerfSystem);

    //LOGI("Stopping tracking...");
    //sxrSetTrackingMode(kTrackingRotation);

    // Stop Eye Tracking
    LOGI("Stopping eye tracking...");
    qRes = QVRServiceClient_SetEyeTrackingMode(gAppContext->qvrHelper, QVRSERVICE_EYE_TRACKING_MODE_NONE);
    //if (qRes != QVR_SUCCESS)
    //{
    //    L_LogQvrError("QVRServiceClient_SetEyeTrackingMode (Off)", qRes);
    //}

    if (m_eyeSyncCtrl != NULL)
    {
        QVRServiceClient_ReleaseSyncCtrl(gAppContext->qvrHelper, m_eyeSyncCtrl);
        m_eyeSyncCtrl = NULL;
    }

    // See QVRServiceClient_GetRingBufferDescriptor()
    if (m_poseBufferDesc.size > 0) //if (m_poseBufferDesc.fd > -1)
    {
        LOGI("Pose Ring Buffer: Closing fd %d", m_poseBufferDesc.fd);
        close(m_poseBufferDesc.fd);
        //m_poseBufferDesc.fd = -1;
        m_poseBufferDesc.size = 0;
    }

    LOGI("Disconnecting from QVR Service...");
    QVRSERVICE_VRMODE_STATE serviceState;
    serviceState = QVRServiceClient_GetVRMode(gAppContext->qvrHelper);
    LOGI("sxrEndXr: QvrServiceState is %s", QVRServiceClient_StateToName(serviceState));
    const int maxTries = 8;
    const int waitTime = 500000;
    int attempt = 0;

    while (!(serviceState == VRMODE_UNSUPPORTED || serviceState == VRMODE_HEADLESS || serviceState == VRMODE_STOPPED || (gLifecycleSuspendEnabled && serviceState == VRMODE_PAUSED)) && (attempt < maxTries))
    {
        if (attempt > 0)
            LOGE("sxrEndXr called but VR service is in unexpected state, waiting... (attempt %d)", attempt);
        switch (serviceState)
        {
        case VRMODE_STARTED:
            if (gLifecycleSuspendEnabled)
            {
                LOGI("sxrEndXr: PauseVRMode ...");
                qRes = QVRServiceClient_PauseVRMode(gAppContext->qvrHelper);
                if (qRes != QVR_SUCCESS)
                {
                    L_LogQvrError("QVRServiceClient_PauseVRMode", qRes);

                    LOGI("sxrEndXr: StopVRMode ...");
                    qRes = QVRServiceClient_StopVRMode(gAppContext->qvrHelper);
                    if (qRes != QVR_SUCCESS)
                    {
                        L_LogQvrError("QVRServiceClient_StopVRMode", qRes);
                    }
                }
            }
            else
            {
                LOGI("sxrEndXr: StopVRMode ...");
                qRes = QVRServiceClient_StopVRMode(gAppContext->qvrHelper);
                if (qRes != QVR_SUCCESS)
                {
                    L_LogQvrError("QVRServiceClient_StopVRMode", qRes);
                }
            }
            break;
        case VRMODE_PAUSED:
            if (!gLifecycleSuspendEnabled)
            {
                LOGI("sxrEndXr: StopVRMode ...");
                qRes = QVRServiceClient_StopVRMode(gAppContext->qvrHelper);
                if (qRes != QVR_SUCCESS)
                {
                    L_LogQvrError("QVRServiceClient_StopVRMode", qRes);
                }
            }
            break;
        case VRMODE_UNSUPPORTED:
        case VRMODE_STARTING:
        case VRMODE_STOPPING:
        case VRMODE_STOPPED:
        case VRMODE_HEADLESS:
            // When in headless mode, the gadget is disconnected. And the "serviceState"
            // is unknown. So we don't handle the situation in this case.
        case VRMODE_PAUSING:
        case VRMODE_RESUMING:
        default:
            break;
        }
        usleep(waitTime);
        serviceState = QVRServiceClient_GetVRMode(gAppContext->qvrHelper);
        LOGI("sxrEndXr: QvrServiceState is %s", QVRServiceClient_StateToName(serviceState));
        attempt++;

        // Handle headless state here
        if (gAppContext->keepNotification)
        {
            break;
        }
    }

    if (pQueue != NULL)
        pQueue->SubmitEvent(kEventVrModeStopped, data);

    //Delete the mode context
    //We can end up here with gAppContext->modeContext set to NULL
    if (gAppContext->modeContext != NULL)
    {
        //Clean up our synchronization primitives
        LOGI("Cleaning up thread synchronization primitives...");
        pthread_mutex_destroy(&gAppContext->modeContext->warpThreadContextMutex);
        pthread_cond_destroy(&gAppContext->modeContext->warpThreadContextCv);
        pthread_mutex_destroy(&gAppContext->modeContext->vsyncMutex);

        //Clean up any GPU fences still hanging around
        LOGI("Cleaning up frame fences...");
        for (int i = 0; i < NUM_SWAP_FRAMES; i++)
        {
            svrFrameParamsInternal& fp = gAppContext->modeContext->frameParams[i];
            if (fp.frameSync != 0)
            {
                glDeleteSync(fp.frameSync);
            }
            if (fp.motionVectorSync != 0)
            {
                glDeleteSync(fp.motionVectorSync);
            }
        }

        // If in headless mode, keep the eventManager so that we can restart VRMode later.
        if (!gAppContext->keepNotification)
        {
            //Clean up the event manager
            delete gAppContext->modeContext->eventManager;

            LOGI("Deleting mode context...");
            delete gAppContext->modeContext;
            gAppContext->modeContext = NULL;
        }
        else
        {
            LOGI("Headless: keep the eventManager");
        }
    }

    if (gEnableDebugServer)
    {
        svrStopDebugServer();
    }

    LOGI("VR mode ended");

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
void L_LogSubmitFrame(const sxrFrameParams* pFrameParams)
//-----------------------------------------------------------------------------
{
    LOGI("****************************************");
    LOGI("sxrSubmitFrame Parameters:");
    LOGI("    frameIndex = %d", pFrameParams->frameIndex);
    LOGI("    minVsyncs = %d", pFrameParams->minVsyncs);
    LOGI("    frameOptions = 0x%x", pFrameParams->frameOptions);
    LOGI("    warpType = %d", (int)pFrameParams->warpType);
    LOGI("    fieldOfView = %0.2f", pFrameParams->fieldOfView);
    LOGI("    headPoseState:");

    // Head Pose State
    const sxrHeadPoseState *pHeadPose = &pFrameParams->headPoseState;
    LOGI("        Rotation: [%0.4f, %0.4f, %0.4f, %0.4f]", pHeadPose->pose.rotation.x, pHeadPose->pose.rotation.y, pHeadPose->pose.rotation.z, pHeadPose->pose.rotation.w);
    LOGI("        Position: [%0.4f, %0.4f, %0.4f]", pHeadPose->pose.position.x, pHeadPose->pose.position.y, pHeadPose->pose.position.z);

    if(pHeadPose->poseStatus & kTrackingRotation)
        LOGI("        Rotation Valid");
    if (pHeadPose->poseStatus & kTrackingPosition)
        LOGI("        Position Valid");
    if (pHeadPose->poseStatus & kTrackingEye)
        LOGI("        Eye Tracking Valid");

    uint64_t timeNowNano = Svr::GetTimeNano();
    uint64_t diffTime = timeNowNano - pHeadPose->poseFetchTimeNs;
    float poseAge = ((float)diffTime * NANOSECONDS_TO_MILLISECONDS);
    LOGI("        Pose Age: %0.4f ms", poseAge);

    uint64_t expectedTime = pHeadPose->expectedDisplayTimeNs;
    uint64_t diffTimeNano = expectedTime - timeNowNano;
    float diffTimeMs = (float)diffTimeNano * NANOSECONDS_TO_MILLISECONDS;
    LOGI("        Expected Display Time: %0.4f ms", diffTimeMs);

    // Render Layers
    for (uint32_t whichLayer = 0; whichLayer < SXR_MAX_RENDER_LAYERS; whichLayer++)
    {
        if (pFrameParams->renderLayers[whichLayer].imageHandle == 0)
            continue;

        const sxrRenderLayer *pOneLayer = &pFrameParams->renderLayers[whichLayer];
        LOGI("    Render Layer %d:", whichLayer);
        LOGI("        imageHandle = %d", pOneLayer->imageHandle);

        switch (pOneLayer->imageType)
        {
        case kTypeTexture:
            LOGI("        imageType = kTypeTexture");
            break;
        case kTypeTextureArray:
            LOGI("        imageType = kTypeTextureArray");
            break;
        case kTypeImage:
            LOGI("        imageType = kTypeImage");
            break;
        case kTypeEquiRectTexture:
            LOGI("        imageType = kTypeEquiRectTexture");
            break;
        case kTypeEquiRectImage:
            LOGI("        imageType = kTypeEquiRectImage");
            break;
        case kTypeCubemapTexture:
            LOGI("        imageType = kTypeCubemapTexture");
            break;
        case kTypeVulkan:
            LOGI("        imageType = kTypeVulkan");
            break;
        default:
            LOGI("        imageType = Unknown (%d)", pOneLayer->imageType);
            break;
        }

        LOGI("        imageCoords:");
        LOGI("             LL: [%0.2f, %0.2f, %0.2f, %0.2f]", pOneLayer->imageCoords.LowerLeftPos[0], pOneLayer->imageCoords.LowerLeftPos[1], pOneLayer->imageCoords.LowerLeftPos[2], pOneLayer->imageCoords.LowerLeftPos[3]);
        LOGI("             LR: [%0.2f, %0.2f, %0.2f, %0.2f]", pOneLayer->imageCoords.LowerRightPos[0], pOneLayer->imageCoords.LowerRightPos[1], pOneLayer->imageCoords.LowerRightPos[2], pOneLayer->imageCoords.LowerRightPos[3]);
        LOGI("             UL: [%0.2f, %0.2f, %0.2f, %0.2f]", pOneLayer->imageCoords.UpperLeftPos[0], pOneLayer->imageCoords.UpperLeftPos[1], pOneLayer->imageCoords.UpperLeftPos[2], pOneLayer->imageCoords.UpperLeftPos[3]);
        LOGI("             UR: [%0.2f, %0.2f, %0.2f, %0.2f]", pOneLayer->imageCoords.UpperRightPos[0], pOneLayer->imageCoords.UpperRightPos[1], pOneLayer->imageCoords.UpperRightPos[2], pOneLayer->imageCoords.UpperRightPos[3]);
        LOGI("            LUV: [%0.2f, %0.2f, %0.2f, %0.2f]", pOneLayer->imageCoords.LowerUVs[0], pOneLayer->imageCoords.LowerUVs[1], pOneLayer->imageCoords.LowerUVs[2], pOneLayer->imageCoords.LowerUVs[3]);
        LOGI("            UUV: [%0.2f, %0.2f, %0.2f, %0.2f]", pOneLayer->imageCoords.UpperUVs[0], pOneLayer->imageCoords.UpperUVs[1], pOneLayer->imageCoords.UpperUVs[2], pOneLayer->imageCoords.UpperUVs[3]);

        switch (pOneLayer->eyeMask)
        {
        case kEyeMaskLeft:
            LOGI("        eyeMask = kEyeMaskLeft");
            break;
        case kEyeMaskRight:
            LOGI("        eyeMask = kEyeMaskRight");
            break;
        case kEyeMaskBoth:
            LOGI("        eyeMask = kEyeMaskBoth");
            break;
        default:
            LOGI("        eyeMask = Unknown (0x%x)", pOneLayer->eyeMask);
            break;
        }

        // kLayerFlagNone = 0x00000000,
        // kLayerFlagHeadLocked = 0x00000001,
        // kLayerFlagOpaque = 0x00000002
        // TODO: Better way to handle this as flags are added
        if (pOneLayer->layerFlags == 0)
            LOGI("        layerFlags = kLayerFlagNone");
        else if (pOneLayer->layerFlags == kLayerFlagHeadLocked)
            LOGI("        layerFlags = kLayerFlagHeadLocked");
        else if (pOneLayer->layerFlags == kLayerFlagOpaque)
            LOGI("        layerFlags = kLayerFlagOpaque");
        else if (pOneLayer->layerFlags == kLayerFlagHeadLocked + kLayerFlagOpaque)
            LOGI("        layerFlags = kLayerFlagHeadLocked + kLayerFlagOpaque");
        else
            LOGI("        layerFlags = 0x%x", pOneLayer->layerFlags);

        if (pOneLayer->imageType == kTypeVulkan)
        {
            LOGI("        vulkanInfo:");
            LOGI("             memsize: %d", pOneLayer->vulkanInfo.memSize);
            LOGI("             width: %d", pOneLayer->vulkanInfo.width);
            LOGI("             height: %d", pOneLayer->vulkanInfo.height);
            LOGI("             numMips: %d", pOneLayer->vulkanInfo.numMips);
            LOGI("             bytesPerPixel: %d", pOneLayer->vulkanInfo.bytesPerPixel);
            LOGI("             renderSemaphore: %d", pOneLayer->vulkanInfo.renderSemaphore);
        }

    }   // Each Render Layer

    LOGI("****************************************");
}

//-----------------------------------------------------------------------------
SxrResult sxrSubmitFrame(const sxrFrameParams* pFrameParams)
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL)
    {
        LOGE("svrSubmitFrame Failed: Called without VR initialized");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    if (gAppContext->inVrMode == false)
    {
        LOGE("svrSubmitFrame Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_STARTED;
    }

    if (gAppContext->modeContext == NULL)
    {
        LOGE("svrSubmitFrame Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_STARTED;
    }

    //Process the events for the frame
    //LOGI("EventManager ProcessEvents");
    PROFILE_ENTER(GROUP_WORLDRENDER, 0, "ProcessEvents");
    gAppContext->modeContext->eventManager->ProcessEvents();
    PROFILE_EXIT(GROUP_WORLDRENDER);

#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gEnableRVR && gEnableRVRLagacy) {
        if(gRVRManager) {
           gRVRManager->SubmitFrame(pFrameParams, &(gAppContext->modeContext->recenterRot), &(gAppContext->modeContext->recenterPos));
           return SXR_ERROR_NONE;
        } else {
           return SXR_ERROR_VRMODE_NOT_INITIALIZED;
        }
    }
#endif

    if (gLogSubmitFrame)
    {
        L_LogSubmitFrame(pFrameParams);
    }

    while (gDisableFrameSubmit)
    {
        sleep(1);
        continue;
    }

    if (gLogSubmitFps)
    {
        unsigned int currentTimeMs = GetTimeNano() * 1e-6;
        gAppContext->modeContext->fpsFrameCounter++;
        if (gAppContext->modeContext->fpsPrevTimeMs == 0)
        {
            gAppContext->modeContext->fpsPrevTimeMs = currentTimeMs;
        }
        if (currentTimeMs - gAppContext->modeContext->fpsPrevTimeMs > 1000)
        {
            float elapsedSec = (float)(currentTimeMs - gAppContext->modeContext->fpsPrevTimeMs) / 1000.0f;
            float currentFPS = (float)gAppContext->modeContext->fpsFrameCounter / elapsedSec;
            LOGI("FPS: %0.2f", currentFPS);

            gAppContext->modeContext->fpsFrameCounter = 0;
            gAppContext->modeContext->fpsPrevTimeMs = currentTimeMs;
        }
    }

    if (!gEnableTimeWarp)
    {
        // Since Timewarp has not been started, can't really wait for it to consume buffer :)
        // But put here so we can log FPS
        return SXR_ERROR_NONE;
    }

    unsigned int lastFrameCount = gAppContext->modeContext->submitFrameCount;
    unsigned int nextFrameCount = gAppContext->modeContext->submitFrameCount + 1;

    if (gLogPrediction)
    {
        float timeSinceVrStart = L_MilliSecondsSinceVrStart();

        uint64_t timeNowNano = Svr::GetTimeNano();
        uint64_t expectedTime = pFrameParams->headPoseState.expectedDisplayTimeNs;
        uint64_t diffTimeNano = expectedTime - timeNowNano;
        float diffTimeMs = (float)diffTimeNano * NANOSECONDS_TO_MILLISECONDS;
        // LOGI("Time Now = %llu; Expected Time = %llu; Diff Time = %llu", (long long unsigned int)timeNowNano, (long long unsigned int)expectedTime, (long long unsigned int)diffTimeNano);
        LOGI("(%0.3f ms) Fame %d submitted. Expected to be displayed in %0.2f ms ", timeSinceVrStart, pFrameParams->frameIndex, diffTimeMs);
    }

    svrFrameParamsInternal& fp = gAppContext->modeContext->frameParams[nextFrameCount % NUM_SWAP_FRAMES];
    fp.frameParams = *pFrameParams;

    // Need the frame index on the submitted frame to match
    fp.frameParams.frameIndex = nextFrameCount;

    if (gForceMinVsync > 0)
    {
        fp.frameParams.minVsyncs = gForceMinVsync;
    }

    // Make sure we have at least "1" here
    if (fp.frameParams.minVsyncs == 0)
        fp.frameParams.minVsyncs = 1;

    fp.externalSync = 0;
    fp.frameSubmitTimeStamp = 0;
    fp.warpFrameLeftTimeStamp = 0;
    fp.warpFrameRightTimeStamp = 0;
    fp.minVSyncCount = gAppContext->modeContext->prevSubmitVsyncCount + fp.frameParams.minVsyncs;

    if (gDisableReprojection)
    {
        fp.frameParams.frameOptions |= kDisableReprojection;
    }

#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gEnableRVR && !gEnableRVRLagacy) {
        fp.frameParams.frameOptions |= kDisableDistortionCorrection;
    }
#endif

    fp.frameSubmitTimeStamp = Svr::GetTimeNano();

    // Vulkan or GL we need to release the last one
    // If this is a Vulkan command buffer then we have a handle.
    // Determine it is Vulkan by looking at the first eyebuffer.
    if (pFrameParams->renderLayers[0].vulkanInfo.renderSemaphore != 0)
    {
        // This is a Vulkan frame
        fp.frameSync = 0;   // TimeWarp will handle the sync object
    }
    else
    {
        if (fp.frameSync != 0)
        {
            // LOGI("Deleting sync: %d...", (int)fp.frameSync);
            glDeleteSync(fp.frameSync);
        }

        // This is a GL Frame
        fp.frameSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        // LOGI("Created sync: %d", (int)fp.frameSync);

        PROFILE_ENTER(GROUP_WORLDRENDER, 0, "glFlush");
        glFlush();
        PROFILE_EXIT(GROUP_WORLDRENDER);
    }

    // This log lines up with the log in TimeWarp for which frame is actually warped
    // LOGI("DEBUG! Submitting Frame %d", nextFrameCount % NUM_SWAP_FRAMES);

    gAppContext->modeContext->submitFrameCount = nextFrameCount;
    LOGV("Submitting Frame : %d (Ring = %d) [minV=%llu curV=%llu]", gAppContext->modeContext->submitFrameCount, gAppContext->modeContext->submitFrameCount % NUM_SWAP_FRAMES, fp.minVSyncCount, gAppContext->modeContext->vsyncCount);

#ifdef ENABLE_MOTION_VECTORS
    // Since submitted, generate the motion vectors for this frame
    if (gEnableMotionVectors)
    {
        GenerateMotionData(&fp);
	if (fp.motionVectorSync != 0)
	{
	    glDeleteSync(fp.motionVectorSync);
	}
	fp.motionVectorSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();
    }
#endif // ENABLE_MOTION_VECTORS

    PROFILE_ENTER(GROUP_WORLDRENDER, 0, "Submit Frame : %d (Slot %d)", gAppContext->modeContext->submitFrameCount, nextFrameCount % NUM_SWAP_FRAMES);

    //Wait until the eyebuffer has been consumed
    while (true)
    {
        pthread_mutex_lock(&gAppContext->modeContext->warpBufferConsumedMutex);
        pthread_cond_wait(&gAppContext->modeContext->warpBufferConsumedCv, &gAppContext->modeContext->warpBufferConsumedMutex);
        pthread_mutex_unlock(&gAppContext->modeContext->warpBufferConsumedMutex);

        if (gAppContext->modeContext->warpFrameCount >= lastFrameCount )
        {
            //Make sure we maintain the minSync interval
            gAppContext->modeContext->prevSubmitVsyncCount = glm::max(gAppContext->modeContext->vsyncCount, gAppContext->modeContext->prevSubmitVsyncCount + fp.frameParams.minVsyncs);

            LOGV("Finished : %d [%llu]", gAppContext->modeContext->submitFrameCount, gAppContext->modeContext->vsyncCount);
            break;
        }
    }

    PROFILE_EXIT(GROUP_WORLDRENDER);

	PROFILE_TICK();

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetPerformanceLevels(sxrPerfLevel cpuPerfLevel, sxrPerfLevel gpuPerfLevel)
//-----------------------------------------------------------------------------
{
	//Currently as of 8998 LA 2.0 kPerfSystem doesn't deliver enough performance for even simple VR use cases
	//so if kPerfSystem has been specified by the app default to kPerfMedium instead
	if (cpuPerfLevel == kPerfSystem)
	{
		cpuPerfLevel = kPerfMedium;
	}

	if (gpuPerfLevel == kPerfSystem)
	{
		gpuPerfLevel = kPerfMedium;
	}

    // Apply the override here, rather than in sxrSetPerformanceLevelsInternal
    cpuPerfLevel = (gForceCpuLevel < 0) ? cpuPerfLevel : (sxrPerfLevel)gForceCpuLevel;
    gpuPerfLevel = (gForceGpuLevel < 0) ? gpuPerfLevel : (sxrPerfLevel)gForceGpuLevel;


	return sxrSetPerformanceLevelsInternal(cpuPerfLevel, gpuPerfLevel);
}

//-----------------------------------------------------------------------------
SXRP_EXPORT float sxrGetPredictedDisplayTimePipelined(unsigned int depth)
//-----------------------------------------------------------------------------
{
    if (!gEnableTimeWarp)
    {
        // Since Timewarp has not been started, can't really get a predicted time :)
        return 0.0f;
    }

    if (gDisablePredictedTime)
    {
        return 0.0f;
    }

    if (gHeuristicPredictedTime)
    {
        return L_GetHeuristicPredictTime();
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false)
    {
        LOGE("sxrGetPredictedDisplayTime Failed: Called when not in VR mode!");
        return 0.0f;
    }

    if (gAppContext->modeContext == NULL)
    {
        LOGE("sxrGetPredictedDisplayTime Failed: Called when not in VR mode!");
        return 0.0f;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrGetPredictedDisplayTime Failed: SnapdragonVR not initialized!");
        return 0.0f;
    }

    // Need number of nanoseconds per VSync interval for later calculations
    uint64_t nanosPerVSync = 1000000000LL / (uint64_t)gAppContext->deviceInfo.displayRefreshRateHz;
    double msPerVSync = (double)nanosPerVSync * NANOSECONDS_TO_MILLISECONDS;

    // Now start building up how long until the frame should be displayed.
    // First, we have to get to the next VSync time ...
    double timeToVsync = L_MilliSecondsToNextVSync();

    // ... add in the pipeline depth (need depth + 1 so correct pose on screen the longest) ...
    double pipelineDepth = depth * msPerVSync;

    // ... then we add time to half exposure (already in milliseconds)...
    double timeToHalfExp = gTimeToHalfExposure;

    // ... so warp is split between each eye
    double timeToMidEye = gTimeToMidEyeWarp;

    // Sum up all the times for the total
    double predictedTime = timeToVsync + pipelineDepth + timeToHalfExp + timeToMidEye;

    if (gLogPrediction)
    {
        float timeSinceVrStart = L_MilliSecondsSinceVrStart();
        LOGI("(%0.3f ms) Predicted Display Time: %0.2f ms (To VSync = %0.2f; Pipeline = %0.2f; Half Exposure = %0.2f; Mid Eye = %0.2f)", timeSinceVrStart, predictedTime, timeToVsync, pipelineDepth, timeToHalfExp, timeToMidEye);
    }

    // OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD!
    {
        // double framePeriodNano = 1e9 / gAppContext->deviceInfo.displayRefreshRateHz;
        //
        // double framePct = (double)gAppContext->modeContext->vsyncCount + ((double)(timestamp - latestVsyncTimeNano) / (framePeriodNano));
        // double fractFrame = framePct - ((long)framePct);
        //
        // // (Time left until the next vsync) +
        // // ( pipeline depth * framePerid) +
        // // ( fixed display latency ) +
        // // ( 1/4 through next vysnc interval )
        // // double oldPredictedTime = MIN((framePeriodNano / 2.0), (framePeriodNano - (fractFrame * framePeriodNano))) + (depth * framePeriodNano) + (gFixedDisplayDelay * framePeriodNano) + (0.25 * framePeriodNano);
        // double oldPredictedTime = MIN((framePeriodNano / 2.0), (framePeriodNano - (fractFrame * framePeriodNano)));
        // oldPredictedTime += (depth * framePeriodNano);
        // // oldPredictedTime += (gFixedDisplayDelay * framePeriodNano);
        // oldPredictedTime += (0.25 * framePeriodNano);
        //
        // float oldRetValue = oldPredictedTime * 1e-6;
        //
        // LOGE("Old Predicted Time: %0.4f; New Predicted Time: %0.4f", oldRetValue, predictedTime);
    }
    // OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD! OLD METHOD!

    if (predictedTime < 0.0f)
    {
        LOGE("sxrGetPredictedDisplayTime: Predicted display time is negative! (%0.2f ms)", predictedTime);
        // LOGE("       vSyncCount = %llu", gAppContext->modeContext->vsyncCount);
        // LOGE("          TimeNow = %llu", timestamp);
        // LOGE("    vSyncTimeNano = %llu", gAppContext->modeContext->vsyncTimeNano);
        // LOGE("      FramePeriod = %0.4f", framePeriodNano);
        // LOGE("         framePct = %0.4f", framePct);
        // LOGE("       fractFrame = %0.4f", fractFrame);
        predictedTime = 0.0f;
    }

    return predictedTime;
}


//-----------------------------------------------------------------------------
float sxrGetPredictedDisplayTime()
//-----------------------------------------------------------------------------
{
    if (gDisablePredictedTime)
    {
        return 0.0f;
    }

    if (gHeuristicPredictedTime)
    {
        return L_GetHeuristicPredictTime();
    }

    return sxrGetPredictedDisplayTimePipelined(1);
}

//-----------------------------------------------------------------------------
SxrResult sxrGetXrServiceVersion(char *pRetBuffer, int bufferSize)
//-----------------------------------------------------------------------------
{
    int32_t qvrReturn;
    unsigned int retLen = 0;

    if (pRetBuffer == NULL)
    {
        return SXR_ERROR_UNKNOWN;
    }
    memset(pRetBuffer, 0, bufferSize);

    if (gAppContext == NULL)
    {
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    qvrReturn = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SERVICE_VERSION, &retLen, NULL);
    if (qvrReturn != QVR_SUCCESS)
    {
        L_LogQvrError("sxrGetXrServiceVersion(Length)", qvrReturn);

        SxrResult retCode = SXR_ERROR_UNKNOWN;
        switch (qvrReturn)
        {
        case QVR_ERROR:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        case QVR_CALLBACK_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_API_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_INVALID_PARAM:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        default:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        }

        return retCode;
    }

    // Will this fit in the return buffer?
    if ((int)retLen >= bufferSize - 1)
    {
        LOGE("sxrGetXrServiceVersion passed return buffer that is too small! Need %d bytes, buffer is %d bytes.", retLen + 1, bufferSize);
        return SXR_ERROR_UNKNOWN;
    }


    qvrReturn = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SERVICE_VERSION, &retLen, pRetBuffer);
    if (qvrReturn != QVR_SUCCESS)
    {
        L_LogQvrError("sxrGetXrServiceVersion()", qvrReturn);

        SxrResult retCode = SXR_ERROR_UNKNOWN;
        switch (qvrReturn)
        {
        case QVR_ERROR:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        case QVR_CALLBACK_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_API_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_INVALID_PARAM:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        default:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        }

        return retCode;
    }

    // Read the string correctly
    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrGetXrClientVersion(char *pRetBuffer, int bufferSize)
//-----------------------------------------------------------------------------
{
    int32_t qvrReturn;
    unsigned int retLen = 0;

    if (pRetBuffer == NULL)
    {
        return SXR_ERROR_UNKNOWN;
    }
    memset(pRetBuffer, 0, bufferSize);

    if (gAppContext == NULL)
    {
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    qvrReturn = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_CLIENT_VERSION, &retLen, NULL);
    if (qvrReturn != QVR_SUCCESS)
    {
        L_LogQvrError("svrGetVrClientVersion(Length)", qvrReturn);

        SxrResult retCode = SXR_ERROR_UNKNOWN;
        switch (qvrReturn)
        {
        case QVR_ERROR:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        case QVR_CALLBACK_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_API_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_INVALID_PARAM:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        default:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        }

        return retCode;
    }

    // Will this fit in the return buffer?
    if ((int)retLen >= bufferSize - 1)
    {
        LOGE("svrGetVrClientVersion passed return buffer that is too small! Need %d bytes, buffer is %d bytes.", retLen + 1, bufferSize);
        return SXR_ERROR_UNKNOWN;
    }


    qvrReturn = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_CLIENT_VERSION, &retLen, pRetBuffer);
    if (qvrReturn != QVR_SUCCESS)
    {
        L_LogQvrError("svrGetVrClientVersion()", qvrReturn);

        SxrResult retCode = SXR_ERROR_UNKNOWN;
        switch (qvrReturn)
        {
        case QVR_ERROR:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        case QVR_CALLBACK_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_API_NOT_SUPPORTED:
            retCode = SXR_ERROR_UNSUPPORTED;
            break;
        case QVR_INVALID_PARAM:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        default:
            retCode = SXR_ERROR_UNKNOWN;
            break;
        }

        return retCode;
    }

    // Read the string correctly
    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetXrStart()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrSetXrStart Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    LOGI("sxrSetXrStart: StartVRMode ...");
    int32_t qRes = QVRServiceClient_StartVRMode(gAppContext->qvrHelper);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_StartVRMode", qRes);
        return SXR_ERROR_UNKNOWN;
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetXrStop()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrSetXrStop Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    LOGI("sxrSetXrStop: StopVRMode ...");
    int32_t qRes = QVRServiceClient_StopVRMode(gAppContext->qvrHelper);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_StopVRMode", qRes);
        return SXR_ERROR_UNKNOWN;
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetXrPause()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrSetXrPause Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    LOGI("sxrSetXrPause: PauseVRMode ...");
    int32_t qRes = QVRServiceClient_PauseVRMode(gAppContext->qvrHelper);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_PauseVRMode", qRes);
        return SXR_ERROR_UNKNOWN;
    }

#ifdef ENABLE_CAMERA
    gSvrCameraManager.Pause(); // pause color camera
#endif

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrSetXrResume()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrSetXrResume Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    LOGI("sxrSetXrResume: ResumeVRMode ...");
    int32_t qRes = QVRServiceClient_ResumeVRMode(gAppContext->qvrHelper);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_ResumeVRMode", qRes);
        return SXR_ERROR_UNKNOWN;
    }

#ifdef ENABLE_CAMERA
    gSvrCameraManager.Resume(); // resume color camera
#endif

    return SXR_ERROR_NONE;
}

SxrResult sxrGetQvrDataTransform(sxrMatrix *pQvrTransform)
{
    if (pQvrTransform == NULL)
    {
        LOGE("sxrGetQvrDataTransform Failed: Called with NULL parameter");
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false || gAppContext->modeContext == NULL)
    {
        LOGE("sxrGetQvrDataTransform Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    ////memcpy(glm::value_ptr(gAppContext->modeContext->qvrTransformMat), pQvrTransform, sizeof(pQvrTransform));
    float *qvrTransform = glm::value_ptr(gAppContext->modeContext->qvrTransformMat);
    for (int i = 0; i < 16; i++) pQvrTransform->M[i] = qvrTransform[i];

    //memcpy(glm::value_ptr(gAppContext->modeContext->qvrTransformMat), pQvrTransform->M, 16*sizeof(float));

    return SXR_ERROR_NONE;
}

SxrResult sxrGetQvrDataInverse(sxrMatrix *pQvrTransform)
{
    if (pQvrTransform == NULL)
    {
        LOGE("sxrGetQvrDataTransform Failed: Called with NULL parameter");
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false || gAppContext->modeContext == NULL)
    {
        LOGE("sxrGetQvrDataTransform Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    ////memcpy(glm::value_ptr(gAppContext->modeContext->qvrTransformMat), pQvrTransform, sizeof(pQvrTransform));
    float *qvrInvTransform = glm::value_ptr(gAppContext->modeContext->qvrInverseMat);
    for (int i = 0; i < 16; i++) pQvrTransform->M[i] = qvrInvTransform[i];

    //memcpy(glm::value_ptr(gAppContext->modeContext->qvrTransformMat), pQvrTransform->M, 16*sizeof(float));

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
void L_SetQvrTransform()
//-----------------------------------------------------------------------------
{
    glm::mat4 m(1.0f);

    //m[0][0] = m[1][1] = m[2][2] = 0.2f;

    glm::quat rot = glm::quat(glm::vec3(glm::radians(gSensorOrientationCorrectX), glm::radians(gSensorOrientationCorrectY), glm::radians(gSensorOrientationCorrectZ)));
    m = m * glm::mat4(rot);

    // The matrix represents the CORRECTION to a rotation.
    // So the "Left" matrix is really a "Right" rotation matrix.
    // R(Theta) = | Cos(Theta)  -Sin(Theta) |
    //            | Sin(Theta)   Cos(Theta) |

    float LandRotLeft[16] = {
        0.0f,  1.0f,  0.0f,  0.0f,
        -1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  0.0f,
        0.0f,  0.0f,  0.0f,  1.0f };

    float LandRotLeftInv[16] = {
        0.0f, -1.0f,  0.0f,  0.0f,
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  0.0f,
        0.0f,  0.0f,  0.0f,  1.0f };

    // Z 90 degrees
    float LandRotRight[16] = {
        0.0f, -1.0f,  0.0f,  0.0f,
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  0.0f,
        0.0f,  0.0f,  0.0f,  1.0f };

    float LandRotRightInv[16] = {
        0.0f,  1.0f,  0.0f,  0.0f,
        -1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  0.0f,
        0.0f,  0.0f,  0.0f,  1.0f };

    glm::mat4 input_glm = m;

    if (gAppContext->deviceInfo.displayOrientation == 90)
    {
        // Landscape Left
        LOGI("Correcting for Landscape Left!");
        glm::mat4 LandRotLeft_glm = glm::make_mat4(LandRotLeft);
        glm::mat4 LandRotLeftInv_glm = glm::make_mat4(LandRotLeftInv);
        m = LandRotLeft_glm * input_glm * LandRotLeftInv_glm;
    }
    else if (gAppContext->deviceInfo.displayOrientation == 270)
    {
        // Landscape Right
        LOGI("Correcting for Landscape Right!");
        glm::mat4 LandRotRight_glm = glm::make_mat4(LandRotRight);
        glm::mat4 LandRotRightInv_glm = glm::make_mat4(LandRotRightInv);
        m = LandRotRight_glm * input_glm * LandRotRightInv_glm;

        // If we are running on Android-M (23) then the sensor rotation correction is different.
        // Android-N (24) is the new rotation.
        if (gAppContext->deviceInfo.deviceOSVersion < 24)
        {
            // For "M":
            // Need an extra 180 degrees around X
            glm::mat4 t = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
            m = m * t;
        }
        else
        {
            // For "N":
            // X: Rotated 180 around Final Y
            // Y: Rotated 180 around final Z (Look Blue, right is red, green down, purple up)
            // Z: Correct starting rotation of looking down -Z
            glm::mat4 t = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(0.0f, 0.0f, 1.0f));
            m = m * t;
        }
    }
    else if (gAppContext->deviceInfo.displayOrientation == 0)
    {
        // Landscape left: sensor input Landscape
        LOGI("Correcting for Landscape : orientation 0 degree!");
        glm::mat4 LandRotLeft_glm = glm::make_mat4(LandRotLeft);
        glm::mat4 LandRotLeftInv_glm = glm::make_mat4(LandRotLeftInv);
        m = LandRotLeft_glm * input_glm * LandRotLeftInv_glm;
    }
    else if (gAppContext->deviceInfo.displayOrientation == 180)
    {
        // Landscape right: sensor input Landscape
        LOGI("Correcting for Landscape : orientation 180 degree!");
        glm::mat4 LandRotRight_glm = glm::make_mat4(LandRotRight);
        glm::mat4 LandRotRightInv_glm = glm::make_mat4(LandRotRightInv);
        m = LandRotRight_glm * input_glm * LandRotRightInv_glm;

        // Need an extra 180 degrees around Z
        glm::mat4 t = glm::rotate(glm::mat4(1.0f), ((float)M_PI), glm::vec3(0.0f, 0.0f, 1.0f));
        m = m * t;
    }

    // QVR to SXR
    // Need an extra 90 degrees around Z
    //glm::mat4 t = glm::rotate(glm::mat4(1.0f), ((float)M_PI_2), glm::vec3(0.0f, 0.0f, 1.0f));
    //m = m * t;

    glm::vec3 trans = glm::vec3(gSensorHeadOffsetX, gSensorHeadOffsetY, gSensorHeadOffsetZ);
    m = glm::translate(m, trans);

    glm::fquat rotation(m);
    glm::vec3 euler = eulerAngles(rotation);
    glm::vec3 position = glm::vec3(m[3]);
    LOGI("    Qvr Transform Orientation: (%0.2f, %0.2f, %0.2f, %0.2f) => Euler(%0.2f, %0.2f, %0.2f)", rotation.x, rotation.y, rotation.z, rotation.w, glm::degrees(euler.x), glm::degrees(euler.y), glm::degrees(euler.z));
    LOGI("    Qvr Transform Position: (%0.2f, %0.2f, %0.2f)", position.x, position.y, position.z);

    gAppContext->modeContext->qvrTransformMat = m;
    gAppContext->modeContext->qvrInverseMat = glm::affineInverse(m);
}

//-----------------------------------------------------------------------------
void L_SetPoseState(sxrHeadPoseState &poseState, float predictedTimeMs)
//-----------------------------------------------------------------------------
{
    // Need glm versions for math
    glm::fquat poseRot;
    glm::vec3 posePos;

    poseRot.x = poseState.pose.rotation.x;
    poseRot.y = poseState.pose.rotation.y;
    poseRot.z = poseState.pose.rotation.z;
    poseRot.w = poseState.pose.rotation.w;

    posePos.x = poseState.pose.position.x;
    posePos.y = poseState.pose.position.y;
    posePos.z = poseState.pose.position.z;

    // Adjust by the recenter value
    poseRot = poseRot * gAppContext->modeContext->recenterRot;

    if (gAppContext->currentTrackingMode & kTrackingPosition)
    {
        // If no actual 6DOF camera the positions come back as NaN
        posePos = posePos - gAppContext->modeContext->recenterPos;

        // Need to adjust this new position by the rotation correction
        posePos = posePos * gAppContext->modeContext->recenterRot;
    }

    // Come back out of glm
    poseState.pose.rotation.x = poseRot.x;
    poseState.pose.rotation.y = poseRot.y;
    poseState.pose.rotation.z = poseRot.z;
    poseState.pose.rotation.w = poseRot.w;

    poseState.pose.position.x = posePos.x;
    poseState.pose.position.y = posePos.y;
    poseState.pose.position.z = posePos.z;

    // Don't set this since the time is already set in the structure...
    //poseState.poseTimeStampNs = Svr::GetTimeNano();

    // ... however, we need to adjust for DSP time
    poseState.poseTimeStampNs -= gQTimeToAndroidBoot;
#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gEnableRVR && gRVRUseQVRServicePose) {
        poseState.poseTimeStampNs += gQTimeToAndroidBoot;
        poseState.poseTimeStampNs = poseState.poseTimeStampNs/1000;
    }
#endif
    // When did we fetch this pose
    uint64_t timeNowNano = Svr::GetTimeNano();
    poseState.poseFetchTimeNs = timeNowNano;

    // When do we expect this pose to show up on the display
    poseState.expectedDisplayTimeNs = 0;
    if (!gDisablePredictedTime && predictedTimeMs > 0.0f)
    {
        poseState.expectedDisplayTimeNs = timeNowNano + (uint64_t)(predictedTimeMs * MILLISECONDS_TO_NANOSECONDS);
    }
}

//-----------------------------------------------------------------------------
sxrHeadPoseState sxrGetPredictedHeadPose(float predictedTimeMs)
//-----------------------------------------------------------------------------
{
    sxrHeadPoseState poseState;

    glm::fquat poseRot;     // Identity
    glm::vec3  posePos;     // Identity

    poseState.poseStatus = 0;

	poseState.pose.rotation.x = poseRot.x;
	poseState.pose.rotation.y = poseRot.y;
	poseState.pose.rotation.z = poseRot.z;
	poseState.pose.rotation.w = poseRot.w;

	poseState.pose.position.x = posePos.x;
	poseState.pose.position.y = posePos.y;
	poseState.pose.position.z = posePos.z;

    if (gAppContext == NULL)
    {
        LOGE("sxrGetPredictedHeadPose Failed: SnapdragonVR not initialized!");
		return poseState;
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false)
    {
        LOGE("sxrGetPredictedHeadPose Failed: Called when not in VR mode!");
        return poseState;
    }
#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gEnableRVR && !gRVRUseQVRServicePose && gRVRManager) {
        gRVRManager->GetHeadPose(poseState);
        predictedTimeMs = 0.0f;
    }
    else {
#endif
    if (gAppContext->modeContext == NULL)
    {
        LOGE("sxrGetPredictedHeadPose Failed: Called when not in VR mode!");
        return poseState;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrGetPredictedHeadPose Failed: QVR service not initialized!");
        return poseState;
    }

    uint64_t sampleTimeStamp;

    // Set the pose status and let it be changed by quality settings
    poseState.poseStatus = 0;
    if((gAppContext->currentTrackingMode & kTrackingRotation) != 0)
        poseState.poseStatus |= kTrackingRotation;
    if ((gAppContext->currentTrackingMode & kTrackingPosition) != 0)
        poseState.poseStatus |= kTrackingPosition;

    if (SXR_ERROR_NONE != GetTrackingFromPredictiveSensor(predictedTimeMs, &sampleTimeStamp, poseState))
    {
        // LOGE("sxrGetPredictedHeadPose Failed: QVR service not initialized!");
        poseState.poseStatus = 0;
        return poseState;
    }
#ifdef ENABLE_REMOTE_XR_RENDERING
    }
#endif

    // Assign to return value after adjusting for recenter
    L_SetPoseState(poseState, predictedTimeMs);


    if(gAppContext->modeContext->prevPoseState.poseTimeStampNs == 0)
    {
        //First time through
        gAppContext->modeContext->prevPoseState = poseState;
    }
    else
    {
        glm::fquat prevPoseQuat = glm::fquat(gAppContext->modeContext->prevPoseState.pose.rotation.w, gAppContext->modeContext->prevPoseState.pose.rotation.x, gAppContext->modeContext->prevPoseState.pose.rotation.y, gAppContext->modeContext->prevPoseState.pose.rotation.z);
        glm::fquat poseQuat = glm::fquat(poseState.pose.rotation.w, poseState.pose.rotation.x, poseState.pose.rotation.y, poseState.pose.rotation.z);
        poseQuat = glm::normalize(poseQuat);
        glm::fquat inversePrev = glm::conjugate(prevPoseQuat);
        glm::fquat diffValue = poseQuat * inversePrev;
        float diffRad = acosf(diffValue.w) * 2.0f;

        if(isnan(diffRad) || isinf(diffRad))
        {
            diffRad = 0.0f;
        }

        float diffTimeSeconds = ((poseState.poseTimeStampNs - gAppContext->modeContext->prevPoseState.poseTimeStampNs) * 1e-9);
        float angVelocity = 0.0f;
        float linVelocity = 0.0f;

        if(diffTimeSeconds > 0.0f)
        {
            //Angular velocity (degrees per second)
            angVelocity= (diffRad * RAD_TO_DEG) / diffTimeSeconds;

            //Positional velocity
            glm::vec3 prevToCurVec = glm::vec3( poseState.pose.position.x - gAppContext->modeContext->prevPoseState.pose.position.x,
                                                poseState.pose.position.y - gAppContext->modeContext->prevPoseState.pose.position.y,
                                                poseState.pose.position.z - gAppContext->modeContext->prevPoseState.pose.position.z );
            float length = glm::length(prevToCurVec);
            linVelocity = length / diffTimeSeconds;
        }

        if(gLogPoseVelocity)
        {
            LOGI("Ang Vel %f (deg/s): Lin Vel : %f (m/s)", angVelocity, linVelocity);
        }

        if (gLogPoseVelocity && angVelocity > gMaxAngVel)
        {
            LOGE("sxrGetPredictedHeadPose, angular velocity exceeded max of %f : %f", gMaxAngVel, angVelocity);
        }

        if (gLogPoseVelocity && linVelocity > gMaxLinearVel)
        {
            LOGE("sxrGetPredictedHeadPose, linear velocity exceeded max of %f : %f", gMaxLinearVel, linVelocity);
        }


        gAppContext->modeContext->prevPoseState = poseState;
    }

    return poseState;
}

//-----------------------------------------------------------------------------
sxrHeadPoseState sxrGetHistoricHeadPose(int64_t timestampNs)
//-----------------------------------------------------------------------------
{
    // Must adjust for gQTimeToAndroidBoot
    timestampNs += gQTimeToAndroidBoot;

    sxrHeadPoseState poseState;

    glm::fquat poseRot;     // Identity
    glm::vec3  posePos;     // Identity

    // Start with the pose status of what they expect, then correct later based on quality
    poseState.poseStatus = 0;
    if ((gAppContext->currentTrackingMode & kTrackingRotation) != 0)
        poseState.poseStatus |= kTrackingRotation;
    if ((gAppContext->currentTrackingMode & kTrackingPosition) != 0)
        poseState.poseStatus |= kTrackingPosition;

    poseState.pose.rotation.x = poseRot.x;
    poseState.pose.rotation.y = poseRot.y;
    poseState.pose.rotation.z = poseRot.z;
    poseState.pose.rotation.w = poseRot.w;

    poseState.pose.position.x = posePos.x;
    poseState.pose.position.y = posePos.y;
    poseState.pose.position.z = posePos.z;

    if (gAppContext == NULL)
    {
        LOGE("sxrGetHistoricHeadPose Failed: SnapdragonVR not initialized!");
        poseState.poseStatus = 0;
        return poseState;
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false)
    {
        LOGE("sxrGetHistoricHeadPose Failed: Called when not in VR mode!");
        poseState.poseStatus = 0;
        return poseState;
    }

    if (gAppContext->modeContext == NULL)
    {
        LOGE("sxrGetHistoricHeadPose Failed: Called when not in VR mode!");
        poseState.poseStatus = 0;
        return poseState;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrGetHistoricHeadPose() not supported on this device!");
        poseState.poseStatus = 0;
        return poseState;
    }

    if (SXR_ERROR_NONE != GetTrackingFromHistoricSensor(timestampNs, poseState))
    {
        LOGE("sxrGetHistoricHeadPose Failed: Unknown!");
        poseState.poseStatus = 0;
        return poseState;
    }

    // Assign to return value after adjusting for recenter
    L_SetPoseState(poseState, 0.0f);

    return poseState;
}

//-----------------------------------------------------------------------------
SxrResult sxrGetEyePose(sxrEyePoseState *pReturnPose, int32_t eyePoseFlags)
//-----------------------------------------------------------------------------
{
    // Clear out return in case we leave early
    memset(pReturnPose, 0, sizeof(sxrEyePoseState));

    if (pReturnPose == NULL)
    {
        LOGE("svrGetEyePose Failed: Called with NULL parameter");
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext == NULL)
    {
        LOGE("svrGetEyePose Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false)
    {
        LOGE("svrGetEyePose Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    if (gAppContext->modeContext == NULL)
    {
        LOGE("svrGetEyePose Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("svrGetEyePose() not supported on this device!");
        return SXR_ERROR_UNSUPPORTED;
    }

    if ((gAppContext->currentTrackingMode & kTrackingEye) == 0)
    {
        LOGE("svrGetEyePose Failed: Eye tracking has not been enabled!");
        return SXR_ERROR_UNSUPPORTED;
    }


    // Finally, we can get the actual eye pose
    int32_t qvrReturn = 0;
    int64_t timestampNs = 0;
    qvrservice_eye_tracking_data_t* qvr_eye_pose;
    //qvrReturn = QVRServiceClient_GetEyeTrackingData(gAppContext->qvrHelper, &qvr_eye_pose, timestampNs);
    qvr_eye_tracking_data_flags_t flags = (eyePoseFlags & kEyeTrackingDataSynced) != 0 ? QVR_EYE_TRACKING_DATA_ENABLE_SYNC : 0;
    if (flags != 0 && m_eyeSyncCtrl != NULL)
    {
        qvrReturn = QVRServiceClient_GetEyeTrackingDataWithFlags(gAppContext->qvrHelper, &qvr_eye_pose, timestampNs, flags);
    }
    else
    {
        qvrReturn = QVRServiceClient_GetEyeTrackingData(gAppContext->qvrHelper, &qvr_eye_pose, timestampNs);
    }
    if (qvrReturn != QVR_SUCCESS)
    {
        switch (qvrReturn)
        {
        case QVR_CALLBACK_NOT_SUPPORTED:
            LOGE("Error from QVRServiceClient_GetEyeTrackingData: QVR_CALLBACK_NOT_SUPPORTED");
            return SXR_ERROR_UNSUPPORTED;

        case QVR_API_NOT_SUPPORTED:
            LOGE("Error from QVRServiceClient_GetEyeTrackingData: QVR_API_NOT_SUPPORTED");
            return SXR_ERROR_UNSUPPORTED;

        case QVR_INVALID_PARAM:
            LOGE("Error from QVRServiceClient_GetEyeTrackingData: QVR_INVALID_PARAM");
            return SXR_ERROR_UNKNOWN;

        default:
            LOGE("Error from QVRServiceClient_GetEyeTrackingData: Unknown = %d", qvrReturn);
            return SXR_ERROR_UNKNOWN;
        }
    }

    memset(pReturnPose, 0, sizeof(sxrEyePoseState));

    pReturnPose->timestamp = qvr_eye_pose->timestamp;

    // Set the status values
    pReturnPose->leftEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_LEFT].flags & QVR_GAZE_PER_EYE_GAZE_ORIGIN_VALID) ? kGazePointValid : 0;
    pReturnPose->leftEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_LEFT].flags & QVR_GAZE_PER_EYE_GAZE_DIRECTION_VALID) ? kGazeVectorValid : 0;
    pReturnPose->leftEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_LEFT].flags & QVR_GAZE_PER_EYE_EYE_OPENNESS_VALID) ? kEyeOpennessValid : 0;
    pReturnPose->leftEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_LEFT].flags & QVR_GAZE_PER_EYE_PUPIL_DILATION_VALID) ? kEyePupilDilationValid : 0;
    pReturnPose->leftEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_LEFT].flags & QVR_GAZE_PER_EYE_POSITION_GUIDE_VALID) ? kEyePositionGuideValid : 0;
    pReturnPose->leftEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_LEFT].flags & QVR_GAZE_PER_EYE_BLINK_VALID) ? kEyeBlinkValid : 0;

    pReturnPose->rightEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_RIGHT].flags & QVR_GAZE_PER_EYE_GAZE_ORIGIN_VALID) ? kGazePointValid : 0;
    pReturnPose->rightEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_RIGHT].flags & QVR_GAZE_PER_EYE_GAZE_DIRECTION_VALID) ? kGazeVectorValid : 0;
    pReturnPose->rightEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_RIGHT].flags & QVR_GAZE_PER_EYE_EYE_OPENNESS_VALID) ? kEyeOpennessValid : 0;
    pReturnPose->rightEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_RIGHT].flags & QVR_GAZE_PER_EYE_PUPIL_DILATION_VALID) ? kEyePupilDilationValid : 0;
    pReturnPose->rightEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_RIGHT].flags & QVR_GAZE_PER_EYE_POSITION_GUIDE_VALID) ? kEyePositionGuideValid : 0;
    pReturnPose->rightEyePoseStatus |= (qvr_eye_pose->eye[QVR_EYE_RIGHT].flags & QVR_GAZE_PER_EYE_BLINK_VALID) ? kEyeBlinkValid : 0;

    //LOGE("CombinedEyePoseStatus = %d", (int)qvr_eye_pose->flags);
    pReturnPose->combinedEyePoseStatus |= kGazePointValid;// (qvr_eye_pose->flags & QVR_GAZE_ORIGIN_COMBINED_VALID) ? kGazePointValid : 0;
    pReturnPose->combinedEyePoseStatus |= kGazeVectorValid;// (qvr_eye_pose->flags & QVR_GAZE_DIRECTION_COMBINED_VALID ) ? kGazeVectorValid : 0;

    // Set base point for left, right, and combined (meters)
    pReturnPose->leftEyeGazePoint[0] = qvr_eye_pose->eye[QVR_EYE_LEFT].gazeOrigin[0] * 0.001f;
    pReturnPose->leftEyeGazePoint[1] = qvr_eye_pose->eye[QVR_EYE_LEFT].gazeOrigin[1] * 0.001f;
    pReturnPose->leftEyeGazePoint[2] = qvr_eye_pose->eye[QVR_EYE_LEFT].gazeOrigin[2] * 0.001f;

    pReturnPose->rightEyeGazePoint[0] = qvr_eye_pose->eye[QVR_EYE_RIGHT].gazeOrigin[0] * 0.001f;
    pReturnPose->rightEyeGazePoint[1] = qvr_eye_pose->eye[QVR_EYE_RIGHT].gazeOrigin[1] * 0.001f;
    pReturnPose->rightEyeGazePoint[2] = qvr_eye_pose->eye[QVR_EYE_RIGHT].gazeOrigin[2] * 0.001f;

    pReturnPose->combinedEyeGazePoint[0] = qvr_eye_pose->gazeOriginCombined[0] * 0.001f;
    pReturnPose->combinedEyeGazePoint[1] = qvr_eye_pose->gazeOriginCombined[1] * 0.001f;
    pReturnPose->combinedEyeGazePoint[2] = qvr_eye_pose->gazeOriginCombined[2] * 0.001f;

    // Set direction vector for left, right, and combined
    pReturnPose->leftEyeGazeVector[0] = qvr_eye_pose->eye[QVR_EYE_LEFT].gazeDirection[0];
    pReturnPose->leftEyeGazeVector[1] = qvr_eye_pose->eye[QVR_EYE_LEFT].gazeDirection[1];
    pReturnPose->leftEyeGazeVector[2] = qvr_eye_pose->eye[QVR_EYE_LEFT].gazeDirection[2];

    pReturnPose->rightEyeGazeVector[0] = qvr_eye_pose->eye[QVR_EYE_RIGHT].gazeDirection[0];
    pReturnPose->rightEyeGazeVector[1] = qvr_eye_pose->eye[QVR_EYE_RIGHT].gazeDirection[1];
    pReturnPose->rightEyeGazeVector[2] = qvr_eye_pose->eye[QVR_EYE_RIGHT].gazeDirection[2];

    pReturnPose->combinedEyeGazeVector[0] = qvr_eye_pose->gazeDirectionCombined[0];
    pReturnPose->combinedEyeGazeVector[1] = qvr_eye_pose->gazeDirectionCombined[1];
    pReturnPose->combinedEyeGazeVector[2] = qvr_eye_pose->gazeDirectionCombined[2];

    // Set the left, right position guides
    pReturnPose->leftEyePositionGuide[0] = qvr_eye_pose->eye[QVR_EYE_LEFT].positionGuide[0];
    pReturnPose->leftEyePositionGuide[1] = qvr_eye_pose->eye[QVR_EYE_LEFT].positionGuide[1];
    pReturnPose->leftEyePositionGuide[2] = qvr_eye_pose->eye[QVR_EYE_LEFT].positionGuide[2];

    pReturnPose->rightEyePositionGuide[0] = qvr_eye_pose->eye[QVR_EYE_RIGHT].positionGuide[0];
    pReturnPose->rightEyePositionGuide[1] = qvr_eye_pose->eye[QVR_EYE_RIGHT].positionGuide[1];
    pReturnPose->rightEyePositionGuide[2] = qvr_eye_pose->eye[QVR_EYE_RIGHT].positionGuide[2];

    // Set the openness
    pReturnPose->leftEyeOpenness = qvr_eye_pose->eye[QVR_EYE_LEFT].eyeOpenness;
    pReturnPose->rightEyeOpenness = qvr_eye_pose->eye[QVR_EYE_RIGHT].eyeOpenness;

    // Set the dilation
    pReturnPose->leftEyePupilDilation = qvr_eye_pose->eye[QVR_EYE_LEFT].pupilDilation;
    pReturnPose->rightEyePupilDilation = qvr_eye_pose->eye[QVR_EYE_RIGHT].pupilDilation;

    // Set the status value for foveation
    pReturnPose->foveatedEyeGazeState = qvr_eye_pose->foveatedGazeTrackingState;

    // Set the direction vector for foveation
    pReturnPose->foveatedEyeGazeVector[0] = qvr_eye_pose->foveatedGazeDirection[0];
    pReturnPose->foveatedEyeGazeVector[1] = qvr_eye_pose->foveatedGazeDirection[1];
    pReturnPose->foveatedEyeGazeVector[2] = qvr_eye_pose->foveatedGazeDirection[2];

    // Set the blink status (true = eye is closed, false = eye is open)
    pReturnPose->leftEyeBlink = qvr_eye_pose->eye[QVR_EYE_LEFT].blink;
    pReturnPose->rightEyeBlink = qvr_eye_pose->eye[QVR_EYE_RIGHT].blink;

    // Transform from Tobii RH z-forward to SVR RH z-back by negating x-axis and z-axis (rotation 180 y-axis)
    pReturnPose->leftEyeGazePoint[0] = -pReturnPose->leftEyeGazePoint[0];
    pReturnPose->leftEyeGazePoint[2] = -pReturnPose->leftEyeGazePoint[2];

    pReturnPose->rightEyeGazePoint[0] = -pReturnPose->rightEyeGazePoint[0];
    pReturnPose->rightEyeGazePoint[2] = -pReturnPose->rightEyeGazePoint[2];

    pReturnPose->combinedEyeGazePoint[0] = -pReturnPose->combinedEyeGazePoint[0];
    pReturnPose->combinedEyeGazePoint[2] = -pReturnPose->combinedEyeGazePoint[2];

    pReturnPose->leftEyeGazeVector[0] = -pReturnPose->leftEyeGazeVector[0];
    pReturnPose->leftEyeGazeVector[2] = -pReturnPose->leftEyeGazeVector[2];

    pReturnPose->rightEyeGazeVector[0] = -pReturnPose->rightEyeGazeVector[0];
    pReturnPose->rightEyeGazeVector[2] = -pReturnPose->rightEyeGazeVector[2];

    pReturnPose->combinedEyeGazeVector[0] = -pReturnPose->combinedEyeGazeVector[0];
    pReturnPose->combinedEyeGazeVector[2] = -pReturnPose->combinedEyeGazeVector[2];

    pReturnPose->foveatedEyeGazeVector[0] = -pReturnPose->foveatedEyeGazeVector[0];
    pReturnPose->foveatedEyeGazeVector[2] = -pReturnPose->foveatedEyeGazeVector[2];

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrGetTrackingCapabilities(uint64_t *pCapabilities)
//-----------------------------------------------------------------------------
{
    // Clear out return in case we leave early
    memset(pCapabilities, 0, sizeof(uint64_t));

    if (pCapabilities == NULL)
    {
        LOGE("sxrGetTrackingCapabilities Failed: Called with NULL parameter");
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext == NULL)
    {
        LOGE("sxrGetTrackingCapabilities Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    //if (gAppContext == NULL || gAppContext->inVrMode == false)
    //{
    //    LOGE("sxrGetTrackingCapabilities Failed: Called when not in VR mode!");
    //    return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    //}

    //if (gAppContext->modeContext == NULL)
    //{
    //    LOGE("sxrGetTrackingCapabilities Failed: Called when not in VR mode!");
    //    return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    //}

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrGetTrackingCapabilities() not supported on this device!");
        return SXR_ERROR_UNSUPPORTED;
    }

    // Eye Tracking Capabilities
    qvr_capabilities_flags_t qvr_eye_caps = 0;
    int qRes = QVRServiceClient_GetEyeTrackingCapabilities(gAppContext->qvrHelper, &qvr_eye_caps);
    if (qRes == QVR_SUCCESS)
    {
        *pCapabilities = (uint64_t)qvr_eye_caps;

        LOGV("Eye Tracking Capabilities:");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_COMBINED_GAZE) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_COMBINED_GAZE");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_CONVERGENCE_DISTANCE) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_CONVERGENCE_DISTANCE");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_FOVEATED_GAZE) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_FOVEATED_GAZE");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_GAZE_ORIGIN) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_GAZE_ORIGIN");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_GAZE_DIRECTION) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_GAZE_DIRECTION");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_2D_GAZE_POINT) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_2D_GAZE_POINT");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_EYE_OPENNESS) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_EYE_OPENNESS");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_PUPIL_DILATION) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_PUPIL_DILATION");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_POSITION_GUIDE) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_POSITION_GUIDE");
        if ((qvr_eye_caps & QVR_CAPABILITY_GAZE_PER_EYE_BLINK) != 0)
            LOGV("\tQVR_CAPABILITY_GAZE_PER_EYE_BLINK");
    }
    else
    {
        L_LogQvrError("QVRServiceClient_GetEyeTrackingCapabilities", qRes);
        return SXR_ERROR_UNSUPPORTED;
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrGetPointCloudData(uint32_t *pNumPoints, sxrCloudPoint **pCloudData)
//-----------------------------------------------------------------------------
{
    // Clear the return values
    *pNumPoints = 0;
    *pCloudData = NULL;

    if (gAppContext == NULL || gAppContext->inVrMode == false)
    {
        LOGE("svrGetPointCloudData Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_STARTED;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("svrGetPointCloudData() not supported on this device!");
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    assert(sizeof(sxrCloudPoint) == sizeof(XrMapPointQTI));

    if (m_SlamPointCloud)
    {
        LOGW("sxrGetPointCloudData previous point cloud is not released - calling sxrReleasePointCloudData");
        sxrReleasePointCloudData();
    }

    int32_t qvrReturn = QVRServiceClient_GetPointCloud(gAppContext->qvrHelper, &m_SlamPointCloud);
    if (qvrReturn != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_GetPointCloud", qvrReturn);
        return SXR_ERROR_UNKNOWN;
    }

    LOGV("QVRServiceClient_GetPointCloud() returned %d points", m_SlamPointCloud->numPoints);

    // Set the return values
    *pNumPoints = (uint32_t)m_SlamPointCloud->numPoints;
    *pCloudData = (sxrCloudPoint*)m_SlamPointCloud->points;

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrReleasePointCloudData()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL || gAppContext->inVrMode == false)
    {
        LOGE("sxrReleasePointCloudData Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_STARTED;
    }

    if (gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrReleasePointCloudData() not supported on this device!");
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    //if (!m_SlamPointCloud)
    //{
    //    return SXR_ERROR_INVALID_OPERATION;
    //}

    int32_t qvrReturn = QVRServiceClient_ReleasePointCloud(gAppContext->qvrHelper, m_SlamPointCloud);
    if (qvrReturn != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_ReleasePointCloud", qvrReturn);
        return SXR_ERROR_UNKNOWN;
    }

    m_SlamPointCloud = nullptr;

    return SXR_ERROR_NONE;
}

SxrResult sxrGetRecenterPose(sxrVector3 *pPosition, sxrQuaternion *pRotation)
{
    if (pPosition == NULL || pRotation == NULL)
    {
        LOGE("sxrGetRecenterPose Failed: Called with NULL parameter");
        return SXR_ERROR_UNKNOWN;
    }

    if (gAppContext == NULL || gAppContext->inVrMode == false || gAppContext->modeContext == NULL)
    {
        LOGE("sxrGetRecenterPose Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    //pPosition = glm::value_ptr(gAppContext->modeContext->recenterPos);
    pPosition->x = gAppContext->modeContext->recenterPos.x;
    pPosition->y = gAppContext->modeContext->recenterPos.y;
    pPosition->z = gAppContext->modeContext->recenterPos.z;

    //pRotation = glm::value_ptr(gAppContext->modeContext->recenterRot);
    pRotation->x = gAppContext->modeContext->recenterRot.x;
    pRotation->y = gAppContext->modeContext->recenterRot.y;
    pRotation->z = gAppContext->modeContext->recenterRot.z;
    pRotation->w = gAppContext->modeContext->recenterRot.w;

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrRecenterPose()
//-----------------------------------------------------------------------------
{
    SxrResult svrResult;

    if (gAppContext == NULL || gAppContext->modeContext == NULL)
    {
        LOGE("sxrRecenterPose Failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

#ifdef ENABLE_REMOTE_XR_RENDERING
    if (gEnableRVR) {
        std::lock_guard<std::mutex> lck (gRVRManagerInitMtx);
        if(!gRVRManagerInit){
            LOGE("[RVR] sxrRecenterPose Failed: RVRManager init not complete! gRVRManagerInit %d",gRVRManagerInit);
            return SXR_ERROR_VRMODE_NOT_INITIALIZED;
        }
    }
#endif

    if (gAppContext->currentTrackingMode & kTrackingPosition)
    {
        svrResult = sxrRecenterPosition();
        if (svrResult != SXR_ERROR_NONE)
            return svrResult;
    }

    svrResult = sxrRecenterOrientation();
    if (svrResult != SXR_ERROR_NONE)
        return svrResult;

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrRecenterPosition()
//-----------------------------------------------------------------------------
{
    if (gDisableTrackingRecenter) {
        /* skip recenter */
        return SXR_ERROR_NONE;
    }
//#ifdef ENABLE_REMOTE_XR_RENDERING
//    if (gEnableRVR) {
//        /* skip recenter */
//        return SXR_ERROR_NONE;
//    }
//#endif
    // We want to disable reprojection for a few frames after recenter has been called
    gRecenterTransition = 0;

    float predictedTimeMs = 0.0f;

    if (gAppContext == NULL || gAppContext->inVrMode == false || gAppContext->modeContext == NULL)
    {
        LOGE("sxrRecenterPosition Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_STARTED;
    }

    LOGI("Recentering Position");

    sxrHeadPoseState poseState;
    glm::fquat poseRot;     // Identity
    glm::vec3  posePos;     // Identity

    // Set the pose status and let it be changed by quality settings
    poseState.pose.rotation.x = poseRot.x;
    poseState.pose.rotation.y = poseRot.y;
    poseState.pose.rotation.z = poseRot.z;
    poseState.pose.rotation.w = poseRot.w;
    poseState.pose.position.x = posePos.x;
    poseState.pose.position.y = posePos.y;
    poseState.pose.position.z = posePos.z;
    poseState.poseStatus = gAppContext->currentTrackingMode;

#ifdef ENABLE_REMOTE_XR_RENDERING
    if (gEnableRVR && !gRVRUseQVRServicePose && gRVRManager) {
        gRVRManager->GetHeadPose(poseState);
        predictedTimeMs = 0.0f;
    }
    else {
#endif
    uint64_t sampleTimeStamp;
    SxrResult resultCode = GetTrackingFromPredictiveSensor(predictedTimeMs, &sampleTimeStamp, poseState);
    if (SXR_ERROR_NONE != resultCode)
    {
        poseState.poseStatus = 0;
        return resultCode;
    }
#ifdef ENABLE_REMOTE_XR_RENDERING
    }
#endif

    if (poseState.poseStatus & kTrackingPosition)
    {
        gAppContext->modeContext->recenterPos.x = poseState.pose.position.x;
        gAppContext->modeContext->recenterPos.y = poseState.pose.position.y;
        gAppContext->modeContext->recenterPos.z = poseState.pose.position.z;
    }
    else
    {
        LOGE("sxrRecenterPosition Failed: Pose quality too low! (Pose Status = %d)", poseState.poseStatus);
        return SXR_ERROR_UNKNOWN;
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SxrResult sxrRecenterOrientation(bool yawOnly)
//-----------------------------------------------------------------------------
{
    if (gDisableTrackingRecenter) {
        /* skip recenter */
        return SXR_ERROR_NONE;
    }
//#ifdef ENABLE_REMOTE_XR_RENDERING
//    if (gEnableRVR) {
//        /* skip recenter */
//        return SXR_ERROR_NONE;
//    }
//#endif
    // We want to disable reprojection for a few frames after recenter has been called
    gRecenterTransition = 0;

    float predictedTimeMs = 0.0f;

    if (gAppContext == NULL || gAppContext->inVrMode == false || gAppContext->modeContext == NULL)
    {
        LOGE("sxrRecenterOrientation Failed: Called when not in VR mode!");
        return SXR_ERROR_VRMODE_NOT_STARTED;
    }

    //if (gAppContext->qvrHelper == NULL)
    //{
    //    LOGE("sxrRecenterOrientation Failed: SnapdragonVR not initialized!");
    //    return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    //}

    LOGI("Recentering Orientation");

    sxrHeadPoseState poseState;
    glm::fquat poseRot;     // Identity
    glm::vec3  posePos;     // Identity

    // Set the pose status and let it be changed by quality settings
    poseState.pose.rotation.x = poseRot.x;
    poseState.pose.rotation.y = poseRot.y;
    poseState.pose.rotation.z = poseRot.z;
    poseState.pose.rotation.w = poseRot.w;
    poseState.pose.position.x = posePos.x;
    poseState.pose.position.y = posePos.y;
    poseState.pose.position.z = posePos.z;
    poseState.poseStatus = gAppContext->currentTrackingMode;

#ifdef ENABLE_REMOTE_XR_RENDERING
    if (gEnableRVR && !gRVRUseQVRServicePose && gRVRManager) {
        gRVRManager->GetHeadPose(poseState);
        predictedTimeMs = 0.0f;
    }
    else {
#endif
    uint64_t sampleTimeStamp;
    SxrResult resultCode = GetTrackingFromPredictiveSensor(predictedTimeMs, &sampleTimeStamp, poseState);
    if (SXR_ERROR_NONE != resultCode)
    {
        poseState.poseStatus = 0;
        return resultCode;
    }
#ifdef ENABLE_REMOTE_XR_RENDERING
    }
#endif

    if (!(poseState.poseStatus & kTrackingRotation))
    {
        LOGE("sxrRecenterOrientation Failed: Pose quality too low! (Pose Status = %d)", poseState.poseStatus);
        return SXR_ERROR_UNKNOWN;
    }

    if (yawOnly)
    {
        // We really want to only adjust for Yaw
        glm::vec3 Forward = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::fquat CurrentQuat;
        CurrentQuat.x = poseState.pose.rotation.x;
        CurrentQuat.y = poseState.pose.rotation.y;
        CurrentQuat.z = poseState.pose.rotation.z;
        CurrentQuat.w = poseState.pose.rotation.w;

        glm::vec3 LookDir = Forward * CurrentQuat;

        float RotateRads = atan2(LookDir.x, LookDir.z);

        glm::fquat IdentityQuat;
        gAppContext->modeContext->recenterRot = glm::rotate(IdentityQuat, RotateRads, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else
    {
        // Recenter is whatever it takes to get back to center
        glm::fquat CurrentQuat;
        CurrentQuat.x = poseState.pose.rotation.x;
        CurrentQuat.y = poseState.pose.rotation.y;
        CurrentQuat.z = poseState.pose.rotation.z;
        CurrentQuat.w = poseState.pose.rotation.w;

        gAppContext->modeContext->recenterRot = inverse(CurrentQuat);
    }

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SXRP_EXPORT uint32_t sxrGetSupportedTrackingModes()
//-----------------------------------------------------------------------------
{
    uint32_t result = 0;
#ifdef ENABLE_REMOTE_XR_RENDERING
    if(gEnableRVR && !gRVRUseQVRServicePose) {
        if (gAppContext == NULL)
        {
            LOGE("sxrSetTrackingMode failed: SnapdragonVR not initialized!");
            return SXR_ERROR_VRMODE_NOT_INITIALIZED;
        }
        result = kTrackingRotation | kTrackingPosition;
        return result;
    }
#endif

    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrGetSupportedTrackingModes failed: SnapdragonVR not initialized!");
        return 0;
    }

    //Get the supported tracking modes
    uint32_t supportedTrackingModes;
    int qRes = QVRServiceClient_GetTrackingMode(gAppContext->qvrHelper, NULL, &supportedTrackingModes);
	if (qRes != QVR_SUCCESS)
	{
		L_LogQvrError("QVRServiceClient_GetTrackingMode", qRes);
	}
    if (supportedTrackingModes & TRACKING_MODE_ROTATIONAL || supportedTrackingModes & TRACKING_MODE_ROTATIONAL_MAG)
    {
        result |= kTrackingRotation;
    }

    if (supportedTrackingModes & TRACKING_MODE_POSITIONAL)
    {
        //Note at this time the QVR service will report that it is capable of positional tracking
        //due to the presence of a second camera even though that second camera may not be a 6DOF module
        result |= kTrackingPosition;
    }

    // Get supported eye tracking
    uint32_t currentEyeMode;
    uint32_t supportedEyeModes;
    qRes = QVRServiceClient_GetEyeTrackingMode(gAppContext->qvrHelper, &currentEyeMode, &supportedEyeModes);
    if (qRes != QVR_SUCCESS)
    {
        L_LogQvrError("QVRServiceClient_GetEyeTrackingMode", qRes);
    }
    else if (supportedEyeModes & QVRSERVICE_EYE_TRACKING_MODE_DUAL)
    {
        result |= kTrackingEye;
    }

    return result;
}

//-----------------------------------------------------------------------------
SXRP_EXPORT SxrResult sxrSetTrackingMode(uint32_t trackingModes)
//-----------------------------------------------------------------------------
{
#ifdef ENABLE_REMOTE_XR_RENDERING
    if (gAppContext == NULL)
    {
        LOGE("sxrSetTrackingMode failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    if(gEnableRVR && !gRVRUseQVRServicePose) {
        gAppContext->currentTrackingMode = trackingModes;
        return SXR_ERROR_NONE;
    }
#endif

    if (gAppContext == NULL || gAppContext->qvrHelper == NULL)
    {
        LOGE("sxrSetTrackingMode failed: SnapdragonVR not initialized!");
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    // Don't want to break here in case they are setting eye tracking mode later.
    // Which may or may not depend on the VR State.

	// QVRSERVICE_VRMODE_STATE qvrState = QVRServiceClient_GetVRMode(gAppContext->qvrHelper);
    // if (qvrState != VRMODE_STOPPED)
    // {
    //     LOGE("sxrSetTrackingMode failed: tracking mode can only be changed when VR mode is in a stopped state");
    //     return SXR_ERROR_VRMODE_NOT_STOPPED;
    // }

    // Validate what is passed in
    if ((trackingModes & kTrackingPosition) != 0)
    {
        // Because at this point having position without rotation is not supported
        trackingModes |= kTrackingRotation;
        trackingModes |= kTrackingPosition;
    }

    //Check for a forced tracking mode from the config file
    uint32_t actualTrackingMode = trackingModes;
    if (gForceTrackingMode != 0)
    {
        actualTrackingMode = 0;
        LOGI("sxrSetTrackingMode : Forcing to %d from config file", gForceTrackingMode);
        if (gForceTrackingMode & kTrackingRotation)
        {
            actualTrackingMode |= kTrackingRotation;
        }
        if (gForceTrackingMode & kTrackingPosition)
        {
            // Because at this point having position without rotation is not supported
            actualTrackingMode |= kTrackingRotation;
            actualTrackingMode |= kTrackingPosition;
        }
        if (gForceTrackingMode & kTrackingEye)
        {
            actualTrackingMode |= kTrackingEye;
        }
    }
    else
    {
        LOGI("sxrSetTrackingMode : Using application defined tracking mode (%d).", trackingModes);
    }

    //Make sure the device actually supports the tracking mode
    uint32_t supportedModes = sxrGetSupportedTrackingModes();

    //QVR Service accepts only a direct positional or rotational value, not a bitmask
    QVRSERVICE_TRACKING_MODE qvrTrackingMode = TRACKING_MODE_NONE;
    if ((actualTrackingMode & kTrackingPosition) != 0)
    {
        if ((supportedModes & kTrackingPosition) != 0)
        {
            LOGI("sxrSetTrackingMode : Setting tracking mode to positional");
            qvrTrackingMode = TRACKING_MODE_POSITIONAL;
        }
        else
        {
            LOGI("sxrSetTrackingMode: Requested positional tracking but device doesn't support, falling back to orientation only.");

            if (gUseMagneticRotationFlag)
                qvrTrackingMode = TRACKING_MODE_ROTATIONAL_MAG;
            else
                qvrTrackingMode = TRACKING_MODE_ROTATIONAL;

            actualTrackingMode &= ~kTrackingPosition;
        }

    }
    else if ((actualTrackingMode & kTrackingRotation) != 0)
    {
        LOGI("sxrSetTrackingMode : Setting tracking mode to rotational");

        if (gUseMagneticRotationFlag)
            qvrTrackingMode = TRACKING_MODE_ROTATIONAL_MAG;
        else
            qvrTrackingMode = TRACKING_MODE_ROTATIONAL;

        actualTrackingMode &= ~kTrackingPosition;
    }

    // Set normal tracking...
	int qRes = QVRServiceClient_SetTrackingMode(gAppContext->qvrHelper, qvrTrackingMode);
	if (qRes != QVR_SUCCESS)
	{
		L_LogQvrError("QVRServiceClient_SetTrackingMode", qRes);
	}

    // ...and eye tracking
    if ((actualTrackingMode & kTrackingEye) != 0)
    {
        LOGI("sxrSetTrackingMode : Eye tracking mode requested");

        // Get the current mode and supported modes
        uint32_t currentEyeMode;
        uint32_t supportedEyeModes;
        qRes = QVRServiceClient_GetEyeTrackingMode(gAppContext->qvrHelper, &currentEyeMode, &supportedEyeModes);
        if (qRes != QVR_SUCCESS)
        {
            L_LogQvrError("QVRServiceClient_GetEyeTrackingMode", qRes);

            // Eye tracking not supported
            LOGI("sxrSetTrackingMode: Eye tracking mode not supported");
            actualTrackingMode &= ~kTrackingEye;
        }
        else if ((supportedEyeModes & QVRSERVICE_EYE_TRACKING_MODE_DUAL) == 0)
        {
            LOGE("Error from QVRServiceClient_GetEyeTrackingMode: QVRSERVICE_EYE_TRACKING_MODE_DUAL is not supported!");
            LOGI("sxrSetTrackingMode: Eye tracking mode not supported");
            actualTrackingMode &= ~kTrackingEye;
        }

        // If the current mode is not what we want then set it now
        if ((actualTrackingMode & kTrackingEye) != 0 && (currentEyeMode & QVRSERVICE_EYE_TRACKING_MODE_DUAL) == 0)
        {
            LOGI("sxrSetTrackingMode : Enabling eye tracking mode");
            currentEyeMode = QVRSERVICE_EYE_TRACKING_MODE_DUAL;
            qRes = QVRServiceClient_SetEyeTrackingMode(gAppContext->qvrHelper, currentEyeMode);
            if (qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_SetEyeTrackingMode (Dual)", qRes);

                // Eye tracking not supported
                LOGI("sxrSetTrackingMode: Eye tracking mode not supported");
                actualTrackingMode &= ~kTrackingEye;
            }
        }
    } // Trying to enable eye tracking
    else
    {
        if ((supportedModes & kTrackingEye) != 0)
        {
            // Eye tracking is NOT enabled, make sure it is off.
            qRes = QVRServiceClient_SetEyeTrackingMode(gAppContext->qvrHelper, QVRSERVICE_EYE_TRACKING_MODE_NONE);
            if (qRes != QVR_SUCCESS)
            {
                L_LogQvrError("QVRServiceClient_SetEyeTrackingMode (Off)", qRes);
            }
        }

        // Always pull out this bit if eye tracking turned off
        actualTrackingMode &= ~kTrackingEye;
    }

    gAppContext->currentTrackingMode = actualTrackingMode;

    return SXR_ERROR_NONE;
}

//-----------------------------------------------------------------------------
SXRP_EXPORT uint32_t sxrGetTrackingMode()
//-----------------------------------------------------------------------------
{
    if (gAppContext == NULL)
    {
        LOGE("svrGetTrackingMode failed: SnapdragonVR not initialized!");
        return 0;
    }

    return gAppContext->currentTrackingMode;
}

SXRP_EXPORT void sxrSetNotificationFlag()
//-----------------------------------------------------------------------------
{
    gAppContext->keepNotification = true;
    LOGI("gAppContext->keepNotification = %d", gAppContext->keepNotification);
}

//-----------------------------------------------------------------------------
SXRP_EXPORT bool sxrIsRemoteRenderEnabled()
//-----------------------------------------------------------------------------
{
    return gEnableRVR == true;
}

//-----------------------------------------------------------------------------
SXRP_EXPORT bool sxrIs3drEnabled()
//-----------------------------------------------------------------------------
{
    return gEnable3DR == true;
}

//-----------------------------------------------------------------------------
SXRP_EXPORT bool sxrIsAnchorsEnabled()
//-----------------------------------------------------------------------------
{
    return gEnableAnchors == true;
}

//-----------------------------------------------------------------------------
SXRP_EXPORT bool sxrPollEvent(sxrEvent *pEvent)
//-----------------------------------------------------------------------------
{
    if (gAppContext != NULL &&
        gAppContext->modeContext != NULL &&
        gAppContext->modeContext->eventManager != NULL)
    {
        return gAppContext->modeContext->eventManager->PollEvent(*pEvent);
    }

    return false;
}
