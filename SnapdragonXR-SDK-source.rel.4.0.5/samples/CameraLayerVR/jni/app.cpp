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
//This app will display camera video feed as the base layer for time warp.
//Besides the camera layer, we will render six torus in each direction(up,down,left,right,
// backward, forward), the torus model is loaded from torus.obj file. Each torus
// will rotate around the X axis, rotations speed is defined by ROTATION_SPEED
//The distance between the viewer and the torus object is defined by
// MODEL_POSITION_RADIUS.
//=============================================================================
#include "app.h"

using namespace Svr;

#define MODEL_POSITION_RADIUS 4.0f
#define BUFFER_SIZE 512
#define ROTATION_SPEED 5.0f

#define ENABLE_POINTCLOUD 1
#define POINTCLOUD_SIZE 0.01f

#define ENABLE_ANCHORS 1

// Extern Cube mesh data
extern float gCubeMeshVerts[];
extern int gNumCubeMeshVerts;
extern unsigned int gCubeMeshIndices[];
extern int gNumCubeMeshIndices;

CameraLayerApp::CameraLayerApp()
{
    mLastRenderTime = 0;
    mModelTexture = 0;
    mOverlayTexture = 0;
    mPointCloudColor = glm::vec4(240.0f, 34.0f, 64.0f, 255.0f) / 255.0f;
}

//Callback function of SvrApplication, it will be called before the VR mode start
// prepare the rendering data here.
void CameraLayerApp::Initialize()
{
    SvrApplication::Initialize();

#if ENABLE_ANCHORS
    SxrResult result = sxrAnchorsInitialize();
    if (result != SXR_ERROR_NONE)
    {
        LOGW("sxrAnchorsInitialize error: %d", result);
    }
#endif

    mAppContext.isCameraLayerEnabled = true;
    InitializeModel();

    sxrAnchorsStartRelocating(mAppContext.externalPath);

#if 0 //ENABLE_ANCHORS
    // Add anchor(s)
    glm::mat4 anchorMatrix = glm::mat4(1.0f); // Identity
    sxrAnchorPose anchorPose;
    anchorPose.rotation[0] = anchorMatrix[0][0];
    anchorPose.rotation[1] = anchorMatrix[0][1];
    anchorPose.rotation[2] = anchorMatrix[0][2];
    anchorPose.rotation[3] = anchorMatrix[1][0];
    anchorPose.rotation[4] = anchorMatrix[1][1];
    anchorPose.rotation[5] = anchorMatrix[1][2];
    anchorPose.rotation[6] = anchorMatrix[2][0];
    anchorPose.rotation[7] = anchorMatrix[2][1];
    anchorPose.rotation[8] = anchorMatrix[2][2];
    anchorPose.location[0] = anchorMatrix[3][0];
    anchorPose.location[1] = anchorMatrix[3][1];
    anchorPose.location[2] = anchorMatrix[3][2];
    uint32_t anchorId;

    result = sxrAnchorsCreate(&anchorPose, &anchorId);
    if (result == SXR_ERROR_NONE)
    {
        LOGI("sxrAnchorsCreate: AnchorId = %d", anchorId);
    }
    else
    {
        LOGE("sxrAnchorsCreate: Error result = %d", result);
    }

    result = sxrAnchorsCreate(&anchorPose, &anchorId);
    if (result == SXR_ERROR_NONE)
    {
        LOGI("sxrAnchorsCreate: AnchorId = %d", anchorId);
    }
    else
    {
        LOGE("sxrAnchorsCreate: Error result = %d", result);
    }
#endif
}

//Initialize  model for rendering: load the model from .obj file, load the shaders,
// load the texture file, prepare the model matrix and model color
void CameraLayerApp::InitializeModel()
{
    //load the model from torus.obj file
    SvrGeometry *pObjGeom;
    int nObjGeom;
    char tmpFilePath[BUFFER_SIZE];
    char tmpFilePath2[BUFFER_SIZE];

    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, "torus.obj");
    SvrGeometry::CreateFromObjFile(tmpFilePath, &pObjGeom, nObjGeom);
    mModel = &pObjGeom[0];

    //sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, "occlude.obj");
    //SvrGeometry::CreateFromObjFile(tmpFilePath, &pObjGeom, nObjGeom);
    //sxrSetOcclusionMesh(&pObjGeom[0]);
    //sxrSetOcclusionMesh(gNumCubeMeshVerts, gNumCubeMeshIndices, gCubeMeshVerts, gCubeMeshIndices);

    //load the vertex shader model_v.glsl and fragment shader model_f.glsl
    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, "model_v.glsl");
    sprintf(tmpFilePath2, "%s/%s", mAppContext.externalPath, "model_f.glsl");
    Svr::InitializeShader(mShader, tmpFilePath, tmpFilePath2, "Vs", "Fs");

    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, "pointcloud_v.glsl");
    sprintf(tmpFilePath2, "%s/%s", mAppContext.externalPath, "pointcloud_f.glsl");
    Svr::InitializeShader(mPointCloudShader, tmpFilePath, tmpFilePath2, "Vs", "Fs");

    //load the texture white.ktx
    const char *pModelTexFile = "white.ktx";
    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, pModelTexFile);
    Svr::LoadTextureCommon(&mModelTexture, tmpFilePath);

    //load overlay texture 
    const char *pOverylayTexFile = "bbb_frame.ktx";
    sprintf(tmpFilePath, "%s/%s", mAppContext.externalPath, pOverylayTexFile);
    Svr::LoadTextureCommon(&mOverlayTexture, tmpFilePath);

#if ENABLE_POINTCLOUD
    std::vector<Svr::SvrProgramAttribute> attrPointCloud = { { Svr::kPosition, 3, GL_FLOAT, false, sizeof(sxrCloudPoint), 0 } };
    mPointCloud.Initialize(attrPointCloud.data(), attrPointCloud.size(), NULL, 0, 0);
    //mPointCloud.Initialize(attrPointCloud.data(), attrPointCloud.size(), NULL, 0, NULL, 0, 0);
    mPointCloudSize = POINTCLOUD_SIZE;
#endif

    //prepare the model matrix and model color
    float colorScale = 0.8f;
    mModelMatrix[0] = glm::translate(glm::mat4(1.0f), glm::vec3(MODEL_POSITION_RADIUS, 0.0f, 0.0f));
    mModelColor[0]  = colorScale * glm::vec3(1.0f, 0.0f, 0.0f);

    mModelMatrix[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, MODEL_POSITION_RADIUS, 0.0f));
    mModelColor[1]  = colorScale * glm::vec3(0.0f, 1.0f, 0.0f);

    mModelMatrix[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, MODEL_POSITION_RADIUS));
    mModelColor[2]  = colorScale * glm::vec3(0.0f, 0.0f, 1.0f);

    mModelMatrix[3] = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(-MODEL_POSITION_RADIUS, 0.0f, 0.0f));
    mModelColor[3]  = colorScale * glm::vec3(0.0f, 1.0f, 1.0f);

    mModelMatrix[4] = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(0.0f, -MODEL_POSITION_RADIUS, 0.0f));
    mModelColor[4]  = colorScale * glm::vec3(1.0f, 0.0f, 1.0f);

    mModelMatrix[5] = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(0.0f, 0.0f, -MODEL_POSITION_RADIUS));
    mModelColor[5]  = colorScale * glm::vec3(1.0f, 1.0f, 0.0f);
}

//Submit the rendered frame
void CameraLayerApp::SubmitFrame()
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
#if 0
    // Render image background instead of camera
    frameParams.renderLayers[numLayers].imageType = kTypeTexture;
    frameParams.renderLayers[numLayers].imageHandle = mOverlayTexture; 
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskLeft;
#endif
    numLayers++;

    /*
     * Right Eye (bottom layer)
     */

    // XRSDK must be compiled with -DENABLE_CAMERA for camera to work
    frameParams.renderLayers[numLayers].imageType = kTypeCamera;  
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskRight;
    frameParams.renderLayers[numLayers].layerFlags = kLayerFlagOpaque;
#if 0
    // Render image background instead of camera
    frameParams.renderLayers[numLayers].imageType = kTypeTexture;
    frameParams.renderLayers[numLayers].imageHandle = mOverlayTexture; 
    L_CreateLayout(0.0f, 0.0f, 1.0f, 1.0f, &frameParams.renderLayers[numLayers].imageCoords);
    frameParams.renderLayers[numLayers].eyeMask = kEyeMaskRight;
#endif
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
void CameraLayerApp::Update()
{
    //base class Update
    SvrApplication::Update();

    sxrDeviceInfo di = sxrGetDeviceInfo();

    //update view matrix
    float predictedTimeMs = sxrGetPredictedDisplayTime();
    mPoseState = sxrGetPredictedHeadPose(predictedTimeMs);

	// add in changes for view matrix view customization parameters
	glm::vec3 LeftEyePos(di.leftEyeFrustum.position.x, di.leftEyeFrustum.position.y, di.leftEyeFrustum.position.z);
	glm::fquat LeftEyeRot(di.leftEyeFrustum.rotation.w, di.leftEyeFrustum.rotation.x, di.leftEyeFrustum.rotation.y, di.leftEyeFrustum.rotation.z);  // glm quat is (w)(xyz)
	glm::vec3 RightEyePos(di.rightEyeFrustum.position.x, di.rightEyeFrustum.position.y, di.rightEyeFrustum.position.z);
	glm::fquat RightEyeRot(di.rightEyeFrustum.rotation.w, di.rightEyeFrustum.rotation.x, di.rightEyeFrustum.rotation.y, di.rightEyeFrustum.rotation.z);  // glm quat is (w)(xyz)

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
    mQvrToSxrMat = glm::make_mat4((float*)mQvrDataTransformMat.M);
    mSxrToQvrMat = glm::affineInverse(mQvrToSxrMat);

    // Moved to Render
    //if ((mAppContext.frameCount % 100) == 0)
    //{
    //    sxrSetOcclusionMesh(gNumCubeMeshVerts, gNumCubeMeshIndices, gCubeMeshVerts, gCubeMeshIndices);
    //}
#if ENABLE_POINTCLOUD
    if ((mAppContext.frameCount % 100) == 0)
    {
        // Update point cloud
        uint32_t numPoints = 0;
        sxrCloudPoint *cloudPoints;
        SxrResult sxrResult = sxrGetPointCloudData(&numPoints, &cloudPoints);
        if (sxrResult == SXR_ERROR_NONE)
        {
            LOGI("sxrGetPointCloudData: %d points", numPoints);

            if (numPoints > 0)
            {
                //mCloudPoints.resize(numPoints);
                mCloudIndices.resize(numPoints);
                for (uint32_t i = 0; i < numPoints; ++i)
                {
                    //mCloudPoints[i] = cloudPoints[i];
                    mCloudIndices[i] = i;
                }

                mPointCloud.Update(cloudPoints, numPoints * sizeof(sxrCloudPoint), numPoints/*,
                    mCloudIndices.data(), mCloudIndices.size()*/);
            }
            sxrReleasePointCloudData();
        }
    }
#endif

#if ENABLE_ANCHORS
    // Get the ray information in world 
    glm::fquat poseQuat = glm::fquat(mPoseState.pose.rotation.w, mPoseState.pose.rotation.x, mPoseState.pose.rotation.y, mPoseState.pose.rotation.z);
    glm::mat4 poseMat = glm::mat4_cast(poseQuat);
    poseMat = glm::translate(poseMat, glm::vec3(mPoseState.pose.position.x, mPoseState.pose.position.y, mPoseState.pose.position.z));
    glm::mat4 headToWorldMat = glm::affineInverse(poseMat);
    glm::vec3 rayStartPosInWorld = glm::vec3(headToWorldMat * glm::vec4(0.0f, 0.2f, 0.0f, 0.0f));
    glm::vec3 rayDirectionInWorld = glm::vec3(headToWorldMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    glm::vec3 rayEndPosInWorld = rayStartPosInWorld + rayDirectionInWorld * MODEL_POSITION_RADIUS;

    //glm::mat4 anchorMatrix = glm::translate(glm::mat4(1.0f), rayEndPosInWorld);
    //anchorMatrix = glm::rotate(anchorMatrix, glm::radians(rotateAmount), glm::vec3(1.0f, 0.0f, 0.0f));

    mModelMatrix[0][3] = glm::vec4(rayEndPosInWorld, 1.0f);

    //if (mInput.IsKeyDown(kSvrKey_ENTER))
    //{
    //    LOGI("Place an anchor");
    //}
    bool placeAnchor = false;
    unsigned int TimeNow = GetTimeMS();
    if ((mInput.IsPointerDown(0) || mInput.IsKeyDown(kSvrKey_ENTER))
        && (TimeNow - mLastToggleTime > 1000))
    {
        LOGI("Place Anchor: ");
        placeAnchor = true;

        mLastToggleTime = TimeNow;
    }

    //if ((mAppContext.frameCount % 100) == 0)
    if (placeAnchor)
    {
        // Remove anchor(s)
        if (mAnchorId.size() >= MODEL_SIZE-1)
        {
            sxrAnchorUuid anchorId = mAnchorId[0];
            SxrResult result = sxrAnchorsDestroy(anchorId);
            if (result == SXR_ERROR_NONE)
            {
                LOGI("sxrAnchorsDestroy: AnchorId: %s", sxrAnchorsToString(anchorId));
            }
            else
            {
                LOGE("sxrAnchorsDestroy: AnchorId = %s, Error result = %d", sxrAnchorsToString(anchorId), result);
            }
            mAnchorId.erase(mAnchorId.begin());
            LOGI("sxrAnchorsDestroy: AnchorId Count = %lu", mAnchorId.size());

        }

        // Add anchor(s)
        glm::mat4 anchorMatrix = mSxrToQvrMat * mModelMatrix[0]; // Convert from sxr to qvr

        glm::fquat anchorQuat = glm::quat_cast(anchorMatrix);
        glm::vec3 anchorPos = glm::vec3(anchorMatrix[3][0], anchorMatrix[3][1], anchorMatrix[3][2]);

        sxrAnchorPose anchorPose;
        //anchorPose.orientation = mPoseState.pose.rotation;
        anchorPose.orientation.x = anchorQuat.x;
        anchorPose.orientation.y = anchorQuat.y;
        anchorPose.orientation.z = anchorQuat.z;
        anchorPose.orientation.w = anchorQuat.w;
        //anchorPose.position = mPoseState.pose.position;
        anchorPose.position.x = anchorPos.x;
        anchorPose.position.y = anchorPos.y;
        anchorPose.position.z = anchorPos.z;
        anchorPose.poseQuality = 1.0f;

        sxrAnchorUuid anchorId;
        SxrResult result = sxrAnchorsCreate(&anchorPose, &anchorId);
        if (result == SXR_ERROR_NONE)
        {
            LOGI("sxrAnchorsCreate: AnchorId = %s, Position: [%f %f %f], Orientation: [%f %f %f %f]\n",
                sxrAnchorsToString(anchorId),
                anchorPose.position.x, anchorPose.position.y, anchorPose.position.z,
                anchorPose.orientation.x, anchorPose.orientation.y, anchorPose.orientation.z, anchorPose.orientation.w);

            //if (anchorId > 0)
            {
                mAnchorId.push_back(anchorId);
            }
            //else
            //{
            //    LOGW("sxrAnchorsCreate: AnchorId = %d", anchorId);
            //}
        }
        else
        {
            LOGE("sxrAnchorsCreate: Error result = %d", result);
        }
        LOGI("sxrAnchorsCreate: AnchorId Count = %lu", mAnchorId.size());

    }

    if ((mAppContext.frameCount % 100) == 0)
    {
        uint32_t numAnchors = 0;
        sxrAnchorInfo *anchors;
        SxrResult sxrResult = sxrAnchorsGetData(&numAnchors, &anchors);
        if (sxrResult == SXR_ERROR_NONE)
        {
            LOGI("sxrAnchorsGetData: %d anchors", numAnchors);

            if (numAnchors > 0)
            {
                mAnchorMatrix.resize(numAnchors);

                for (uint32_t i = 0; i < numAnchors; ++i)
                {
                    sxrAnchorUuid anchorId = anchors[i].id;
                    sxrAnchorPose anchorPose = anchors[i].pose;

                    anchorPose = sxrAnchorsGetPose(anchorPose, mQvrDataTransformMat);

                    LOGI("sxrAnchorsShow: AnchorId = %s, Position: [%f %f %f], Orientation: [%f %f %f %f]\n",
                        sxrAnchorsToString(anchorId),
                        anchorPose.position.x, anchorPose.position.y, anchorPose.position.z,
                        anchorPose.orientation.x, anchorPose.orientation.y, anchorPose.orientation.z, anchorPose.orientation.w);

                    glm::fquat anchorRot = glm::fquat(anchorPose.orientation.w, anchorPose.orientation.x, anchorPose.orientation.y, anchorPose.orientation.z);
                    glm::vec4 anchorPos = glm::vec4(anchorPose.position.x, anchorPose.position.y, anchorPose.position.z, 1.0f);
                    mAnchorMatrix[i] = glm::mat4(anchorRot);
                    mAnchorMatrix[i][3] = anchorPos;
                }
            }
            sxrAnchorsReleaseData();
        }
    }

#endif
}

//Render content for one eye
//SvrEyeId: the id of the eye, kLeft for the left eye, kRight for the right eye.
void CameraLayerApp::RenderEye(Svr::SvrEyeId eyeId)
{
    unsigned int timeNow = GetTimeMS();
    float rotateAmount = GetRotationAmount(timeNow, ROTATION_SPEED);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Bind();
    glViewport(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight);
    glScissor(0, 0, mAppContext.targetEyeWidth, mAppContext.targetEyeHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f); // Transparent
    //glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Opaque
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sxrBeginEye((sxrWhichEye) eyeId);

    //sxrOccludeEye((sxrWhichEye)eyeId, glm::value_ptr(mProjectionMatrix[eyeId]), glm::value_ptr(mViewMatrix[eyeId])); // TEST

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

    mPointCloud.Submit();
    //mPointCloud.SubmitPoints();

    mPointCloudShader.Unbind();
#endif

    mShader.Bind();
    mShader.SetUniformMat4("projectionMatrix", mProjectionMatrix[eyeId]);
    mShader.SetUniformMat4("viewMatrix", mViewMatrix[eyeId]);
    mShader.SetUniformSampler("srcTex", mModelTexture, GL_TEXTURE_2D, 0);
    glm::vec3 eyePos = glm::vec3(-mViewMatrix[eyeId][3][0], -mViewMatrix[eyeId][3][1], -mViewMatrix[eyeId][3][2]);
    mShader.SetUniformVec3("eyePos", eyePos);

    int j = 0;
#if ENABLE_ANCHORS
    {
        mModelMatrix[j] = glm::rotate(mModelMatrix[j], glm::radians(rotateAmount), glm::vec3(1.0f, 0.0f, 0.0f));
        mShader.SetUniformMat4("modelMatrix", mModelMatrix[j]);
        mShader.SetUniformVec3("modelColor", mModelColor[j]);
        mModel->Submit();
    } j++;
    for ( ; j <= mAnchorMatrix.size() && j < MODEL_SIZE; j++) {
        mModelMatrix[j] = mAnchorMatrix[j-1]; // * QvrTransform
        mShader.SetUniformMat4("modelMatrix", mModelMatrix[j]);
        mShader.SetUniformVec3("modelColor", mModelColor[j]);
        mModel->Submit();
    }
#endif
    for ( ; j < MODEL_SIZE; j++) {
        mModelMatrix[j] = glm::rotate(mModelMatrix[j], glm::radians(rotateAmount), glm::vec3(1.0f, 0.0f, 0.0f));
        mShader.SetUniformMat4("modelMatrix", mModelMatrix[j]);
        mShader.SetUniformVec3("modelColor", mModelColor[j]);
        mModel->Submit();
    }

    mShader.Unbind();

    sxrEndEye((sxrWhichEye) eyeId);

	mAppContext.eyeBuffers[mAppContext.eyeBufferIndex].eyeTarget[eyeId].Unbind();

    mLastRenderTime = timeNow;
}

//Callback function of SvrApplication, after the VR mode start,it will be called in a loop
// once each frame after Update(), render for two eyes and submit the frame.
void CameraLayerApp::Render()
{
    // Todo: Set only as needed
    //if ((mAppContext.frameCount % 100) == 0)
    //{
    //    sxrSetOcclusionMesh(gNumCubeMeshVerts, gNumCubeMeshIndices, gCubeMeshVerts, gCubeMeshIndices);
    //}

    RenderEye(kLeft);
    RenderEye(kRight);
    SubmitFrame();
}

//Get rotation amount for rotationRate
// rotationRate: the rotation rate, for example rotationRate = 5 means 5 degree every second
float CameraLayerApp::GetRotationAmount(unsigned int timeNow, float rotationRate)
{
    //unsigned int timeNow = GetTimeMS();
    //if (mLastRenderTime == 0) {
    //    mLastRenderTime = timeNow;
    //}

    float elapsedTime = (float) (timeNow - mLastRenderTime) / 1000.0f;
    float rotateAmount = elapsedTime * (360.0f / rotationRate);

    //mLastRenderTime = timeNow;
    return rotateAmount;
}

//Callback function of SvrApplication, called when VR mode stop
//Clean up the model texture
void CameraLayerApp::Shutdown()
{
    for (int i = 0; i < mAnchorId.size(); i++)
    {
        sxrAnchorUuid anchorId = mAnchorId.at(i);
        LOGI("Shutdown: Save anchor %s", sxrAnchorsToString(anchorId));
        SxrResult result = sxrAnchorsSave(anchorId, mAppContext.externalPath);
        if (result != SXR_ERROR_NONE)
        {
            LOGE("sxrAnchorsSave Error: Anchor %s result = %d", sxrAnchorsToString(anchorId), result);
        }
    }

    SvrApplication::Shutdown();

#if ENABLE_ANCHORS
    SxrResult result = sxrAnchorsShutdown();
    if (result != SXR_ERROR_NONE)
    {
        LOGW("sxrAnchorsShutdown error: %d", result);
    }
#endif

    if (mModelTexture != 0)
    {
        GL(glDeleteTextures(1, &mModelTexture));
        mModelTexture = 0;
    }
    if (mOverlayTexture != 0)
    {
        GL(glDeleteTextures(1, &mOverlayTexture));
        mOverlayTexture = 0;
    }
    mShader.Destroy();
    mModel->Destroy();
    sxrSetOcclusionMesh(0, 0, NULL, NULL);
}

namespace Svr {
    //Return your own SvrApplication instance
    Svr::SvrApplication *CreateApplication()
    {
        return new CameraLayerApp();
    }
}
