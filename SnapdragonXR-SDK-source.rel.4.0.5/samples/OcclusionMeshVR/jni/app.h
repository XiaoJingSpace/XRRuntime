//=============================================================================
// FILE: app.h
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================

#pragma once

#include <glm/glm.hpp>

#include "sxrApi3dr.h"
#include "sxrApi.h"
#include "svrApplication.h"
#include "svrGeometry.h"
#include "svrShader.h"
#include "svrUtil.h"
#include "utils.h"

class OcclusionMeshApp : public Svr::SvrApplication {

public:
    OcclusionMeshApp();

    void Initialize() override;

    void LoadConfiguration() override;

    void Update() override;

    void Render() override;

    void Shutdown() override;

private:
    void RenderEye(Svr::SvrEyeId);

    void SubmitFrame();

    Svr::SvrGeometry CreateCube(const float width);

    static constexpr int NUM_OBJECT = 20;

    //SvrGeometry
    Svr::SvrGeometry mCube;
    //position of each object
    glm::mat4 mCubeModelMat[NUM_OBJECT];
    //cube color
    glm::vec4 mCubeColor;

    //Use SvrShader to load vertex and fragment shaders from file.
    Svr::SvrShader mLightShader;

    //pose state
    sxrHeadPoseState mPoseState;

    //view matrix
    glm::mat4 mViewMatrix[kNumEyes];

    //projection matrix
    glm::mat4 mProjectionMatrix[kNumEyes];

    //light position
    glm::vec3 mLightPosition;

    //light color
    glm::vec3 mLightColor;
};
