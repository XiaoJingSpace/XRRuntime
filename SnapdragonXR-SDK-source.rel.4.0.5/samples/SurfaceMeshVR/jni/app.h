//=============================================================================
// FILE: app.h
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include "sxrApi3dr.h"
#include "sxrApi.h"
#include "svrApplication.h"
#include "svrGeometry.h"
#include "svrShader.h"
#include "svrUtil.h"
#include "utils.h"

class SurfaceMeshApp : public Svr::SvrApplication {

public:
    SurfaceMeshApp();

    void Initialize() override;

    void LoadConfiguration() override;

    void Update() override;

    void Render() override;

    void Shutdown() override;

private:
    void RenderEye(Svr::SvrEyeId);

    void SubmitFrame();

    Svr::SvrGeometry CreateCube(const float width);

    Svr::SvrGeometry CreateSphere(const float radius, const uint32_t stacks, const uint32_t slices);

    Svr::SvrGeometry CreateCone(const float radius, const int circleResolution);

    std::vector<glm::vec2> PlanarMapping(const std::vector<sxrVector3> &vertices,
                                         const glm::vec3 &normal);

    void WorkInBackground();

    static constexpr int NUM_OBJECT = 20;

    //SvrGeometry
    Svr::SvrGeometry mSurfaceMesh;
    Svr::SvrGeometry mCube;
    Svr::SvrGeometry mSphere;
    Svr::SvrGeometry mTargetingRay;
    Svr::SvrGeometry mPointCloud;
    Svr::SvrGeometry *mLogo;

    //position of each object
    sxrVector3 mCubeCenter[NUM_OBJECT];
    sxrVector3 mSphereCenter[NUM_OBJECT];
    sxrVector3 mCubeExtents;
    float mSphereRadius;
    float mPointCloudSize;

    //color for each object
    glm::vec4 mTargetingRayColor;
    glm::vec4 mPointCloudColor;
    glm::vec4 mLogoColor;

    //Use SvrShader to load vertex and fragment shaders from file.
    Svr::SvrShader mLightShader;
    Svr::SvrShader mColorShader;
    Svr::SvrShader mTextureShader;
    Svr::SvrShader mPointCloudShader;

    //Use a texture for coloring
    GLuint mPlaneTexture;

    //pose state
    sxrHeadPoseState mPoseState;

    //view matrix
    glm::mat4 mViewMatrix[kNumEyes];

    //projection matrix
    glm::mat4 mProjectionMatrix[kNumEyes];

    // model matrix for 3dr objects
    sxrMatrix mQvrDataTransformMat;

    //mesh color
    glm::vec4 mMeshColor;

    //light position
    glm::vec3 mLightPosition;
    //light color
    glm::vec3 mLightColor;

    std::vector<sxrCloudPoint> mCloudPoints;
    std::vector<uint32_t> mCloudIndices;

    struct VertexNormal {
        sxrVector3 vertex;
        sxrVector3 normal;
    };

    struct MeshData {
        std::vector<VertexNormal> vertices;
        std::vector<uint32_t> indices;

        MeshData()
        {
        }
    };

    MeshData mMeshData;

    struct PlaneRenderData {
        Svr::SvrGeometry geometry;
        glm::vec4 color;
        bool visible;

        PlaneRenderData() : visible(true),
                            color(glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)), 1.0f)
        {
        }
    };

    struct PlaneData {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        uint32 rootPlaneId;

        PlaneData() :
                rootPlaneId(0)
        {
        }
    };

    std::unordered_map<uint32_t, PlaneData> mPlanesData;
    std::unordered_map<uint32_t, PlaneRenderData> mPlaneRenderData;

    //worker thread and data mutex
    std::unique_ptr<std::thread> mWorkerThread;
    std::mutex m3drDataMutex;
    std::atomic_bool m3drDataDirty;
    bool mShutdown;
};