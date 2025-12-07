//=============================================================================
// FILE: app.h
//
//                  Copyright (c) 2018 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================

#pragma once

#include <math.h>
#include "sxrApi.h"
#include "svrApplication.h"
#include "svrGeometry.h"
#include "svrRenderTarget.h"
#include "svrShader.h"
#include "svrUtil.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/transform.hpp"

#include "utils.h"

class CameraApp : public Svr::SvrApplication {

public:
    CameraApp();

    void Initialize();

    void LoadConfiguration() {};

    void Update();

    void Render();

    void Shutdown();

    void RenderEye(Svr::SvrEyeId);

    void SubmitFrame();


private:

    //pose state
    sxrHeadPoseState mPoseState;

};