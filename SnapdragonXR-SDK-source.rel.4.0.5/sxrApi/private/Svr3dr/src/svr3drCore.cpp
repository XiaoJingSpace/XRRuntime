#include "svr3drCore.h"

#include <cassert>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "svrApiCore.h"
#include "svrUtil.h"

Svr3drCore::Svr3drCore() :
        m3drHandle(nullptr),
        mSurfaceMesh(nullptr)
{
}

Svr3drCore::~Svr3drCore()
{
    if (m3drHandle)
    {
        LOGW("3dr module was not shutdown");
    }
}

Sxr3drResult Svr3drCore::Initialize(qvrservice_client_helper_t *helper)
{
    if (m3drHandle)
    {
        return SXR_3DR_SUCCESS;
    }

    if (!helper)
    {
        return SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    m3drHandle = QVRServiceClient_GetClassHandle(helper, CLASS_ID_3DR_BETA, nullptr);
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_UNSUPPORTED;
    }

    return SXR_3DR_SUCCESS;
}

Sxr3drResult Svr3drCore::Shutdown(qvrservice_client_helper_t *helper)
{
    Sxr3drResult result = SXR_3DR_SUCCESS;
    if (helper)
    {
        if (m3drHandle)
        {
            QVRServiceClient_ReleaseClassHandle(helper, m3drHandle);
            m3drHandle = nullptr;
        }
        else
        {
            result = SXR_3DR_ERROR_NOT_INITIALIZED;
        }
    }
    else
    {
        result = SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    return result;
}

Sxr3drResult Svr3drCore::GetSurfaceMesh(uint32_t *numVertices, sxrVector3 **vertices,
                                        uint32_t *numIndices, uint32_t **indices)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!numVertices || !numIndices || !vertices || !indices)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    assert(sizeof(sxrVector3) == sizeof(XrVector3fQTI));

    if (mSurfaceMesh)
    {
        // Something wrong here, maybe the user didn't call ReleaseSurfaceMesh.
        LOGW("GetSurfaceMesh previous mesh is not released");
        ReleaseSurfaceMesh();
    }

    Sxr3drResult result = SXR_3DR_SUCCESS;

    int qvrResult = QVR3DR_GetSurfaceMesh(m3drHandle, &mSurfaceMesh);
    if (qvrResult == QVR_SUCCESS)
    {
        *numVertices = mSurfaceMesh->numVertices;
        *numIndices = mSurfaceMesh->numIndices;
        *vertices = (sxrVector3 *) mSurfaceMesh->vertices;
        *indices = mSurfaceMesh->indices;
    }
    else
    {
        result = (qvrResult == QVR_RESULT_PENDING) ? SXR_3DR_ERROR_PENDING
                                                   : SXR_3DR_ERROR_INVALID_OPERATION;
        *numVertices = 0;
        *numIndices = 0;
        *vertices = nullptr;
        *indices = nullptr;
    }

    return result;
}

Sxr3drResult Svr3drCore::ReleaseSurfaceMesh()
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!mSurfaceMesh)
    {
        return SXR_3DR_ERROR_INVALID_OPERATION;
    }

    Sxr3drResult result = SXR_3DR_SUCCESS;
    int qvrResult = QVR3DR_ReleaseSurfaceMesh(m3drHandle, mSurfaceMesh);
    if (qvrResult != QVR_SUCCESS)
    {
        LOGW("ReleaseSurfaceMesh error %d", qvrResult);
        result = SXR_3DR_ERROR_INVALID_OPERATION;
    }
    mSurfaceMesh = nullptr;

    return result;
}

Sxr3drResult Svr3drCore::GetPlanes(uint32_t *numPlanes, uint32_t *planeIds)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!numPlanes)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    Sxr3drResult result = SXR_3DR_SUCCESS;

    // totalBytes is the total size in bytes required to hold the data
    uint32_t totalBytes = planeIds ? *numPlanes * sizeof(*numPlanes) : 0;
    int qvrResult = QVR3DR_GetPlanes(m3drHandle, &totalBytes, planeIds);
    if (qvrResult == QVR_SUCCESS)
    {
        if (!planeIds)
        {
            *numPlanes = totalBytes / sizeof(*numPlanes);
            result = SXR_3DR_ERROR_SIZE_INSUFFICIENT;
        }
    }
    else if (qvrResult == QVR_RESULT_PENDING)
    {
        result = SXR_3DR_ERROR_PENDING;
    }
    else
    {
        result = SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    return result;
}

Sxr3drResult Svr3drCore::GetRootPlane(uint32_t planeId, uint32_t *rootPlaneId)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    return QvrErrorToSxrError(QVR3DR_GetRootPlane(m3drHandle, planeId, rootPlaneId));
}

Sxr3drResult Svr3drCore::GetPlaneGeometry(uint32_t planeId, uint32_t *numVertices,
                                          sxrVector3 *vertices)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    Sxr3drResult result = SXR_3DR_SUCCESS;

    // totalBytes is the total size in bytes required to hold the data
    uint32_t totalBytes = numVertices ? *numVertices * (3 * sizeof(float)) : 0;
    int qvrResult = QVR3DR_GetPlaneGeometry(m3drHandle, planeId, &totalBytes, (float *) vertices);
    if (qvrResult == QVR_SUCCESS)
    {
        if (!vertices)
        {
            *numVertices = totalBytes / (3 * sizeof(float));
            result = SXR_3DR_ERROR_SIZE_INSUFFICIENT;
        }
    }
    else if (qvrResult == QVR_RESULT_PENDING)
    {
        result = SXR_3DR_ERROR_PENDING;
    }
    else
    {
        result = SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    return result;
}

Sxr3drResult Svr3drCore::GetPlaneOrientation(uint32_t planeId, SxrPlaneOrientation *orientation)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!orientation)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    QVR3DR_PLANE_ORIENTATION qvrOrientation;
    int qvrResult = QVR3DR_GetPlaneOrientation(m3drHandle, planeId, &qvrOrientation);
    switch (qvrOrientation)
    {
        case QVR3DR_PLANE_HORIZONTAL:
            *orientation = SXR_3DR_PLANE_HORIZONTAL;
            break;

        case QVR3DR_PLANE_VERTICAL:
            *orientation = SXR_3DR_PLANE_VERTICAL;
            break;

        default:
            *orientation = SXR_3DR_PLANE_SLANT;
            break;
    }

    return QvrErrorToSxrError(qvrResult);
}

Sxr3drResult Svr3drCore::CheckPointIntersect(const sxrVector3 *point, float maxClearance,
                                             SxrCollisionResult *collision,
                                             float *distanceToSurface,
                                             sxrVector3 *closestPointOnSurface)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!point || !collision || !distanceToSurface || !closestPointOnSurface)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    GetTransformMatrixIfNeeded();

    // Input should be in qvr coordinate space
    XrVector3fQTI pointInQvrSpace = Rotate(*point, mTransform.glToQvrMatrix);

    qvr3dr_collision_warning_result_t collisionResult;
    int qvrResult = QVR3DR_CheckPointIntersect(m3drHandle, &pointInQvrSpace, maxClearance,
                                               &collisionResult);
    FillCollisionResult((qvrResult == QVR_SUCCESS) ? &collisionResult : nullptr, collision,
                        distanceToSurface, closestPointOnSurface);

    return QvrErrorToSxrError(qvrResult);
}

Sxr3drResult Svr3drCore::CheckAABBIntersect(const sxrVector3 *center, const sxrVector3 *extents,
                                            float maxClearance, SxrCollisionResult *collision,
                                            float *distanceToSurface,
                                            sxrVector3 *closestPointOnSurface)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!center || !extents || !collision || !distanceToSurface || !closestPointOnSurface)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    GetTransformMatrixIfNeeded();

    // Input should be in qvr coordinate space
    XrVector3fQTI centerInQvrSpace = Rotate(*center, mTransform.glToQvrMatrix);
    XrVector3fQTI extentsInQvr = Rotate(*extents, mTransform.glToQvrMatrix);
    extentsInQvr = {abs(extentsInQvr.x), abs(extentsInQvr.y), abs(extentsInQvr.z)};

    qvr3dr_collision_warning_result_t collisionResult;
    int qvrResult = QVR3DR_CheckAABBIntersect(m3drHandle, &centerInQvrSpace, &extentsInQvr,
                                              maxClearance, &collisionResult);
    FillCollisionResult((qvrResult == QVR_SUCCESS) ? &collisionResult : nullptr, collision,
                        distanceToSurface, closestPointOnSurface);

    return QvrErrorToSxrError(qvrResult);
}

Sxr3drResult Svr3drCore::CheckSphereIntersect(const sxrVector3 *center, float radius,
                                              float maxClearance, SxrCollisionResult *collision,
                                              float *distanceToSurface,
                                              sxrVector3 *closestPointOnSurface)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!center || !collision || !distanceToSurface || !closestPointOnSurface)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    GetTransformMatrixIfNeeded();

    // Input should be in qvr coordinate space
    XrVector3fQTI centerInQvrSpace = Rotate(*center, mTransform.glToQvrMatrix);

    qvr3dr_collision_warning_result_t collisionResult;
    int qvrResult = QVR3DR_CheckSphereIntersect(m3drHandle, &centerInQvrSpace, radius, maxClearance,
                                                &collisionResult);
    FillCollisionResult((qvrResult == QVR_SUCCESS) ? &collisionResult : nullptr, collision,
                        distanceToSurface, closestPointOnSurface);

    return QvrErrorToSxrError(qvrResult);
}

Sxr3drResult Svr3drCore::GetPlacementInfo(const sxrVector3 *rayStartPosition,
                                          const sxrVector3 *rayDirection, float maxDistance,
                                          bool *surfaceNormalEstimated,
                                          sxrHeadPose *poseLocalToWorld)
{
    if (!m3drHandle)
    {
        return SXR_3DR_ERROR_NOT_INITIALIZED;
    }

    if (!rayStartPosition || !rayDirection || !surfaceNormalEstimated || !poseLocalToWorld)
    {
        return SXR_3DR_ERROR_INVALID_PARAMETER;
    }

    GetTransformMatrixIfNeeded();

    // Input should be in qvr coordinate space
    XrVector3fQTI rayStartPositionInQvrSpace = Rotate(*rayStartPosition, mTransform.glToQvrMatrix);
    XrVector3fQTI rayDirectionInQvrSpace = Rotate(*rayDirection, mTransform.glToQvrMatrix);

    qvr3dr_placement_info_t placementInfo;
    int qvrResult = QVR3DR_GetPlacementInfo(m3drHandle, &rayStartPositionInQvrSpace,
                                            &rayDirectionInQvrSpace, &placementInfo, maxDistance);
    if (qvrResult == QVR_SUCCESS)
    {
        *surfaceNormalEstimated = placementInfo.surfaceNormalEstimated;

        // Output should be in OpenGL coordinate space
        glm::mat4 qvrPose = glm::mat4_cast(glm::fquat(placementInfo.poseLocalToWorld.orientation.w,
                                                      placementInfo.poseLocalToWorld.orientation.x,
                                                      placementInfo.poseLocalToWorld.orientation.y,
                                                      placementInfo.poseLocalToWorld.orientation.z));
        qvrPose[3] = glm::vec4(glm::vec3(placementInfo.poseLocalToWorld.position.x,
                                         placementInfo.poseLocalToWorld.position.y,
                                         placementInfo.poseLocalToWorld.position.z), 1.0f);
        glm::mat4 glPose = mTransform.qvrToGlMatrix * qvrPose;

        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(glPose, scale, rotation, translation, skew, perspective);
        rotation = glm::conjugate(rotation);
        poseLocalToWorld->position.x = translation.x;
        poseLocalToWorld->position.y = translation.y;
        poseLocalToWorld->position.z = translation.z;
        poseLocalToWorld->rotation.w = rotation.w;
        poseLocalToWorld->rotation.x = rotation.x;
        poseLocalToWorld->rotation.y = rotation.y;
        poseLocalToWorld->rotation.z = rotation.z;
    }
    else
    {
        *surfaceNormalEstimated = false;
        memset(poseLocalToWorld, 0, sizeof(*poseLocalToWorld));
    }
    return QvrErrorToSxrError(qvrResult);
}

Sxr3drResult Svr3drCore::QvrErrorToSxrError(int error)
{
    Sxr3drResult res;
    switch (error)
    {
        case QVR_SUCCESS:
            res = SXR_3DR_SUCCESS;
            break;

        case QVR_RESULT_PENDING:
            res = SXR_3DR_ERROR_PENDING;
            break;

        case QVR_INVALID_PARAM:
            res = SXR_3DR_ERROR_INVALID_PARAMETER;
            break;

        default:
            res = SXR_3DR_ERROR_INVALID_OPERATION;
    }

    return res;
}

SxrCollisionResult Svr3drCore::QvrCollisionToSxrCollision(QVR3DR_COLLISION_RESULT collision)
{
    SxrCollisionResult result;
    switch (collision)
    {
        case QVR3DR_COLLISION:
            result = SXR_3DR_COLLISION;
            break;

        case QVR3DR_NO_COLLISION:
            result = SXR_3DR_NO_COLLISION;
            break;

        default:
            result = SXR_3DR_COLLISION_UNKNOWN;
            break;
    }

    return result;
}

void Svr3drCore::GetTransformMatrixIfNeeded()
{
    if (mTransform.ready)
    {
        return;
    }

    mTransform.qvrToGlMatrix = Svr::gAppContext->modeContext->qvrTransformMat;
    mTransform.glToQvrMatrix = glm::affineInverse(mTransform.qvrToGlMatrix);
    mTransform.ready = true;
}

sxrVector3 Svr3drCore::Rotate(const XrVector3fQTI &src, const glm::mat4 &rotationMatrix)
{
    glm::vec3 out = glm::vec3(rotationMatrix * glm::vec4(src.x, src.y, src.z, 1.0f));
    return {out.x, out.y, out.z};
}

XrVector3fQTI Svr3drCore::Rotate(const sxrVector3 &src, const glm::mat4 &rotationMatrix)
{
    glm::vec3 out = glm::vec3(rotationMatrix * glm::vec4(src.x, src.y, src.z, 1.0f));
    return {out.x, out.y, out.z};
}

void Svr3drCore::FillCollisionResult(const qvr3dr_collision_warning_result_t *result,
                                     SxrCollisionResult *collision, float *distanceToSurface,
                                     sxrVector3 *closestPointOnSurface)
{
    if (result)
    {
        *collision = QvrCollisionToSxrCollision(result->collision);
        *distanceToSurface = result->distanceToSurface;
        // Output should be in OpenGL coordinate space
        sxrVector3 closestPointOnSurfaceInGlSpace = Rotate(result->closestPointOnSurface,
                                                           mTransform.qvrToGlMatrix);
        memcpy(closestPointOnSurface, &closestPointOnSurfaceInGlSpace,
               sizeof(*closestPointOnSurface));
    }
    else
    {
        *collision = SXR_3DR_COLLISION_UNKNOWN;
        *distanceToSurface = 0.0f;
        memset(closestPointOnSurface, 0, sizeof(*closestPointOnSurface));
    }
}
