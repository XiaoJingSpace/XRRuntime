//=============================================================================
// FILE: SxrServiceClientManager.h
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================

#ifndef _SXR_SERVICE_CLIENT_MANAGER_H_
#define _SXR_SERVICE_CLIENT_MANAGER_H_

#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "SXRServiceClient.hpp"
#include "sxrApi.h"

class SxrServiceClientManager
{
public:
    SxrServiceClientManager();
    virtual ~SxrServiceClientManager();

    SxrResult Init();
    void      Release();

    SxrResult SubmitFrame(const sxrFrameParams* pFrameParams, glm::fquat* recenterRot, glm::vec3* recenterPos);
    SxrResult GetHeadPose(sxrHeadPoseState &pose);
    SxrResult SetNativeWindowInfo(const SXRGLInfo &info);

    static bool ReadLocalSvrConfigFile(std::string &str);
    static void UpdateRenderFrustum();
private:
    ISXRServiceClient*    mServiceClient = nullptr;
    ISXRRVRServiceClient* mRVRClient = nullptr;
};
#endif //_SXR_SERVICE_CLIENT_MANAGER_H_
