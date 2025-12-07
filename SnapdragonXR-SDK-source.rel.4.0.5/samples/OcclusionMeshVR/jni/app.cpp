//=============================================================================
// FILE: app.cpp
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
//                         Occlusion Mesh VR
//=============================================================================
//This app demonstrates how to integrate camera layer, 3DR mesh and occlusion mesh in VR app.
//=============================================================================
//                        Introduction for this app
//=============================================================================
// This app will display camera video feed as the base layer for time warp with occlusion mesh
// from realtime reconstructed mesh data.
// We would also render NUM_OBJECT cube in random position to show that the virtual cube
// would be occluded by the actual environment.
//=============================================================================
#include "app.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace Svr;

#define BUFFER_SIZE 512
#define OBJECT_DISTRIBUTE_RADIUS 3.0f

// 3DR constants
#define CUBE_WIDTH 0.3f

#define LOG_TAG "OcclusionMeshVR"
#define ALOGI(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

OcclusionMeshApp::OcclusionMeshApp() :
        mLightPosition(0.0f, 4.0f, 0.0f),
        mLightColor(1.0f)
{
    mCubeColor = glm::vec4(34.0f, 64.0f, 240.0f, 255.0f) / 255.0f;
}

//Callback function of SvrApplication, it will be called before the VR mode start
// prepare the rendering data here.
void OcclusionMeshApp::Initialize()
{
    SvrApplication::Initialize();
    mAppContext.isCameraLayerEnabled = true;

    Sxr3drResult result = sxr3drInitialize();
    if (result != SXR_3DR_SUCCESS)
    {
        ALOGW("sxr3drInitialize error: %d", result);
    }

    char vsFilePath[BUFFER_SIZE];
    char fsFilePath[BUFFER_SIZE];

    //load the vertex shader model_v.glsl and fragment shader model_f.glsl
    sprintf(vsFilePath, "%s/%s", mAppContext.externalPath, "model_v.glsl");
    sprintf(fsFilePath, "%s/%s", mAppContext.externalPath, "model_f.glsl");
    Svr::InitializeShader(mLightShader, vsFilePath, fsFilePath, "Vs", "Fs");

    mCube = CreateCube(CUBE_WIDTH);

    //prepare the model matrix
    for (int i = 0; i < NUM_OBJECT; ++i)
    {
        mCubeModelMat[i] = glm::translate(glm::mat4(1.0f), glm::ballRand(OBJECT_DISTRIBUTE_RADIUS));
    }
}

void OcclusionMeshApp::LoadConfiguration()
{
}

//Submit the rendered frame
void OcclusionMeshApp::SubmitFrame()
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

    /*
     * Left Eye (top layer)
     */
    frameParams.renderLayers[numLayers].imageType = kTypeTexture;
    frameParams.renderLayers[numLayers].imageHandle = mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[Svr::kLeft].GetColorAttachment();
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskLeft;

    numLayers++;

    /*
     * Right Eye (top layer)
     */
    frameParams.renderLayers[numLayers].imageType = kTypeTexture;
    frameParams.renderLayers[numLayers].imageHandle = mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[Svr::kRight].GetColorAttachment();
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskRight;

    numLayers++;

    frameParams.headPoseState = mPoseState;
    frameParams.minVsyncs = 1;
    sxrSubmitFrame(&frameParams);
    mAppContext.eyeBufferIndex = (mAppContext.eyeBufferIndex + 1) % SVR_NUM_EYE_BUFFERS;
}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame. Update view matrix and projection matrix here.
void OcclusionMeshApp::Update()
{
    //base class Update
    SvrApplication::Update();

    sxrDeviceInfo di = sxrGetDeviceInfo();

    //update view matrix
    float predictedTimeMs = sxrGetPredictedDisplayTime();
    mPoseState = sxrGetPredictedHeadPose(predictedTimeMs);

    // add in changes for view matrix view customization parameters
    glm::vec3 LeftEyePos(di.leftEyeFrustum.position.x, di.leftEyeFrustum.position.y,
                         di.leftEyeFrustum.position.z);
    glm::fquat LeftEyeRot(di.leftEyeFrustum.rotation.w, di.leftEyeFrustum.rotation.x,
                          di.leftEyeFrustum.rotation.y,
                          di.leftEyeFrustum.rotation.z);  // glm quat is (w)(xyz)
    glm::vec3 RightEyePos(di.rightEyeFrustum.position.x, di.rightEyeFrustum.position.y,
                          di.rightEyeFrustum.position.z);
    glm::fquat RightEyeRot(di.rightEyeFrustum.rotation.w, di.rightEyeFrustum.rotation.x,
                           di.rightEyeFrustum.rotation.y,
                           di.rightEyeFrustum.rotation.z);  // glm quat is (w)(xyz)

    SvrGetEyeViewMatrices(mPoseState,
                          true, // use a head model?
                          LeftEyePos, RightEyePos,
                          LeftEyeRot, RightEyeRot,
                          DEFAULT_HEAD_HEIGHT, DEFAULT_HEAD_DEPTH,
                          mViewMatrix[kLeft],
                          mViewMatrix[kRight]);

    //update projection matrix
    sxrViewFrustum *pLeftFrust = &di.leftEyeFrustum;
    mProjectionMatrix[kLeft] = glm::frustum(pLeftFrust->left, pLeftFrust->right,
                                            pLeftFrust->bottom, pLeftFrust->top,
                                            pLeftFrust->near, pLeftFrust->far);

    sxrViewFrustum *pRightFrust = &di.rightEyeFrustum;
    mProjectionMatrix[kRight] = glm::frustum(pRightFrust->left, pRightFrust->right,
                                             pRightFrust->bottom, pRightFrust->top,
                                             pRightFrust->near, pRightFrust->far);
 
    if ((mAppContext.frameCount % 100) == 0)
    {
        // Update occlusion mesh
        uint32_t numVertices = 0, numIndices = 0;
        sxrVector3 *vertices;
        uint32_t *indices;
        Sxr3drResult result = sxr3drGetSurfaceMesh(&numVertices, &vertices, &numIndices,
            &indices);
        if (result == SXR_3DR_SUCCESS)
        {
            if (numVertices > 0 && numIndices > 0)
            {
                sxrSetOcclusionMesh((int)numVertices, (int)numIndices, (float *)vertices,
                    (unsigned int *)indices);
            }
            sxr3drReleaseSurfaceMesh();
        }
    }
}

//Render content for one eye
//SvrEyeId: the id of the eye, kLeft for the left eye, kRight for the right eye.
void OcclusionMeshApp::RenderEye(Svr::SvrEyeId eyeId)
{
    GL(glEnable(GL_SCISSOR_TEST));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_CULL_FACE));
    GL(glDepthFunc(GL_LESS));
    GL(glDepthMask(GL_TRUE));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glEnable(GL_BLEND));

    mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Bind();
    GL(glViewport(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight));
    GL(glScissor(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight));
    GL(glClearColor(0.2f, 0.2f, 0.2f, 0.0f));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    sxrBeginEye((sxrWhichEye) eyeId);
    sxrOccludeEye((sxrWhichEye) eyeId, glm::value_ptr(mProjectionMatrix[eyeId]),
                  glm::value_ptr(mViewMatrix[eyeId]));

    mLightShader.Bind();
    mLightShader.SetUniformMat4("view", mViewMatrix[eyeId]);
    mLightShader.SetUniformMat4("projection", mProjectionMatrix[eyeId]);
    mLightShader.SetUniformVec3("lightPosition", mLightPosition);
    mLightShader.SetUniformVec3("lightColor", mLightColor);

    // Render the cubes
    for (int i = 0; i < NUM_OBJECT; ++i)
    {
        // Cubes
        mLightShader.SetUniformMat4("model", mCubeModelMat[i]);
        mLightShader.SetUniformVec4("objectColor", mCubeColor);
        mCube.Submit();
    }
    mLightShader.Unbind();

    sxrEndEye((sxrWhichEye) eyeId);
    mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Unbind();

    // We enabled blending, put it back
    GL(glDisable(GL_BLEND));
}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame after Update(), render for two eyes and submit the frame.
void OcclusionMeshApp::Render()
{
    RenderEye(kLeft);
    RenderEye(kRight);
    SubmitFrame();
}

//Callback function of SvrApplication, called when VR mode stop
//Clean up the model texture
void OcclusionMeshApp::Shutdown()
{
    mLightShader.Destroy();
    mCube.Destroy();
    sxrSetOcclusionMesh(0, 0, NULL, NULL);
    sxr3drShutdown();
    SvrApplication::Shutdown();
}

Svr::SvrGeometry OcclusionMeshApp::CreateCube(const float width)
{
    // Create attributes of the cube
    unsigned int numElementsPerVert = 6;
    unsigned int numAttribs = 2;
    int stride = (int) (numElementsPerVert * sizeof(float));

    SvrProgramAttribute attribs[2] =
            {
                    {kPosition, 3, GL_FLOAT, false, stride, 0},
                    {kNormal,   3, GL_FLOAT, false, stride, 3 * sizeof(float)},
            };

    float halfWidth = width / 2.0f;

    // Create vertex data of the cube with position and normal data
    float cubeVerts[] = {
            // Front
            halfWidth, halfWidth, halfWidth, 0.0f, 0.0f, 1.0f,
            -halfWidth, halfWidth, halfWidth, 0.0f, 0.0f, 1.0f,
            -halfWidth, -halfWidth, halfWidth, 0.0f, 0.0f, 1.0f,
            halfWidth, -halfWidth, halfWidth, 0.0f, 0.0f, 1.0f,
            // Right
            halfWidth, halfWidth, halfWidth, 1.0f, 0.0f, 0.0f,
            halfWidth, -halfWidth, halfWidth, 1.0f, 0.0f, 0.0f,
            halfWidth, -halfWidth, -halfWidth, 1.0f, 0.0f, 0.0f,
            halfWidth, halfWidth, -halfWidth, 1.0f, 0.0f, 0.0f,
            // Top
            halfWidth, halfWidth, halfWidth, 0.0f, 1.0f, 0.0f,
            halfWidth, halfWidth, -halfWidth, 0.0f, 1.0f, 0.0f,
            -halfWidth, halfWidth, -halfWidth, 0.0f, 1.0f, 0.0f,
            -halfWidth, halfWidth, halfWidth, 0.0f, 1.0f, 0.0f,
            // Left
            -halfWidth, halfWidth, halfWidth, -1.0f, 0.0f, 0.0f,
            -halfWidth, halfWidth, -halfWidth, -1.0f, 0.0f, 0.0f,
            -halfWidth, -halfWidth, -halfWidth, -1.0f, 0.0f, 0.0f,
            -halfWidth, -halfWidth, halfWidth, -1.0f, 0.0f, 0.0f,
            // Bottom
            -halfWidth, -halfWidth, -halfWidth, 0.0f, -1.0f, 0.0f,
            halfWidth, -halfWidth, -halfWidth, 0.0f, -1.0f, 0.0f,
            halfWidth, -halfWidth, halfWidth, 0.0f, -1.0f, 0.0f,
            -halfWidth, -halfWidth, halfWidth, 0.0f, -1.0f, 0.0f,
            // Back
            halfWidth, -halfWidth, -halfWidth, 0.0f, 0.0f, -1.0f,
            -halfWidth, -halfWidth, -halfWidth, 0.0f, 0.0f, -1.0f,
            -halfWidth, halfWidth, -halfWidth, 0.0f, 0.0f, -1.0f,
            halfWidth, halfWidth, -halfWidth, 0.0f, 0.0f, -1.0f

    };
    int numCubeVerts = sizeof(cubeVerts) / (numElementsPerVert * sizeof(float));

    // Create index data of the cube
    unsigned int cubeIndices[] = {
            // Front
            0, 1, 2,
            2, 3, 0,
            // Right
            4, 5, 6,
            6, 7, 4,
            // Top
            8, 9, 10,
            10, 11, 8,
            // Left
            12, 13, 14,
            14, 15, 12,
            // Bottom
            16, 17, 18,
            18, 19, 16,
            // Back
            20, 21, 22,
            22, 23, 20

    };
    int numCubeIndices = sizeof(cubeIndices) / sizeof(int);

    SvrGeometry outCube;
    outCube.Initialize(&attribs[0], numAttribs, cubeIndices, numCubeIndices, cubeVerts,
                       numElementsPerVert * numCubeVerts * sizeof(float), numCubeVerts);
    return std::move(outCube);
}

namespace Svr {
    //Return your own SvrApplication instance
    Svr::SvrApplication *CreateApplication()
    {
        return new OcclusionMeshApp();
    }
}