//=============================================================================
// FILE: svrApiSpaceWarp.cpp
//                  Copyright (c) 2017 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>
#include <android/native_window.h>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/transform.hpp"

#include "svrCpuTimer.h"
#include "svrGpuTimer.h"
#include "svrGeometry.h"
#include "svrProfile.h"
#include "svrShader.h"
#include "svrUtil.h"
#include "svrConfig.h"
#include "svrRenderExt.h"

#include "private/svrApiCore.h"
#include "private/svrApiEvent.h"
#include "private/svrApiHelper.h"
#include "private/svrApiTimeWarp.h"

using namespace Svr;

#ifdef ENABLE_MOTION_VECTORS
VAR(bool, gEnableMotionVectors, false, kVariableNonpersistent);         //Enables motion vector support
VAR(bool, gForceAppEnableMotionVectors, false, kVariableNonpersistent); //Force motion vectors for all applications. Otherwise up to application
VAR(bool, gUseMotionVectors, true, kVariableNonpersistent);             //For Power: Whether or not to use motion data
VAR(bool, gLogMotionVectors, false, kVariableNonpersistent);            //Enables logging of motion vector activities
VAR(bool, gSmoothMotionVectors, true, kVariableNonpersistent);          //Motion data is average of surrounding samples
VAR(bool, gSmoothMotionVectorsWithGPU, true, kVariableNonpersistent);   //Motion data smoothing is done on the GPU
VAR(bool, gRenderMotionVectors, false, kVariableNonpersistent);         //Enables display of motion vectors as one of the eye buffers
VAR(bool, gRenderMotionInput, false, kVariableNonpersistent);           //Enables display of motion input. Current/Previous are the Left/Right eyes (for checking warp). Subject to gRenderMotionVectors
VAR(bool, gGenerateBothEyes, false, kVariableNonpersistent);            //Generate separate motion data for each eye. Otherwise, same data is used for both
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS
SvrMotionData *gpMotionData = NULL;

uint32_t gMostRecentMotionLeft = 0;
uint32_t gMostRecentMotionRight = 0;

// If we are using spacewarp, but not on a frame, we need a stub result texture
GLuint gStubResultTexture = 0;

#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS
bool L_CreateMotionVectorResult(SvrMotionData *pMotionData)
{

    LOGI("**************************************************");
    LOGI("Creating %dx%d motion result texture...", pMotionData->meshWidth, pMotionData->meshHeight);
    LOGI("**************************************************");

    GL(glGenTextures(1, &pMotionData->resultTexture));
    GL(glBindTexture(GL_TEXTURE_2D, pMotionData->resultTexture));

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // This is RGBA only float data
    int numChannels = 4;    // RG will be XY motion vectors
    int numItems = pMotionData->meshWidth * pMotionData->meshHeight * numChannels;
    float *pData = (float *)new float[pMotionData->meshWidth * pMotionData->meshHeight * numChannels];
    if (pData == NULL)
    {
        LOGE("Unable to allocate %d bytes for texture memory!", numItems * (int)sizeof(float));
        return false;
    }
    memset(pData, 0, numItems * sizeof(float));

    for (GLuint uiRowIndx = 0; uiRowIndx < pMotionData->meshWidth; uiRowIndx++)
    {
        for (GLuint uiColIndx = 0; uiColIndx < pMotionData->meshHeight; uiColIndx++)
        {
            int iIndx = (uiColIndx * pMotionData->meshWidth + uiRowIndx) * numChannels;

            // Default value that can show up if rendering.  Should be updated by actual motion vectors if everything is working.
            pData[iIndx + 0] = 0.0f;
            pData[iIndx + 1] = 0.0f;
            pData[iIndx + 2] = 0.0f;
            pData[iIndx + 3] = 0.0f;
        }
    }

    GL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, pMotionData->meshWidth, pMotionData->meshHeight));

    // No longer need the data since passed off to OpenGL
    delete pData;

    GL(glBindTexture(GL_TEXTURE_2D, 0));

    LOGI("    Result texture (%d) created", pMotionData->resultTexture);

    return true;
}
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS
bool L_CreateStubMotionVectorResult()
{
    uint32_t stubWidth = 4;
    uint32_t stubHeight = 4;

    LOGI("**************************************************");
    LOGI("Creating %dx%d stub motion result texture...", stubWidth, stubHeight);
    LOGI("**************************************************");

    GL(glGenTextures(1, &gStubResultTexture));
    GL(glBindTexture(GL_TEXTURE_2D, gStubResultTexture));

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // This is RGBA only float data
    int numChannels = 4;    // RG will be XY motion vectors
    int numItems = stubWidth * stubHeight * numChannels;
    float *pData = (float *)new float[stubWidth * stubHeight * numChannels];
    if (pData == NULL)
    {
        LOGE("Unable to allocate %d bytes for texture memory!", numItems * (int)sizeof(float));
        return false;
    }
    memset(pData, 0, numItems * sizeof(float));

    for (GLuint uiRowIndx = 0; uiRowIndx < stubWidth; uiRowIndx++)
    {
        for (GLuint uiColIndx = 0; uiColIndx < stubHeight; uiColIndx++)
        {
            int iIndx = (uiColIndx * stubWidth + uiRowIndx) * numChannels;

            // For testing, can set this to some offset to prove it is using the stub
            pData[iIndx + 0] = 0.0f;
            pData[iIndx + 1] = 0.0f;
            pData[iIndx + 2] = 0.0f;
            pData[iIndx + 3] = 0.0f;
        }
    }

    GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, stubWidth, stubHeight, 0, GL_RGBA, GL_FLOAT, pData));

    // No longer need the data since passed off to OpenGL
    delete pData;

    GL(glBindTexture(GL_TEXTURE_2D, 0));

    LOGI("    Result texture created");

    return true;
}
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS
void L_DestroyMotionVectorResult(SvrMotionData *pMotionData)
{
    LOGI("Destroying Motion Vector Result. Handle = %d", pMotionData->resultTexture);
    if (pMotionData->resultTexture != 0)
    {
        GL(glDeleteTextures(1, &pMotionData->resultTexture));
    }
    pMotionData->resultTexture = 0;

}
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS
void L_DestroyStubMotionVectorResult()
{
    LOGI("Destroying Stub Motion Vector Result. Handle = %d", gStubResultTexture);
    if (gStubResultTexture != 0)
    {
        GL(glDeleteTextures(1, &gStubResultTexture));
    }
    gStubResultTexture = 0;
}
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS
void TerminateMotionVectors()
{
    // Start/Stop logic tells us we may or may not be in an initialized state
    if (gpMotionData == NULL)
        return;

    LOGI("****************************************");
    LOGI("Terminating Motion Vectors...");
    LOGI("****************************************");
    L_DestroyMotionVectorResult(&gpMotionData[0]);
    L_DestroyMotionVectorResult(&gpMotionData[1]);

    L_DestroyStubMotionVectorResult();

    // Release the list of structures
    LOGI("Releasing motion vector memory...");
    delete[] gpMotionData;
    gpMotionData = NULL;
}
#endif // ENABLE_MOTION_VECTORS

#if defined(ENABLE_MOTION_VECTORS)
void InitializeMotionVectors()
{
    if (gLogMotionVectors)
    {
        LOGI("Initializing Motion Vectors...");
    }

    // Start/Stop logic tells us we may or may not be in an initialized state
    if (gpMotionData != NULL)
    {
        TerminateMotionVectors();
    }

    // Allocate our list of structures
    LOGI("**************************************************");
    LOGI("**************************************************");
    LOGI("Allocating motion data structures!!!");
    LOGI("**************************************************");
    LOGI("**************************************************");
    gpMotionData = new SvrMotionData[NUM_MOTION_VECTOR_BUFFERS];
    if (gpMotionData == NULL)
    {
        LOGE("InitializeMotionVectors: Unable to allocate %d bytes for motion data structures!", NUM_MOTION_VECTOR_BUFFERS * (int)sizeof(SvrMotionData));
        return;
    }

    bool result = ME_Init(gAppContext->deviceInfo.targetEyeWidthPixels,
            gAppContext->deviceInfo.targetEyeHeightPixels,
            true,
            &gpMotionData[0].meshWidth,
            &gpMotionData[0].meshHeight);

    if (result == false)
    {
        LOGE("InitializeMotionVectors: ME_Init failed!");
    }

    gpMotionData[1].meshWidth  = gpMotionData[0].meshWidth;
    gpMotionData[1].meshHeight = gpMotionData[0].meshHeight;

    L_CreateMotionVectorResult(&gpMotionData[0]);
    L_CreateMotionVectorResult(&gpMotionData[1]);

    L_CreateStubMotionVectorResult();
}
#endif // ENABLE_MOTION_VECTORS

#ifdef ENABLE_MOTION_VECTORS

#include "sys/system_properties.h"
int android_property_get2(const char *key, char *value, const char *default_value)
{
     int iReturn = __system_property_get(key, value);
     if (!iReturn) strcpy(value, default_value);
     return iReturn;
}

void GenerateMotionData(svrFrameParamsInternal* pWarpFrame)
{
    static int              previous[2]      = {0};
    static sxrHeadPoseState previousHeadPose;//C++ default-initializes every static datafield to zero

    if (gpMotionData == NULL)
    {
        LOGE("Unable to generate motion data: Functions have not been initialized!");
        return;
    }

    GLfloat tm[9];

    if (previous[0] != 0)
    {
        // Need warp matrix to remove head motion from motion data
        glm::fquat oldQuat = glmQuatFromSvrQuat(pWarpFrame->frameParams.headPoseState.pose.rotation);
        glm::fquat newQuat = glmQuatFromSvrQuat(previousHeadPose.pose.rotation);

        // Need to get the difference between new and old
        glm::fquat recenterRot = inverse(oldQuat);
        glm::fquat diffQuat = newQuat * recenterRot;

        // Rotation Matrix around Z:
        //      Cos -Sin  0
        //      Sin  Cos  0
        //        0    0  1
        glm::vec3 diffEuler = glm::eulerAngles(diffQuat);
        glm::mat3 transMat = glm::mat3(1.0f);

        transMat[0].x = cos(diffEuler.z);
        transMat[0].y = -sin(diffEuler.z);
        transMat[1].x = -transMat[0].y;
        transMat[1].y = transMat[0].x;
        // LOGI("DEBUG! Diff Quat (Euler): (Pitch = %0.4f; Yaw = %0.4f; Roll = %0.4f)", diffEuler.x, diffEuler.y, diffEuler.z);

        glm::vec3 Forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 LookDir = diffQuat * Forward;
        // LOGI("DEBUG! Look Dir: (%0.4f, %0.4f, %0.4f)", -LookDir.x, -LookDir.y, LookDir.z);

        // We now have a vector that we need to set Z to be the distance to near plane
        if(fabs(LookDir.z) > 0.000001f)   // Epsilon check so not dividing by zero
            LookDir *= (gAppContext->deviceInfo.leftEyeFrustum.near / LookDir.z);
        // LOGI("DEBUG!    Adjust Near: (%0.4f, %0.4f, %0.4f)", -LookDir.x, -LookDir.y, LookDir.z);

        float xTotal = (gAppContext->deviceInfo.leftEyeFrustum.right - gAppContext->deviceInfo.leftEyeFrustum.left);
        float xAdjust = 0.0f;
        if (xTotal != 0.0f)
            xAdjust = LookDir.x / xTotal;

        float yTotal = (gAppContext->deviceInfo.leftEyeFrustum.top - gAppContext->deviceInfo.leftEyeFrustum.bottom);
        float yAdjust = 0.0f;
        if (yTotal != 0.0f)
            yAdjust = LookDir.y / yTotal;

        // Screen is [-1, 1] and adjustment is [0,1]
        xAdjust *= 2.0f;
        yAdjust *= 2.0f;
        // LOGI("DEBUG!    Screen Adjust: (%0.4f, %0.4f)", xAdjust, yAdjust);

        // Need the translation in the transform matrix
        transMat[2].x = xAdjust;
        transMat[2].y = yAdjust;

        tm[0] = transMat[0].x; tm[1] = transMat[0].y; tm[2] = transMat[0].z;
        tm[3] = transMat[1].x; tm[4] = transMat[1].y; tm[5] = transMat[1].z;
        tm[6] = transMat[2].x; tm[7] = transMat[2].y; tm[8] = transMat[2].z;
    }

    for (uint32_t whichEye = 0; whichEye < 2; whichEye++)
    {
        if (previous[whichEye] != 0)
        {
            if (gLogMotionVectors)
            {
                LOGI("%s: ME_GenerateVectors(%d, %d) -> %d .... %d",
                     (pWarpFrame->frameParams.renderLayers[whichEye].eyeMask == kEyeMaskLeft) ? "Left" : "Right",
                      pWarpFrame->frameParams.renderLayers[whichEye].imageHandle,
                      previous[whichEye],
                      gpMotionData[whichEye].resultTexture, whichEye);
            }

            bool result = ME_GenerateVectors(
                    pWarpFrame->frameParams.renderLayers[whichEye].imageHandle,
                    previous[whichEye],
                    tm,
					false,
                    gpMotionData[whichEye].resultTexture);

            if (result == false)
            {
                LOGE("%s: ME_GenerateVectors(%d, %d) Failed!",
                    (pWarpFrame->frameParams.renderLayers[whichEye].eyeMask == kEyeMaskLeft) ? "Left" : "Right",
                    pWarpFrame->frameParams.renderLayers[whichEye].imageHandle,
                    previous[whichEye]);
            }
        }
        previous[whichEye] = pWarpFrame->frameParams.renderLayers[whichEye].imageHandle;

        if (pWarpFrame->frameParams.renderLayers[whichEye].eyeMask == kEyeMaskLeft)
	{
            gMostRecentMotionLeft = whichEye;
        }
        else
	{
            gMostRecentMotionRight = whichEye;
        }
    } 
    previousHeadPose = pWarpFrame->frameParams.headPoseState;

    unsigned int enabled = 0;
    char enabledStr[255];
    android_property_get2("debug.motionengine.showvectors", enabledStr, "0");
    enabled = atoi(enabledStr);

    gRenderMotionVectors = ((enabled == 1) ? true : false);
    gUseMotionVectors = !gRenderMotionVectors;
}
#endif // ENABLE_MOTION_VECTORS
