//=============================================================================
// FILE: app.cpp
//
//                  Copyright (c) 2018 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
//                         Advanced Layer VR 
//=============================================================================
//This app demonstrates how to dislay the camera as a layer in VR app. 
//=============================================================================
//                        Introduction for this app
//=============================================================================
//This app will display camera video feed as the only layer for time warp.
//=============================================================================
#include "app.h"

using namespace Svr;

CameraApp::CameraApp()
{
}

//Callback function of SvrApplication, it will be called before the VR mode start
// prepare the rendering data here.
void CameraApp::Initialize()
{
    SvrApplication::Initialize();
    mAppContext.isCameraLayerEnabled = true;
}

//Submit the rendered frame
void CameraApp::SubmitFrame()
{
    sxrFrameParams frameParams;
    memset(&frameParams, 0, sizeof(frameParams));
    frameParams.frameIndex = mAppContext.frameCount;

    int numLayers = 0;

    /*
     * Left Eye (bottom layer)
     */

    // XRSDK must be compiled with -DENABLE_CAMERA for camera to work
    frameParams.renderLayers[numLayers].imageType = kTypeCamera;  
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskLeft;
    frameParams.renderLayers[numLayers].layerFlags = kLayerFlagOpaque;

    numLayers++;

    /*
     * Right Eye (bottom layer)
     */

    // XRSDK must be compiled with -DENABLE_CAMERA for camera to work
    frameParams.renderLayers[numLayers].imageType = kTypeCamera;  
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskRight;
    frameParams.renderLayers[numLayers].layerFlags = kLayerFlagOpaque;

    numLayers++;

    frameParams.minVsyncs = 1;
    sxrSubmitFrame(&frameParams);
    mAppContext.eyeBufferIndex = (mAppContext.eyeBufferIndex + 1) % SVR_NUM_EYE_BUFFERS;
}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame. Update view matrix and projection matrix here.
void CameraApp::Update()
{
    //base class Update
    SvrApplication::Update();

    /*
     * Hit '3' key to pause, '4' key to resume VR
     */
    if (mInput.IsKeyDown(kSvrKey_3))
    {
        SxrResult result = sxrSetXrPause();
        if (result != SXR_ERROR_NONE)
        {
            LOGE("  sxrSetXrPause Failed");
        }
    }
    else if (mInput.IsKeyDown(kSvrKey_4))
    {
        SxrResult result = sxrSetXrResume();
        if (result != SXR_ERROR_NONE)
        {
            LOGE("  sxrSetXrPause Failed");
        }
    }
}

//Render content for one eye
//SvrEyeId: the id of the eye, kLeft for the left eye, kRight for the right eye.
void CameraApp::RenderEye(Svr::SvrEyeId eyeId)
{
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Bind();
    glViewport(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight);
    glScissor(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sxrBeginEye((sxrWhichEye) eyeId);

    sxrEndEye((sxrWhichEye) eyeId);
	mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Unbind();

}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame after Update(), render for two eyes and submit the frame.
void CameraApp::Render()
{
    SubmitFrame();
}

//Callback function of SvrApplication, called when VR mode stop
//Clean up the model texture
void CameraApp::Shutdown()
{
    SvrApplication::Shutdown();
}

namespace Svr {
    //Return your own SvrApplication instance
    Svr::SvrApplication *CreateApplication()
    {
        return new CameraApp();
    }
}