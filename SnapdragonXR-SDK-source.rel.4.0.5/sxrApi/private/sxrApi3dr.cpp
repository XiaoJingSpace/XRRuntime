/**********************************************************
 * FILE: sxrApi3dr.cpp
 *
 * Copyright (c) 2020 QUALCOMM Technologies Inc.
 **********************************************************/
#include "sxrApi3dr.h"

#include "svr3drCore.h"
#include "svrApiCore.h"
#include "svrUtil.h"

Svr3drCore gSvr3drCore;

Sxr3drResult sxr3drInitialize()
{
    LOGI("sxr3drInitialize");

    if (Svr::gAppContext == nullptr)
    {
        return SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    return gSvr3drCore.Initialize(Svr::gAppContext->qvrHelper);
}

Sxr3drResult sxr3drShutdown()
{
    LOGI("sxr3drShutdown");

    if (Svr::gAppContext == nullptr)
    {
        return SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    return gSvr3drCore.Shutdown(Svr::gAppContext->qvrHelper);
}

Sxr3drResult sxr3drGetSurfaceMesh(uint32_t *numVertices, sxrVector3 **vertices,
                                  uint32_t *numIndices, uint32_t **indices)
{
    return gSvr3drCore.GetSurfaceMesh(numVertices, vertices, numIndices, indices);
}

Sxr3drResult sxr3drReleaseSurfaceMesh()
{
    return gSvr3drCore.ReleaseSurfaceMesh();
}

Sxr3drResult sxr3drGetPlanes(uint32_t *numPlanes, uint32_t *planeIds)
{
    return gSvr3drCore.GetPlanes(numPlanes, planeIds);
}

Sxr3drResult sxr3drGetRootPlane(uint32_t planeId, uint32_t *rootPlaneId)
{
    return gSvr3drCore.GetRootPlane(planeId, rootPlaneId);
}

Sxr3drResult sxr3drGetPlaneGeometry(uint32_t planeId, uint32_t *numVertices, sxrVector3 *vertices)
{
    return gSvr3drCore.GetPlaneGeometry(planeId, numVertices, vertices);
}

Sxr3drResult sxr3drGetPlaneOrientation(uint32_t planeId, SxrPlaneOrientation *orientation)
{
    return gSvr3drCore.GetPlaneOrientation(planeId, orientation);
}

Sxr3drResult sxr3drCheckPointIntersect(const sxrVector3 *point, float maxClearance,
                                       SxrCollisionResult *collision, float *distanceToSurface,
                                       sxrVector3 *closestPointOnSurface)
{
    return gSvr3drCore.CheckPointIntersect(point, maxClearance, collision, distanceToSurface,
                                           closestPointOnSurface);
}

Sxr3drResult sxr3drCheckAABBIntersect(const sxrVector3 *center, const sxrVector3 *extents,
                                      float maxClearance, SxrCollisionResult *collision,
                                      float *distanceToSurface, sxrVector3 *closestPointOnSurface)
{
    return gSvr3drCore.CheckAABBIntersect(center, extents, maxClearance, collision,
                                          distanceToSurface, closestPointOnSurface);
}

Sxr3drResult sxr3drCheckSphereIntersect(const sxrVector3 *center, float radius, float maxClearance,
                                        SxrCollisionResult *collision, float *distanceToSurface,
                                        sxrVector3 *closestPointOnSurface)
{
    return gSvr3drCore.CheckSphereIntersect(center, radius, maxClearance, collision,
                                            distanceToSurface, closestPointOnSurface);
}

Sxr3drResult sxr3drGetPlacementInfo(const sxrVector3 *rayStartPosition,
                                    const sxrVector3 *rayDirection,
                                    float maxDistance, bool *surfaceNormalEstimated,
                                    sxrHeadPose *poseLocalToWorld)
{
    return gSvr3drCore.GetPlacementInfo(rayStartPosition, rayDirection, maxDistance,
                                        surfaceNormalEstimated, poseLocalToWorld);
}
