//=============================================================================
// FILE: app.cpp
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
//                         Surface Mesh VR
//=============================================================================
//This app demonstrates how to integrate 3DR features in VR app.
//=============================================================================
//                        Introduction for this app
//=============================================================================
//This app do the following tasks,
// 1. Render surface mesh from 3DR api that is constructed based on the realtime environment.
// 2. Define ENABLE_COLLISION to 1 to render NUM_OBJECT of cubes and spheres distribute around
// OBJECT_DISTRIBUTE_RADIUS with different color depending on the result of collision test.
// 3. Define ENABLE_PLACEMENTIFO to 1 to get the targeting ray on the head, and do the placement
// information test with this targeting ray which would follow the head movement.
// 4. Define ENABLE_PLANE to 1 to enable the plane rendering with the plane data obtained from
// 3dr api. Each plane will be displayed in a different, random color.
//=============================================================================
#include "app.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/normal.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

using namespace Svr;

#define BUFFER_SIZE 512
#define OBJECT_DISTRIBUTE_RADIUS 3.0f

// 3DR options
#define ENABLE_SURFACEMESH 1
#define ENABLE_COLLISION 1
#define ENABLE_PLACEMENTIFO 1
#define ENABLE_PLANE 0
#define ENABLE_POINTCLOUD 0

// 3DR constants
#define CUBE_WIDTH 0.3f
#define SPHERE_RADIUS ((CUBE_WIDTH) / 2.0f)
#define COLLISION_MAX_CLEARANCE 0.0f
#define LOGO_SIZE 0.4f
#define LOGO_Y_OFFSET 0.05f
#define TARGET_RAY_RADIUS 0.003f
#define POINTCLOUD_SIZE 0.01f

#define LOG_TAG "SurfaceMeshVR"
#define ALOGI(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// target ray for placementInfo which was defined in head space
static const glm::vec3 RAY_START_POSITION = glm::vec3(0.0f, 0.2f, 0.0f);
static const glm::vec3 RAY_DIRECTION = glm::vec3(0.0f, 0.0f, -1.0f);
static const float RAY_MAX_DISTANCE = 10.0f;

static const glm::vec4 COLOR_COLLISION_UNKNOWN = glm::vec4(84.0f, 84.0f, 84.0f, 255.0f) / 255.0f;
static const glm::vec4 COLOR_COLLISION_NONE = glm::vec4(48.0f, 142.0f, 151.0f, 255.0f) / 255.0f;
static const glm::vec4 COLOR_COLLISION = glm::vec4(48.0f, 142.0f, 151.0f, 150.0f) / 255.0f;

SurfaceMeshApp::SurfaceMeshApp() :
        mMeshColor(0.79f, 0.79f, 0.79f, 1.0f),
        mShutdown(false),
        mLightPosition(0.0f, 4.0f, 0.0f),
        mLightColor(1.0f)
{
    mTargetingRayColor = glm::vec4(34.0f, 70.0f, 180.0f, 200.0f) / 255.0f;
    mPointCloudColor = glm::vec4(240.0f, 34.0f, 64.0f, 255.0f) / 255.0f;
    mLogoColor = glm::vec4(34.0f, 64.0f, 240.0f, 255.0f) / 255.0f;
}

//Callback function of SvrApplication, it will be called before the VR mode start
// prepare the rendering data here.
void SurfaceMeshApp::Initialize()
{
    SvrApplication::Initialize();

    Sxr3drResult result = sxr3drInitialize();
    if (result != SXR_3DR_SUCCESS)
    {
        ALOGW("sxr3drInitialize error: %d", result);
    }

    char vsFilePath[BUFFER_SIZE];
    char fsFilePath[BUFFER_SIZE];

    //load the vertex shader model_v.glsl and fragment shader model_f.glsl
    sprintf(vsFilePath, "%s/%s", mAppContext.externalPath, "pointcloud_v.glsl");
    sprintf(fsFilePath, "%s/%s", mAppContext.externalPath, "pointcloud_f.glsl");
    Svr::InitializeShader(mPointCloudShader, vsFilePath, fsFilePath, "Vs", "Fs");

    sprintf(vsFilePath, "%s/%s", mAppContext.externalPath, "model_v.glsl");
    sprintf(fsFilePath, "%s/%s", mAppContext.externalPath, "model_f.glsl");
    Svr::InitializeShader(mLightShader, vsFilePath, fsFilePath, "Vs", "Fs");

    sprintf(fsFilePath, "%s/%s", mAppContext.externalPath, "color_f.glsl");
    Svr::InitializeShader(mColorShader, vsFilePath, fsFilePath, "Vs", "Fs");

    sprintf(fsFilePath, "%s/%s", mAppContext.externalPath, "texture_f.glsl");
    Svr::InitializeShader(mTextureShader, vsFilePath, fsFilePath, "Vs", "Fs");

    //load the texture
    char tmpFilePath[BUFFER_SIZE];
    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, "grid.ktx");
    Svr::LoadTextureCommon(&mPlaneTexture, tmpFilePath);

#if ENABLE_SURFACEMESH
    std::vector<Svr::SvrProgramAttribute> attr = {{Svr::kPosition, 3, GL_FLOAT, false, sizeof(VertexNormal), 0},
                                                  {Svr::kNormal,   3, GL_FLOAT, false, sizeof(VertexNormal), sizeof(VertexNormal::vertex)}};
    mSurfaceMesh.Initialize(attr.data(), attr.size(), NULL, 0, NULL, 0, 0);
#endif

    mWorkerThread = std::make_unique<std::thread>([this]() { WorkInBackground(); });

    int nObjGeom;
    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, "q_logo.obj");
    SvrGeometry::CreateFromObjFile(tmpFilePath, &mLogo, nObjGeom);

    mCube = CreateCube(CUBE_WIDTH);
    mTargetingRay = CreateCone(TARGET_RAY_RADIUS, 20);
    mSphere = CreateSphere(SPHERE_RADIUS, 20, 20);

#if ENABLE_POINTCLOUD
    std::vector<Svr::SvrProgramAttribute> attrPointCloud = { { Svr::kPosition, 3, GL_FLOAT, false, sizeof(sxrCloudPoint), 0 } };
    mPointCloud.Initialize(attrPointCloud.data(), attrPointCloud.size(), NULL, 0, NULL, 0, 0);
    mPointCloudSize = POINTCLOUD_SIZE;
#endif

    //prepare the model matrix and model color
    for (int i = 0; i < NUM_OBJECT; ++i)
    {
        glm::vec3 randPosition(glm::ballRand(OBJECT_DISTRIBUTE_RADIUS));
        mCubeCenter[i] = {randPosition.x, randPosition.y, randPosition.z};
        randPosition = glm::ballRand(OBJECT_DISTRIBUTE_RADIUS);
        mSphereCenter[i] = {randPosition.x, randPosition.y, randPosition.z};
    }
    mCubeExtents = {CUBE_WIDTH, CUBE_WIDTH, CUBE_WIDTH};
    mSphereRadius = SPHERE_RADIUS;
}

void SurfaceMeshApp::LoadConfiguration()
{
}

//Submit the rendered frame
void SurfaceMeshApp::SubmitFrame()
{
    sxrFrameParams frameParams;
    memset(&frameParams, 0, sizeof(frameParams));
    frameParams.frameIndex = mAppContext.frameCount;

    frameParams.renderLayers[0].imageType = kTypeTexture;
    frameParams.renderLayers[0].imageHandle = mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[Svr::kLeft].GetColorAttachment();
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[0].imageCoords);
    frameParams.renderLayers[0].eyeMask = kEyeMaskLeft;

    frameParams.renderLayers[1].imageType = kTypeTexture;
    frameParams.renderLayers[1].imageHandle = mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[Svr::kRight].GetColorAttachment();
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[1].imageCoords);
    frameParams.renderLayers[1].eyeMask = kEyeMaskRight;

    frameParams.headPoseState = mPoseState;
    frameParams.minVsyncs = 1;
    sxrSubmitFrame(&frameParams);
    mAppContext.eyeBufferIndex = (mAppContext.eyeBufferIndex + 1) % SVR_NUM_EYE_BUFFERS;
}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame. Update view matrix and projection matrix here.
void SurfaceMeshApp::Update()
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

    // get qvr data transform
    sxrGetQvrDataTransform(&mQvrDataTransformMat);

    //update mesh data if needed
    if (m3drDataDirty)
    {
        std::lock_guard<std::mutex> lock(m3drDataMutex);

#if ENABLE_SURFACEMESH
        mSurfaceMesh.Update(mMeshData.vertices.data(),
                            mMeshData.vertices.size() * sizeof(VertexNormal),
                            mMeshData.vertices.size(), mMeshData.indices.data(),
                            mMeshData.indices.size());
#endif //ENABLE_SURFACEMESH
#if ENABLE_POINTCLOUD
        mPointCloud.Update(mCloudPoints.data(), mCloudPoints.size() * sizeof(sxrCloudPoint), mCloudPoints.size(), 
            mCloudIndices.data(), mCloudIndices.size());
#endif

#if ENABLE_PLANE
        // Update the planes
        unsigned int numElementsPerVert = 5;
        unsigned int numAttribs = 2;
        int stride = (int) (numElementsPerVert * sizeof(float));
        SvrProgramAttribute attribs[] = {{kPosition,  3, GL_FLOAT, false, stride, 0},
                                         {kTexcoord0, 2, GL_FLOAT, false, stride, 3 *
                                                                                  sizeof(float)}};
        for (const auto &it: mPlanesData)
        {
            const auto &data = it.second;
            const auto &planeId = it.first;
            if (mPlaneRenderData.find(planeId) == mPlaneRenderData.end())
            {
                // it is a new plane
                mPlaneRenderData[planeId].geometry.Initialize(&attribs[0], numAttribs, NULL, 0,
                                                              NULL, 0, 0);
            }
            if (data.rootPlaneId == planeId)
            {
                // this plane has not been subsumed
                mPlaneRenderData[planeId].geometry.Update(data.vertices.data(),
                                                          data.vertices.size() * sizeof(float),
                                                          data.vertices.size() / numElementsPerVert,
                                                          (unsigned int *) data.indices.data(),
                                                          data.indices.size());
            }
            else
            {
                mPlaneRenderData[planeId].visible = false;
            }
        }
#endif
        m3drDataDirty = false;
    }
}

//Render content for one eye
//SvrEyeId: the id of the eye, kLeft for the left eye, kRight for the right eye.
void SurfaceMeshApp::RenderEye(Svr::SvrEyeId eyeId)
{
    GL(glEnable(GL_SCISSOR_TEST));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_CULL_FACE));
    GL(glDepthFunc(GL_LESS));
    GL(glDepthMask(GL_TRUE));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glEnable(GL_BLEND));

    //GL(glPointSize(10.0f));
    //GL(glSet(GL_POINT_SIZE, 10.0f));
    //GL(glPointSizex(10));
    //GL(glPointSize(10.0f));
    //GL(glEnable(GL_PROGRAM_POINT_SIZE));

    mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Bind();
    GL(glViewport(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight));
    GL(glScissor(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight));
    GL(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    sxrBeginEye((sxrWhichEye) eyeId);
    Sxr3drResult result;

#if ENABLE_POINTCLOUD
    mPointCloudShader.Bind();

    mPointCloudShader.SetUniformMat4("projMtx", mProjectionMatrix[eyeId]);
    mPointCloudShader.SetUniformMat4("viewMtx", mViewMatrix[eyeId]);
    mPointCloudShader.SetUniformMat4fv("mdlMtx", 1, (float *)mQvrDataTransformMat.M);

    mPointCloudShader.SetUniformVec4("pointColor", mPointCloudColor);

    glm::vec4 screenSize = glm::vec4(mAppContext.targetEyeWidth, mAppContext.targetEyeHeight, 1.0f / mAppContext.targetEyeWidth, 1.0f / mAppContext.targetEyeHeight);
    mPointCloudShader.SetUniformVec4("screenSize", screenSize);

    glm::vec4 sphereSize = glm::vec4(mPointCloudSize, mPointCloudSize, 0.0f, 0.0f);
    mPointCloudShader.SetUniformVec4("sphereSize", sphereSize);

    mPointCloud.SubmitPoints();

    mPointCloudShader.Unbind();
#endif

    mColorShader.Bind();
#if ENABLE_PLACEMENTIFO
    mColorShader.SetUniformMat4("projection", mProjectionMatrix[eyeId]);
    mColorShader.SetUniformMat4("view", mViewMatrix[eyeId]);
    // Get the ray information in world space before calling GetPlacementInfo()
    glm::fquat poseQuat = glm::fquat(mPoseState.pose.rotation.w, mPoseState.pose.rotation.x,
                                     mPoseState.pose.rotation.y, mPoseState.pose.rotation.z);
    glm::mat4 poseMat = glm::mat4_cast(poseQuat);
    poseMat = glm::translate(poseMat,
                             glm::vec3(mPoseState.pose.position.x, mPoseState.pose.position.y,
                                       mPoseState.pose.position.z));
    glm::mat4 headToWorldMat = glm::affineInverse(poseMat);
    glm::vec3 rayStartPosInWorld = glm::vec3(headToWorldMat * glm::vec4(RAY_START_POSITION, 1.0f));
    glm::vec3 rayDirectionInWorld = glm::vec3(headToWorldMat * glm::vec4(RAY_DIRECTION, 0.0f));
    glm::vec3 rayEndPosInWorld = rayStartPosInWorld + rayDirectionInWorld * RAY_MAX_DISTANCE;

    // Get the placement information
    sxrVector3 sxrRayStartPosInWorld = {rayStartPosInWorld.x, rayStartPosInWorld.y,
                                        rayStartPosInWorld.z};
    sxrVector3 sxrRayDirectionInWorld = {rayDirectionInWorld.x, rayDirectionInWorld.y,
                                         rayDirectionInWorld.z};
    bool surfaceNormalEstimated;
    sxrHeadPose poseLocalToWorld;
    result = sxr3drGetPlacementInfo(&sxrRayStartPosInWorld, &sxrRayDirectionInWorld,
                                    RAY_MAX_DISTANCE, &surfaceNormalEstimated, &poseLocalToWorld);
    glm::mat4 logoModelMat;
    bool gotPlacementInfo = (result == SXR_3DR_SUCCESS && surfaceNormalEstimated);
    if (gotPlacementInfo)
    {
        // Do get the estimated surface normal.
        // Ray be stopped at the point where we get the normal. That is the origin of the "local" coordinate.
        rayEndPosInWorld.x = poseLocalToWorld.position.x;
        rayEndPosInWorld.y = poseLocalToWorld.position.y;
        rayEndPosInWorld.z = poseLocalToWorld.position.z;

        // placmentInfo would be on the local coordinate.
        logoModelMat = glm::mat4_cast(glm::fquat(poseLocalToWorld.rotation.w,
                                                 poseLocalToWorld.rotation.x,
                                                 poseLocalToWorld.rotation.y,
                                                 poseLocalToWorld.rotation.z));
        logoModelMat[3] = glm::vec4(
                glm::vec3(poseLocalToWorld.position.x, poseLocalToWorld.position.y,
                          poseLocalToWorld.position.z), 1.0f);
        logoModelMat = glm::translate(logoModelMat, glm::vec3(0.0f, LOGO_Y_OFFSET, 0.0f));
        logoModelMat = glm::scale(logoModelMat, glm::vec3(LOGO_SIZE, LOGO_SIZE, LOGO_SIZE));
    }

    // Render targeting ray without lighting
    // vector from base center point to top.
    glm::vec3 directionVector = rayEndPosInWorld - rayStartPosInWorld;
    float rayLength = glm::length(directionVector);
    // Update model matrix
    glm::fquat rotation = glm::rotation({0, 0, 1}, glm::normalize(directionVector));
    glm::mat4 modelMat = glm::mat4_cast(rotation);
    modelMat[3] = glm::vec4(rayStartPosInWorld, 1.0f);
    modelMat = glm::scale(modelMat, glm::vec3(1.0f, 1.0f, rayLength));

    mColorShader.SetUniformMat4("model", modelMat);
    mColorShader.SetUniformVec4("objectColor", mTargetingRayColor);

    mTargetingRay.Submit();
#endif

    mColorShader.Unbind();

    mLightShader.Bind();
    mLightShader.SetUniformMat4("view", mViewMatrix[eyeId]);
    mLightShader.SetUniformMat4("projection", mProjectionMatrix[eyeId]);
    mLightShader.SetUniformMat4fv("model", 1, (float *) mQvrDataTransformMat.M);
    mLightShader.SetUniformVec4("objectColor", mMeshColor);
    mLightShader.SetUniformVec3("lightPosition", mLightPosition);
    mLightShader.SetUniformVec3("lightColor", mLightColor);

#if ENABLE_SURFACEMESH
    mSurfaceMesh.Submit();
#endif //ENABLE_SURFACEMESH
#if ENABLE_PLACEMENTIFO
    if (gotPlacementInfo)
    {
        mLightShader.SetUniformMat4("model", logoModelMat);
        mLightShader.SetUniformVec4("objectColor", mLogoColor);
        mLogo->Submit();
    }
#endif

#if ENABLE_COLLISION
    // Render the cubes and the sphere objects
    for (int i = 0; i < NUM_OBJECT; ++i)
    {
        // Cubes
        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(mCubeCenter[i].x,
                                                                       mCubeCenter[i].y,
                                                                       mCubeCenter[i].z));
        mLightShader.SetUniformMat4("model", modelMat);
        // Check collision
        glm::vec4 collisionColor = COLOR_COLLISION_UNKNOWN;
        SxrCollisionResult collisionResult;
        float distanceToSurface;
        sxrVector3 closestPoint;
        result = sxr3drCheckAABBIntersect(&mCubeCenter[i], &mCubeExtents, COLLISION_MAX_CLEARANCE,
                                          &collisionResult, &distanceToSurface, &closestPoint);
        if (result == SXR_3DR_SUCCESS)
        {
            if (collisionResult == SXR_3DR_NO_COLLISION)
            {
                collisionColor = COLOR_COLLISION_NONE;
            }
            else if (collisionResult == SXR_3DR_COLLISION)
            {
                collisionColor = COLOR_COLLISION;
            }
        }
        mLightShader.SetUniformVec4("objectColor", collisionColor);
        mCube.Submit();

        // Spheres
        modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(mSphereCenter[i].x, mSphereCenter[i].y,
                                                             mSphereCenter[i].z));
        mLightShader.SetUniformMat4("model", modelMat);
        // Check collision
        collisionColor = COLOR_COLLISION_UNKNOWN;
        result = sxr3drCheckSphereIntersect(&mSphereCenter[i], mSphereRadius,
                                            COLLISION_MAX_CLEARANCE, &collisionResult,
                                            &distanceToSurface, &closestPoint);
        if (result == SXR_3DR_SUCCESS)
        {
            if (collisionResult == SXR_3DR_NO_COLLISION)
            {
                collisionColor = COLOR_COLLISION_NONE;
            }
            else if (collisionResult == SXR_3DR_COLLISION)
            {
                collisionColor = COLOR_COLLISION;
            }
        }
        mLightShader.SetUniformVec4("objectColor", collisionColor);
        mSphere.Submit();
    }
#endif
    mLightShader.Unbind();

    mTextureShader.Bind();
#if ENABLE_PLANE
    // Render planes with grid texture
    // Plane data is in 3dr coordinate space instead of GL coordinate space.
    mTextureShader.SetUniformMat4fv("model", 1, (float *) mQvrDataTransformMat.M);
    mTextureShader.SetUniformSampler("sampler", mPlaneTexture, GL_TEXTURE_2D, 0);
    mTextureShader.SetUniformMat4("projection", mProjectionMatrix[eyeId]);
    mTextureShader.SetUniformMat4("view", mViewMatrix[eyeId]);
    // Submit visible plane with different color
    for (auto &it : mPlaneRenderData)
    {
        if (mPlaneRenderData[it.first].visible)
        {
            mTextureShader.SetUniformVec4("objectColor", mPlaneRenderData[it.first].color);
            it.second.geometry.Submit();
        }
    }
#endif
    mTextureShader.Unbind();

    sxrEndEye((sxrWhichEye) eyeId);
    mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Unbind();

    // We enabled blending, put it back
    GL(glDisable(GL_BLEND));
}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame after Update(), render for two eyes and submit the frame.
void SurfaceMeshApp::Render()
{
    RenderEye(kLeft);
    RenderEye(kRight);
    SubmitFrame();
}

//Callback function of SvrApplication, called when VR mode stop
//Clean up the model texture
void SurfaceMeshApp::Shutdown()
{
    if (mWorkerThread)
    {
        mShutdown = true;
        mWorkerThread->join();
        mWorkerThread.reset(nullptr);
    }

    mLightShader.Destroy();
    mColorShader.Destroy();
    mTextureShader.Destroy();
    mPointCloudShader.Destroy();

    mSurfaceMesh.Destroy();
    mCube.Destroy();
    mSphere.Destroy();
    mTargetingRay.Destroy();
    mLogo->Destroy();
    delete mLogo;
    for (auto &it : mPlaneRenderData)
    {
        it.second.geometry.Destroy();
    }

    sxr3drShutdown();
    SvrApplication::Shutdown();
}

Svr::SvrGeometry SurfaceMeshApp::CreateCube(const float width)
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

Svr::SvrGeometry SurfaceMeshApp::CreateSphere(const float radius, const uint32_t stacks,
                                              const uint32_t slices)
{
    // Create attributes of the sphere
    unsigned int numElementsPerVert = 6;
    unsigned int numAttribs = 2;
    int stride = (int) (numElementsPerVert * sizeof(float));

    SvrProgramAttribute attribs[2] =
            {
                    {kPosition, 3, GL_FLOAT, false, stride, 0},
                    {kNormal,   3, GL_FLOAT, false, stride, 3 * sizeof(float)},
            };

    // Precompute stack and slice angles (including -90/+90 and -180/180, respectively)
    std::vector<VertexNormal> vertices;
    std::vector<uint32_t> indices;
    const float stackAngle = M_PI / stacks;
    std::vector<float> sinStackAngles(stacks + 1);
    std::vector<float> cosStackAngles(stacks + 1);
    for (uint32_t i = 0; i < stacks + 1; ++i)
    {
        sinStackAngles[i] = std::sin(-(M_PI / 2.0f) + i * stackAngle);
        cosStackAngles[i] = std::cos(-(M_PI / 2.0f) + i * stackAngle);
    }

    const float sliceAngle = (M_PI * 2.0f) / slices;
    std::vector<float> sinSliceAngles(slices + 1);
    std::vector<float> cosSliceAngles(slices + 1);
    for (uint32_t i = 0; i < slices + 1; ++i)
    {
        sinSliceAngles[i] = std::sin(-M_PI + i * sliceAngle);
        cosSliceAngles[i] = std::cos(-M_PI + i * sliceAngle);
    }

    /// Vertex positions + normals
    // The start/end vertices are duplicated (in position and normal)
    {
        // Bottom vertex
        vertices.push_back({{0, -radius, 0},
                            {0, -1,      0}});

        // Middle vertices
        for (uint32_t stack = 1; stack < stacks; ++stack)
        {
            const float sinStack = sinStackAngles[stack];
            const float cosStack = cosStackAngles[stack];
            for (uint32_t slice = 0; slice < slices + 1; ++slice)
            {

                sxrVector3 unitCoords = {cosSliceAngles[slice] * cosStack, sinStack,
                                         sinSliceAngles[slice] * cosStack};
                vertices.push_back(
                        {{unitCoords.x * radius, unitCoords.y * radius, unitCoords.z * radius},
                         unitCoords});
            }
        }

        // Top vertex
        vertices.push_back({{0, radius, 0},
                            {0, 1,      0}});
    }

    /// Indices
    {
        auto addTriangle = [&indices](uint32_t i0, uint32_t i1, uint32_t i2) {
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);
        };

        // Bottom stack
        for (uint32_t slice = 0; slice < slices; ++slice)
        {
            addTriangle(0, 1 + slice, 2 + slice);
        }

        // Middle stacks
        for (uint32_t stack = 0; stack < stacks - 2; ++stack)
        {
            const uint32_t bottomRowStart = 1 + stack * (slices + 1);
            const uint32_t topRowStart = bottomRowStart + (slices + 1);
            for (uint32_t slice = 0; slice < slices; ++slice)
            {
                addTriangle(bottomRowStart + slice, topRowStart + slice, topRowStart + slice + 1);
                addTriangle(bottomRowStart + slice, topRowStart + slice + 1,
                            bottomRowStart + slice + 1);
            }
        }

        // Top stack
        const uint32_t topVert = vertices.size() - 1;
        const uint32_t topRowStart = 1 + (stacks - 2) * (slices + 1);
        for (uint32_t slice = 0; slice < slices; ++slice)
        {
            addTriangle(topRowStart + slice, topVert, topRowStart + slice + 1);
        }
    }

    SvrGeometry outSphere;
    outSphere.Initialize(&attribs[0], numAttribs, (unsigned int *) indices.data(), indices.size(),
                         vertices.data(), vertices.size() * sizeof(VertexNormal), vertices.size());
    return std::move(outSphere);
}

Svr::SvrGeometry SurfaceMeshApp::CreateCone(const float radius, const int circleResolution)
{
    // Create attributes of the cone
    unsigned int numElementsPerVert = 3;
    unsigned int numAttribs = 1;
    int stride = (int) (numElementsPerVert * sizeof(float));

    SvrProgramAttribute attribs[1] =
            {
                    {kPosition, 3, GL_FLOAT, false, stride, 0},
            };


    // Create vertex data of the cone with position data only
    // number of vertices would be resolution adding one for centor of the circle and the vertex of the top of the cone
    int numVerts = circleResolution + 2;
    float thetaStep = M_PI * 2 / circleResolution;
    float *verts = new float[numVerts * numElementsPerVert];
    // Create circle that is on the XY plane and origin is its center.
    int vertIdx = 0;
    for (; vertIdx < circleResolution; ++vertIdx)
    {
        verts[3 * vertIdx] = radius * std::cos(vertIdx * thetaStep);
        verts[3 * vertIdx + 1] = radius * std::sin(vertIdx * thetaStep);
        verts[3 * vertIdx + 2] = 0;
    }
    // Put in the base center vertices
    int baseCenterVertIdx = vertIdx;
    verts[3 * vertIdx] = 0;
    verts[3 * vertIdx + 1] = 0;
    verts[3 * vertIdx + 2] = 0;
    ++vertIdx;
    // Put in the top vertex
    int topVertIdx = vertIdx;
    verts[3 * vertIdx] = 0;
    verts[3 * vertIdx + 1] = 0;
    verts[3 * vertIdx + 2] = 1.0f;


    // Create index data of the cone
    int numTri = 2 * circleResolution; // to form the base circle and the side of the cone surface
    int numIndices = numTri * 3;
    int *indices = new int[numIndices];
    // Base circle
    int indicesIdx = 0;
    for (int triIdx = 0; triIdx < circleResolution - 1; ++triIdx)
    {
        indices[indicesIdx++] = baseCenterVertIdx;
        indices[indicesIdx++] = triIdx + 1;
        indices[indicesIdx++] = triIdx;
    }
    indices[indicesIdx++] = baseCenterVertIdx;
    indices[indicesIdx++] = 0;
    indices[indicesIdx++] = circleResolution - 1;
    // Cone surface
    for (int triIdx = 0; triIdx < circleResolution - 1; ++triIdx)
    {
        indices[indicesIdx++] = topVertIdx;
        indices[indicesIdx++] = triIdx;
        indices[indicesIdx++] = triIdx + 1;
    }
    indices[indicesIdx++] = topVertIdx;
    indices[indicesIdx++] = circleResolution - 1;
    indices[indicesIdx++] = 0;

    SvrGeometry outCone;
    outCone.Initialize(&attribs[0], numAttribs, (unsigned int *) indices, numIndices, verts,
                       numElementsPerVert * numVerts * sizeof(float), numVerts);
    delete[] verts;
    delete[] indices;
    return std::move(outCone);
}

std::vector<glm::vec2> SurfaceMeshApp::PlanarMapping(const std::vector<sxrVector3> &vertices,
                                                     const glm::vec3 &normal)
{
    glm::vec3 texCoordOrigin(vertices[0].x, vertices[0].y, vertices[0].z);
    glm::vec3 texCoordXVector = glm::normalize(
            glm::vec3(vertices[1].x, vertices[1].y, vertices[1].z) - texCoordOrigin);
    glm::vec3 texCoordYVector = glm::normalize(glm::cross(normal, texCoordXVector));

    std::vector<glm::vec2> texCoords(vertices.size(), {0, 0});

    for (int i = 0; i < vertices.size(); ++i)
    {
        // Vector from origin of the texture coordinate to the vertex
        glm::vec3 vertexVector(vertices[i].x - texCoordOrigin.x,
                               vertices[i].y - texCoordOrigin.y,
                               vertices[i].z - texCoordOrigin.z);
        // vector dot the x-axis is its x component
        texCoords[i].x = glm::dot(vertexVector, texCoordXVector);
        // vector dot the y-axis is its y component
        texCoords[i].y = glm::dot(vertexVector, texCoordYVector);
    }
    return std::move(texCoords);
}

void SurfaceMeshApp::WorkInBackground()
{
    while (!mShutdown)
    {
        {
            std::lock_guard<std::mutex> lock(m3drDataMutex);
            uint32_t numVertices = 0, numIndices = 0;
            sxrVector3 *vertices;
            uint32_t *indices;
            Sxr3drResult result = sxr3drGetSurfaceMesh(&numVertices, &vertices, &numIndices,
                                                       &indices);
            if (result == SXR_3DR_SUCCESS)
            {
                if (numVertices > 0 && numIndices > 0)
                {

                    auto vectorFrom2Vertices = [](const sxrVector3 &vertex1,
                                                  const sxrVector3 &vertex2) {
                        sxrVector3 result = {vertex2.x - vertex1.x, vertex2.y - vertex1.y,
                                             vertex2.z - vertex1.z};
                        return result;
                    };

                    auto cross = [](const sxrVector3 &vec1, const sxrVector3 &vec2) {
                        sxrVector3 result = {(vec1.y * vec2.z) - (vec1.z * vec2.y),
                                             (vec1.z * vec2.x) - (vec1.x * vec2.z),
                                             (vec1.x * vec2.y) - (vec1.y * vec2.x)};
                        return result;
                    };

                    auto normalize = [](sxrVector3 &vec) {
                        float length = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
                        if (length != 0)
                        {
                            float inverseLength = 1 / length;
                            vec.x *= inverseLength;
                            vec.y *= inverseLength;
                            vec.z *= inverseLength;
                        }
                    };

                    auto add = [](sxrVector3 &src, const sxrVector3 &other) {
                        src.x += other.x;
                        src.y += other.y;
                        src.z += other.z;
                    };

                    // Calculate triangle normal
                    std::vector<sxrVector3> normals(numVertices, {0.0f, 0.0f, 0.0f});
                    for (uint32_t i = 0; i < numIndices; i += 3)
                    {
                        uint32_t v0 = indices[i + 0];
                        uint32_t v1 = indices[i + 1];
                        uint32_t v2 = indices[i + 2];
                        sxrVector3 u = vectorFrom2Vertices(vertices[v0], vertices[v1]);
                        sxrVector3 v = vectorFrom2Vertices(vertices[v0], vertices[v2]);
                        sxrVector3 normal = cross(u, v);
                        normalize(normal);
                        add(normals[v0], normal);
                        add(normals[v1], normal);
                        add(normals[v2], normal);
                    }

                    for (auto &normal : normals)
                    {
                        normalize(normal);
                    }

                    mMeshData.vertices.resize(numVertices);
                    mMeshData.indices.resize(numIndices);
                    for (uint32_t i = 0; i < numVertices; ++i)
                    {
                        mMeshData.vertices[i].vertex = vertices[i];
                        mMeshData.vertices[i].normal = normals[i];
                    }

                    std::copy(&indices[0], &indices[numIndices], mMeshData.indices.data());

                    m3drDataDirty = true;
                }
                sxr3drReleaseSurfaceMesh();
            }

#if ENABLE_POINTCLOUD
            // Update point cloud
            uint32_t numPoints = 0;
            sxrCloudPoint *cloudPoints;
            SxrResult sxrResult = sxrGetPointCloudData(&numPoints, &cloudPoints);
            if (sxrResult == SXR_ERROR_NONE)
            {
                if (numPoints > 0)
                {
                    LOGI("sxrGetPointCloudData: %d points", numPoints);

                    mCloudPoints.resize(numPoints);
                    mCloudIndices.resize(numPoints);
                    for (uint32_t i = 0; i < numPoints; ++i)
                    {
                        mCloudPoints[i] = cloudPoints[i];
                        mCloudIndices[i] = i;
                    }

                    m3drDataDirty = true;
                }
                sxrReleasePointCloudData();
            }
#endif

#if ENABLE_PLANE
            //Update the plane data
            uint32_t numPlanes = 0;
            result = sxr3drGetPlanes(&numPlanes, NULL);
            if (result == SXR_3DR_ERROR_SIZE_INSUFFICIENT && numPlanes > 0)
            {
                std::vector<uint32_t> planeIds(numPlanes, 0);
                result = sxr3drGetPlanes(&numPlanes, planeIds.data());
                if (result == SXR_3DR_SUCCESS)
                {
                    for (int i = 0; i < numPlanes; ++i)
                    {
                        uint32_t numContourVert = 0;
                        result = sxr3drGetPlaneGeometry(planeIds[i], &numContourVert, NULL);
                        if (result == SXR_3DR_ERROR_SIZE_INSUFFICIENT && numContourVert > 0)
                        {
                            std::vector<sxrVector3> contour(numContourVert, {0, 0, 0});
                            result = sxr3drGetPlaneGeometry(planeIds[i], &numContourVert,
                                                            contour.data());
                            if (result == SXR_3DR_SUCCESS)
                            {
                                // triangulation
                                PlaneData data;

                                // Last vertex and first one is same
                                std::vector<sxrVector3> vertices = std::move(contour);
                                vertices.pop_back();
                                uint32_t numVertices = vertices.size();

                                // triangulation, only work for convex hull
                                for (uint32_t i = 1; i < numVertices - 1; ++i)
                                {
                                    data.indices.push_back(0);
                                    data.indices.push_back(i);
                                    data.indices.push_back(i + 1);
                                }

                                // calculate the normal of the plane by the first three vertices of the plane
                                // and then do the planar mapping
                                glm::vec3 vert0(vertices[data.indices[0]].x,
                                                vertices[data.indices[0]].y,
                                                vertices[data.indices[0]].z);
                                glm::vec3 vert1(vertices[data.indices[1]].x,
                                                vertices[data.indices[1]].y,
                                                vertices[data.indices[1]].z);
                                glm::vec3 vert2(vertices[data.indices[2]].x,
                                                vertices[data.indices[2]].y,
                                                vertices[data.indices[2]].z);
                                glm::vec3 normal = glm::triangleNormal(vert0, vert1, vert2);
                                std::vector<glm::vec2> texCoords = PlanarMapping(vertices,
                                                                                 normal);

                                // store the data to the map
                                data.vertices.resize(numVertices * 5);
                                for (uint32_t i = 0; i < numVertices; ++i)
                                {
                                    data.vertices[5 * i] = vertices[i].x;
                                    data.vertices[5 * i + 1] = vertices[i].y;
                                    data.vertices[5 * i + 2] = vertices[i].z;
                                    data.vertices[5 * i + 3] = texCoords[i].x;
                                    data.vertices[5 * i + 4] = texCoords[i].y;
                                }
                                mPlanesData[planeIds[i]] = std::move(data);
                            }
                        }
                    }
                }
                // check the plane be subsumed or not after initializing all the plane data
                for (auto &it : mPlanesData)
                {
                    uint32 rootPlaneId = 0;
                    result = sxr3drGetRootPlane(it.first, &rootPlaneId);
                    if (result == SXR_3DR_SUCCESS)
                    {
                        it.second.rootPlaneId = rootPlaneId;
                        if (rootPlaneId != it.first)
                            ALOGI("Plane %d subsumed by plane %d", it.first, rootPlaneId);
                    }
                    else
                    {
                        it.second.rootPlaneId = it.first;
                    }
                }
                m3drDataDirty = true;
            }
#endif
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

namespace Svr {
    //Return your own SvrApplication instance
    Svr::SvrApplication *CreateApplication()
    {
        return new SurfaceMeshApp();
    }
}