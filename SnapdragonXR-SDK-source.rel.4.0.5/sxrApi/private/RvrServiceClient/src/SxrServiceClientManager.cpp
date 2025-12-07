//=============================================================================
// FILE: SxrServiceClientManager.cpp
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <dlfcn.h>

#include "svrConfig.h"
#include "svrUtil.h"
#include "SxrServiceClientManager.h"

EXTERN_VAR(bool, gEnableRVR);
EXTERN_VAR(bool, gRVRPayloadInRTP);
EXTERN_VAR(int, gRVRStreamingWidth);
EXTERN_VAR(int, gRVRStreamingHeight);
EXTERN_VAR(float, gRVRFoveaAngle);
EXTERN_VAR(int, gRVRAvgBitrateMbPS);
EXTERN_VAR(int, gRVRPeakBitrateMbPS);
EXTERN_VAR(int, gRVRGOPLength);
EXTERN_VAR(int, gRVRSlicesPerFrame);
EXTERN_VAR(float, gRVRFPS);
EXTERN_VAR(char*, gRVRCodec);
EXTERN_VAR(int, gRVRMTU);
EXTERN_VAR(int, gRVRUsePoseSocket);
EXTERN_VAR(int, gRVRMode);
EXTERN_VAR(int, gRVRPerfMode);
EXTERN_VAR(int, gRVRYCoCg);
EXTERN_VAR(int, gRVRDisplayFlags);
EXTERN_VAR(float, gRVRF0);
EXTERN_VAR(float, gRVRF1);
EXTERN_VAR(int, gRVRSynchronizeToHMDVSync);
EXTERN_VAR(bool, gRVRAudio);
EXTERN_VAR(float, gRVRIPD);
EXTERN_VAR(bool, gRVRCalculateDepthDFS);
EXTERN_VAR(int, gRVRKernelSize);
EXTERN_VAR(int, gRVRMaxDisparity);
EXTERN_VAR(int, gRVRLocalSizeX);
EXTERN_VAR(int, gRVRLocalSizeY);
EXTERN_VAR(int, gRVRDownScaleFactor);
EXTERN_VAR(float, gRVRPositionScaleZ);
EXTERN_VAR(float, gRVRFarClipPlane);
EXTERN_VAR(int, gRVRDefaultAtwMode);
EXTERN_VAR(bool, gRVREnableHeartbeat);
EXTERN_VAR(bool, gRVRWaitForHMD);
EXTERN_VAR(int, gRVRFlags);
EXTERN_VAR(float, gRVRRenderFov);
EXTERN_VAR(float, gRVRRenderFovX);
EXTERN_VAR(float, gRVRRenderFovY);

EXTERN_VAR(int, gEyeBufferWidth);
EXTERN_VAR(int, gEyeBufferHeight);
EXTERN_VAR(float, gEyeBufferFovX);
EXTERN_VAR(float, gLeftFrustum_Near);
EXTERN_VAR(float, gLeftFrustum_Far);
EXTERN_VAR(float, gLeftFrustum_Left);
EXTERN_VAR(float, gLeftFrustum_Right);
EXTERN_VAR(float, gLeftFrustum_Top);
EXTERN_VAR(float, gLeftFrustum_Bottom);
EXTERN_VAR(float, gRightFrustum_Near);
EXTERN_VAR(float, gRightFrustum_Far);
EXTERN_VAR(float, gRightFrustum_Left);
EXTERN_VAR(float, gRightFrustum_Right);
EXTERN_VAR(float, gRightFrustum_Top);
EXTERN_VAR(float, gRightFrustum_Bottom);
EXTERN_VAR(int, gWarpMeshType);

SxrServiceClientManager::SxrServiceClientManager()
{
}

SxrServiceClientManager::~SxrServiceClientManager()
{
}

SxrResult SxrServiceClientManager::Init()
{
    ISXRRVRServiceClient *pRVRClient = nullptr;
    void* handle = dlopen("libsxrservice_client.qti.so", RTLD_LAZY);
    if (!handle) {
        LOGE("[SxrServiceClientManager] error opening (libsxrservice_client.qti.so) : %s\n", dlerror());
    } else {
        FnSXRCreateService mPFnSXRCreateService = nullptr;
        mPFnSXRCreateService = (FnSXRCreateService)dlsym(handle, "SXRCreateService");
        if(!mPFnSXRCreateService) {
            LOGE("[SxrServiceClientManager] error loading function : %s\n", dlerror());
        } else {
            SXRRVRConfig param;
            param.width  = gEyeBufferWidth;
            param.height = gEyeBufferHeight;
            param.avg_bitrate = gRVRAvgBitrateMbPS < 0 ? 0 : gRVRAvgBitrateMbPS;
            param.peak_bitrate = gRVRPeakBitrateMbPS < gRVRAvgBitrateMbPS ? gRVRAvgBitrateMbPS : gRVRPeakBitrateMbPS;
            param.gop_length = gRVRGOPLength;
            param.slices_per_frame = gRVRSlicesPerFrame;
            param.fps = gRVRFPS;
            param.mtu = gRVRMTU;
            param.use_pose_socket = gRVRUsePoseSocket;
            param.remote_mode = gRVRMode;
            param.perf_mask = gRVRPerfMode;
            param.ycocg = gRVRYCoCg;
            param.display_flags = (uint8_t)gRVRDisplayFlags;//SXRDisplayFlags
            param.sync_with_remote = gRVRSynchronizeToHMDVSync;
            param.perf_factor0 = gRVRF0;
            param.perf_factor1 = gRVRF1;
            param.cam_near = gLeftFrustum_Near;
            param.cam_far = gLeftFrustum_Far;
            param.audio_enabled = gRVRAudio ? 1 : 0;
            param.warpmesh_type = (uint8_t)gWarpMeshType;
            param.ipd = gRVRIPD;
            param.fov = gRVRRenderFov;
            param.calculateDepthDFS = gRVRCalculateDepthDFS;
            param.kernelSize = gRVRKernelSize;
            param.maxDisparity = gRVRMaxDisparity;
            param.localSizeX = gRVRLocalSizeX;
            param.localSizeY = gRVRLocalSizeY;
            param.downScaleFactor = gRVRDownScaleFactor;
            param.positionScaleZ = gRVRPositionScaleZ;
            param.farClipPlane = (gRVRFarClipPlane > 0.0f) ? gRVRFarClipPlane: gLeftFrustum_Far;
            param.defaultAtwMode = (SXRATWMode)gRVRDefaultAtwMode;
            param.enableHeartbeat = gRVREnableHeartbeat;
            param.waitForHMD = gRVRWaitForHMD;
            param.payloadInRTP = gRVRPayloadInRTP;
            param.streamingWidth = gRVRStreamingWidth;
            param.streamingHeight = gRVRStreamingHeight;
            param.foveaAngle = gRVRFoveaAngle;
            param.flags = (uint8_t)gRVRFlags; //SXRVideoFlags
            strncpy(param.codec, gRVRCodec, sizeof(param.codec));
            LOGI("[SxrServiceClientManager] configuration (%dx%d) @%0.2f fps, codec = %s birate (%d, %d), gop (%d) slices (%d) f0(%0.2f) f1(%0.2f) dfs = %d flags %u",
                param.width, param.height, param.fps, param.codec,
                param.avg_bitrate, param.peak_bitrate,
                param.gop_length, param.slices_per_frame,
                param.perf_factor0, param.perf_factor1,
                param.calculateDepthDFS, param.flags);
            mPFnSXRCreateService(SXR_SERVICE_RVR_SERVER, (void*)&param, &mServiceClient);
            if(mServiceClient) {
                mServiceClient->GetInterface((void**)&pRVRClient);
            }
            if(pRVRClient->Init(param) < 0) {
                return SXR_ERROR_UNSUPPORTED;
            }
        }
    }
    mRVRClient = pRVRClient;

    return SXR_ERROR_NONE;
}

void SxrServiceClientManager::Release()
{
    if(mServiceClient) {
        mServiceClient->Release();
        mServiceClient = nullptr;
        mRVRClient = nullptr;
    }
}

SxrResult SxrServiceClientManager::SubmitFrame(const sxrFrameParams* pFrameParams, glm::fquat* recenterRot, glm::vec3* recenterPos)
{
    if(!mRVRClient) {
        return SXR_ERROR_UNSUPPORTED;
    }

    SXRFrameParams rvrparams;
    memset(&rvrparams, 0, sizeof(rvrparams));

    for(int i = 0; i < 2; i ++) {
        unsigned int texture = pFrameParams->renderLayers[i].imageHandle;
        if(texture == 0 || pFrameParams->renderLayers[i].eyeMask == 0) {
            continue;
        }
        rvrparams.renderLayers[i].imageHandle = pFrameParams->renderLayers[i].imageHandle;
        rvrparams.renderLayers[i].eyeMask = (SXREyeMask)pFrameParams->renderLayers[i].eyeMask;
        rvrparams.renderLayers[i].layerFlags = pFrameParams->renderLayers[i].layerFlags;
        rvrparams.headPoseState.poseTimeStampNs = pFrameParams->headPoseState.poseTimeStampNs;

        LOGV("[SxrServiceClientManager] Submit (eye = %s) (texture = %d) (ts = %" PRId64 "us)",
            (rvrparams.renderLayers[i].eyeMask & kSXREyeMaskLeft) ? "left" : "right",
            rvrparams.renderLayers[i].imageHandle,
            rvrparams.headPoseState.poseTimeStampNs/1000);
    }

    if(recenterRot && recenterPos) {
        // Apply inverse recenter operation
        glm::fquat poseRot;
        glm::vec3 posePos;

        poseRot.x = pFrameParams->headPoseState.pose.rotation.x;
        poseRot.y = pFrameParams->headPoseState.pose.rotation.y;
        poseRot.z = pFrameParams->headPoseState.pose.rotation.z;
        poseRot.w = pFrameParams->headPoseState.pose.rotation.w;

        posePos.x = pFrameParams->headPoseState.pose.position.x;
        posePos.y = pFrameParams->headPoseState.pose.position.y;
        posePos.z = pFrameParams->headPoseState.pose.position.z;

        posePos = posePos * inverse(*recenterRot);
        posePos = posePos + *recenterPos;
        poseRot = poseRot * inverse(*recenterRot);
    
        rvrparams.headPoseState.pose.position.x = posePos.x;
        rvrparams.headPoseState.pose.position.y = posePos.y;
        rvrparams.headPoseState.pose.position.z = posePos.z;

        rvrparams.headPoseState.pose.rotation.x = poseRot.x;
        rvrparams.headPoseState.pose.rotation.y = poseRot.y;
        rvrparams.headPoseState.pose.rotation.z = poseRot.z;
        rvrparams.headPoseState.pose.rotation.w = poseRot.w;
    } else {
        rvrparams.headPoseState.pose.position.x = pFrameParams->headPoseState.pose.position.x;
        rvrparams.headPoseState.pose.position.y = pFrameParams->headPoseState.pose.position.y;
        rvrparams.headPoseState.pose.position.z = pFrameParams->headPoseState.pose.position.z;

        rvrparams.headPoseState.pose.rotation.x = pFrameParams->headPoseState.pose.rotation.x;
        rvrparams.headPoseState.pose.rotation.y = pFrameParams->headPoseState.pose.rotation.y;
        rvrparams.headPoseState.pose.rotation.z = pFrameParams->headPoseState.pose.rotation.z;
        rvrparams.headPoseState.pose.rotation.w = pFrameParams->headPoseState.pose.rotation.w;
    }

    if(mRVRClient->Submit(rvrparams) != 0) {
        return SXR_ERROR_UNKNOWN;
    }

    return SXR_ERROR_NONE;
}

SxrResult SxrServiceClientManager::GetHeadPose(sxrHeadPoseState &poseState)
{
    SXRHeadPoseState headPose;
    mRVRClient->GetHeadPose(headPose);
    poseState.pose.rotation.x = headPose.pose.rotation.x;
    poseState.pose.rotation.y = headPose.pose.rotation.y;
    poseState.pose.rotation.z = headPose.pose.rotation.z;
    poseState.pose.rotation.w = headPose.pose.rotation.w;

    poseState.pose.position.x = headPose.pose.position.x;
    poseState.pose.position.y = headPose.pose.position.y;
    poseState.pose.position.z = headPose.pose.position.z;

    poseState.poseTimeStampNs = headPose.poseTimeStampNs;
    LOGV("[SxrServiceClientManager] sxrGetPredictedHeadPose (%f, %f, %f, %f) (%f, %f, %f) (%d us)",
        poseState.pose.rotation.x, poseState.pose.rotation.y, poseState.pose.rotation.z, poseState.pose.rotation.w,
        poseState.pose.position.x, poseState.pose.position.y, poseState.pose.position.z,
        (int)(poseState.poseTimeStampNs / 1000));

    poseState.poseStatus = kTrackingRotation | kTrackingPosition;

    return SXR_ERROR_NONE;
}

SxrResult SxrServiceClientManager::SetNativeWindowInfo(const SXRGLInfo &info)
{
    if(!mRVRClient) {
        return SXR_ERROR_UNKNOWN;
    }
    mRVRClient->SetNativeWindow((SXRGLInfo*)&info);

    return SXR_ERROR_NONE;
}

/*
Read svr configuration file without QVRService interface
*/
bool SxrServiceClientManager::ReadLocalSvrConfigFile(std::string &str)
{
    bool bConfigAvailable = true;
    std::string sSvrConfigFile("/system/etc/qvr/svrapi_config.txt");

    std::ifstream fStr(sSvrConfigFile);

    if(!fStr.is_open()){
        sSvrConfigFile = "/system_ext/etc/qvr/svrapi_config.txt";
        fStr.open(sSvrConfigFile);
        if(!fStr.is_open()){
            LOGE("[SxrServiceClientManager] Failed to open svr config file: %s", sSvrConfigFile.c_str());       
            bConfigAvailable = false;
        }
    }
    if(bConfigAvailable)
    {
        std::stringstream strStream;
        strStream << fStr.rdbuf();
        str = strStream.str();
        fStr.close();

        LOGI("[SxrServiceClientManager] svr api config: len = %d", (int)str.size());
    }
    return bConfigAvailable;
}

/*
Update render frustum based on configuration
*/
void SxrServiceClientManager::UpdateRenderFrustum()
{
    if(gRVRRenderFov > 0.01f) {
        gRVRRenderFovX = gRVRRenderFov;
        gRVRRenderFovY = gRVRRenderFov;
    }
    if(gRVRRenderFovX > 0.01f && gRVRRenderFovY > 0.01f) {        
        /* override display frustum values*/
        LOGI("[SxrServiceClientManager] display frustum left (%0.4f, %0.4f, %0.4f, %0.4f) ",
            gLeftFrustum_Left, gLeftFrustum_Right, gLeftFrustum_Top, gLeftFrustum_Bottom);
        LOGI("[SxrServiceClientManager] display frustum right (%0.4f, %0.4f, %0.4f, %0.4f) ",
            gRightFrustum_Left, gRightFrustum_Right, gRightFrustum_Top, gRightFrustum_Bottom);

        float ToRadian = 0.01745329252f;
        float localRVRRenderFovX = gRVRRenderFovX * ToRadian;
        float localRVRRenderFovY = gRVRRenderFovY * ToRadian;
        float leftThetaA = atan2(fabs(gLeftFrustum_Left), gLeftFrustum_Near);
        float leftThetaB = atan2(fabs(gLeftFrustum_Right), gLeftFrustum_Near);
        float leftPhiA = atan2(fabs(gLeftFrustum_Top), gLeftFrustum_Near);
        float leftPhiB = atan2(fabs(gLeftFrustum_Bottom), gLeftFrustum_Near);

        float rightThetaA = atan2(fabs(gRightFrustum_Left), gRightFrustum_Near);
        float rightThetaB = atan2(fabs(gRightFrustum_Right), gRightFrustum_Near);
        float rightPhiA = atan2(fabs(gRightFrustum_Top), gRightFrustum_Near);
        float rightPhiB = atan2(fabs(gRightFrustum_Bottom), gRightFrustum_Near);

        float fovDeltaX = localRVRRenderFovX - (leftThetaA + leftThetaB);
        float fovDeltaY = localRVRRenderFovY - (leftPhiA   + leftPhiB);

        float new0 = tan(leftThetaA + (leftThetaA * fovDeltaX)/(leftThetaA + leftThetaB)) * gLeftFrustum_Near;
        float new1 = tan(leftThetaB + (leftThetaB * fovDeltaX)/(leftThetaA + leftThetaB)) * gLeftFrustum_Near;
        gLeftFrustum_Left = new0 * gLeftFrustum_Left / fabs(gLeftFrustum_Left);
        gLeftFrustum_Right = new1 * gLeftFrustum_Right / fabs(gLeftFrustum_Right);

        new0 = tan(leftPhiA + (leftPhiA * fovDeltaY)/(leftPhiA + leftPhiB)) * gLeftFrustum_Near;
        new1 = tan(leftPhiB + (leftPhiB * fovDeltaY)/(leftPhiA + leftPhiB)) * gLeftFrustum_Near;
        gLeftFrustum_Top = new0 * gLeftFrustum_Top / fabs(gLeftFrustum_Top);;
        gLeftFrustum_Bottom = new1 * gLeftFrustum_Bottom / fabs(gLeftFrustum_Bottom);;

        fovDeltaX = localRVRRenderFovX - (rightThetaA + rightThetaB);
        fovDeltaY = localRVRRenderFovY - (rightPhiA   + rightPhiB);

        new0 = tan(rightThetaA + (rightThetaA * fovDeltaX)/(rightThetaA + rightThetaB)) * gRightFrustum_Near;
        new1 = tan(rightThetaB + (rightThetaB * fovDeltaX)/(rightThetaA + rightThetaB)) * gRightFrustum_Near;
        gRightFrustum_Left = new0 * gRightFrustum_Left / fabs(gRightFrustum_Left);;
        gRightFrustum_Right = new1 * gRightFrustum_Right / fabs(gRightFrustum_Right);;

        new0 = tan(rightPhiA + (rightPhiA * fovDeltaY)/(rightPhiA + rightPhiB)) * gRightFrustum_Near;
        new1 = tan(rightPhiB + (rightPhiB * fovDeltaY)/(rightPhiA + rightPhiB)) * gRightFrustum_Near;
        gRightFrustum_Top = new0 * gRightFrustum_Top / fabs(gRightFrustum_Top);;
        gRightFrustum_Bottom = new1 * gRightFrustum_Bottom / fabs(gRightFrustum_Bottom);;
        LOGI("[SxrServiceClientManager] render frustum left (%0.4f, %0.4f, %0.4f, %0.4f) ",
            gLeftFrustum_Left, gLeftFrustum_Right, gLeftFrustum_Top, gLeftFrustum_Bottom);
        LOGI("[SxrServiceClientManager] render frustum right (%0.4f, %0.4f, %0.4f, %0.4f) ",
            gRightFrustum_Left, gRightFrustum_Right, gRightFrustum_Top, gRightFrustum_Bottom);
    }
}

