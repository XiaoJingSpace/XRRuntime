/**********************************************************************************************************************/
/*! \file  QVR3DR.h */
/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
***********************************************************************************************************************/
#ifndef QVR3DR_H
#define QVR3DR_H

/**
 * @addtogroup qvr_3dr
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "QVRTypes.h"
#include "QXR.h"

#define CLASS_ID_3DR_BETA     0xff000002

/*******************************************************************************************************************//**
* \file
* APIs to communicate with QVR3DR. Typical call flow is as follows:
*   1. Create QVRServiceClient object
*   2. Call QVRServiceClient_GetClassHandle() to get a QVR3DR class handle
*   3. Call the other QVR3DR_* APIs using this handle.
*   4. Call QVRServiceClient_ReleaseClassHandle() to release the QVR3DR class handle.
*
* This API is considered BETA and is therefore subject to change and/or removal entirely. Use at your own risk!
***********************************************************************************************************************/

/*******************************************************************************************************************//**
* \enum QVR3DR_CLIENT_NOTIFICATION
*
*   \var NOTIFICATION_3DR_PLANES_READY
*      Result from plane detection algorithm is ready.
*   \var NOTIFICATION_3DR_SURFACE_MESH_READY
*      Surface mesh from 3D reconstruction is ready.
***********************************************************************************************************************/
typedef enum {
    NOTIFICATION_3DR_PLANES_READY,
    NOTIFICATION_3DR_SURFACE_MESH_READY,
    NOTIFICATION_3DR_MAX
} QVR3DR_CLIENT_NOTIFICATION;

/*******************************************************************************************************************//**
* Callback for handling 3DR client notifications.  It is registered using RegisterForNotification().
* @param[in] pCtx           The context passed to RegisterForNotification().
* @param[in] notification   Notification value specifying the reason for the callback.
* @param[in] pPayload       Pointer to payload. Payload type depends on notification.  Memory for the payload is
*                           allocated by qvr service client and will be released when call back returns.
* @param[in] payloadLength  Length of valid data in payload.
***********************************************************************************************************************/
typedef void (*notification_3DR_callback_fn)(void *pCtx, QVR3DR_CLIENT_NOTIFICATION notification, void *pPayload,
    uint32_t payloadLength);

/*******************************************************************************************************************//**
* \enum QVR3DR_API_VERSION
*
* Defines the API versions of this interface. The api_version member of the qvrservice_class_t structure must be set to
* the version of the API that the implementation supports.
***********************************************************************************************************************/
typedef enum {
    QVR3DR_API_VERSION_BETA_1 = 0xfff0,
} QVR3DR_API_VERSION;

/*******************************************************************************************************************//**
* \enum QVR3DR_PLANE_ORIENTATION
*
* \var QVR3DR_PLANE_HORIZONTAL
*     Horizontal
* \var QVR3DR_PLANE_VERTICAL
*     Vertical
* \var QVR3DR_PLANE_SLANT
*     The plane is neither horizontal nor vertical
***********************************************************************************************************************/
typedef enum {
    QVR3DR_PLANE_HORIZONTAL = 0,
    QVR3DR_PLANE_VERTICAL,
    QVR3DR_PLANE_SLANT,
} QVR3DR_PLANE_ORIENTATION;

/*******************************************************************************************************************//**
* \enum QVR3DR_COLLISION_RESULT
*
* \var QVR3DR_NO_COLLISION
*     No collision
* \var QVR3DR_COLLISION
*     Collision with surface
* \var QVR3DR_UNKNOWN
*     The object (point, box or sphere) is not near any observed surface
***********************************************************************************************************************/
typedef enum {
    QVR3DR_NO_COLLISION = 0,
    QVR3DR_COLLISION,
    QVR3DR_UNKNOWN,
} QVR3DR_COLLISION_RESULT;

/*******************************************************************************************************************//**
* This structure is used to retrieve the result of intersection test.
***********************************************************************************************************************/
typedef struct {
    QVR3DR_COLLISION_RESULT collision;    /**< Indicates if the shape collides with the volume. */
    float distanceToSurface;              /**< Closest distance of the collision shape to the surface. -1 if no
                                               surface was found within maximum clearance distance. */
    XrVector3fQTI closestPointOnSurface;  /**< Closet point on the surface. */
} qvr3dr_collision_warning_result_t;

/*******************************************************************************************************************//**
* This structure is used to store mesh data.
***********************************************************************************************************************/
typedef struct {
    uint32_t numVertices;     /**< Number of vertices. */
    uint32_t numIndices;      /**< Number of indices. */
    XrVector3fQTI* vertices;  /**< Array of vertices. */
    uint32_t* indices;        /**< Array of vertex indices where each consecutive triplet form a mesh triangle, e.g.,
                                   vertices[triangles[i]], vertices[triangle[i+1]], vertices[triangle[i+2]] form a mesh
                                   triangle for i = 0, 3, 6, 9, etc. */
} qvr3dr_polygon_mesh_t;

/*******************************************************************************************************************//**
* This structure is used to retrieve the result of raycast test.
***********************************************************************************************************************/
typedef struct {
    bool surfaceNormalEstimated;  /**< Indicates whether there were enough samples in the neighbourhood for a surface
                                       normal to be estimated. */
    XrPosefQTI poseLocalToWorld;  /**< Transformation from local coordinates to world coordinates. In general, the local
                                       Y+ is the surface normal, X+ is perpendicular to the casted ray and parallel to
                                       the plane and Z+ is parallel to the plane and perpendicular to X+ and roughly in
                                       the direction of the user.  This is so that the orientation is a best effort to
                                       face the user's device.  For the degenerate case when the casted ray is parallel
                                       to the surface normal (front-parallel surface), then X+ is perpendicular to the
                                       cast ray and points right, Y+ point up and Z+ points roughly towards the user's
                                       device. */
} qvr3dr_placement_info_t;

typedef struct {
    int32_t (*RegisterForNotification)(qvrservice_class_t* me, QVR3DR_CLIENT_NOTIFICATION notification,
        notification_3DR_callback_fn cb, void *pCtx);

    int32_t (*GetPlanes)(qvrservice_class_t* me, uint32_t* pLen, uint32_t* pPlaneIds);

    int32_t (*GetRootPlane)(qvrservice_class_t* me, uint32_t planeId, uint32_t* pRootPlaneId);

    int32_t (*GetPlaneGeometry)(qvrservice_class_t* me, uint32_t planeId, uint32_t* pLen, float* pPoints);

    int32_t (*GetPlaneOrientation)(qvrservice_class_t* me, uint32_t planeId, QVR3DR_PLANE_ORIENTATION* pOrientation);

    int32_t (*CheckPointIntersect)(qvrservice_class_t* me, const XrVector3fQTI* pPoint, float maxClearance,
        qvr3dr_collision_warning_result_t* pCollisionWarningResult);

    int32_t (*CheckAABBIntersect)(qvrservice_class_t* me, const XrVector3fQTI* pCenter, const XrVector3fQTI* pExtents,
        float maxClearance, qvr3dr_collision_warning_result_t* pCollisionWarningResult);

    int32_t (*CheckSphereIntersect)(qvrservice_class_t* me, const XrVector3fQTI* pCenter, float radius,
        float maxClearance, qvr3dr_collision_warning_result_t* pCollisionWarningResult);

    int32_t (*GetSurfaceMesh)(qvrservice_class_t* me, qvr3dr_polygon_mesh_t** ppPolygonMesh);

    int32_t (*ReleaseSurfaceMesh)(qvrservice_class_t* me, qvr3dr_polygon_mesh_t* pPolygonMesh);

    int32_t (*GetPlacementInfo)(qvrservice_class_t* me, const XrVector3fQTI *pRayStartPosition,
        const XrVector3fQTI *pRayDirection, qvr3dr_placement_info_t* pPlacementInfo, float maxDistance);

    /* Reserved for future use */
    void* reserve[64 - 11];
} qvr3dr_ops_t;


/*******************************************************************************************************************//**
* Register for 3DR event notifications.
*
* @param[in] me            qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in] notification  QVRSERVICE_CLIENT_NOTIFICATION to register for.
* @param[in] cb            Callback function of type notification_callback_fn.
* @param[in] pCtx          Context to be passed to callback function.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    The client will maintain only one callback, so subsequent calls to this function will overwrite the previous
*    callback. cb may be set to NULL to disable notification callbacks.
***********************************************************************************************************************/
static inline int32_t QVR3DR_RegisterForNotification(qvrservice_class_t* me, QVR3DR_CLIENT_NOTIFICATION notification,
    notification_3DR_callback_fn cb, void *pCtx)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->RegisterForNotification) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->RegisterForNotification(me, notification, cb, pCtx);
}

/*******************************************************************************************************************//**
* Get a list of ids of the detected planes.  The plane id is used in subsequent API's to get additional information
* about the plane.
*
* @param[in]    me         qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[inout] pLen       If pPlaneIds is NULL, pLen will be filled in with the number of bytes that may be returned.
*                          If pPlaneIds is non-NULL, pLen must point to an integer that represents the size of the
*                          buffer pointed to by pPlaneIds.  pLen must not be NULL.
* @param[inout] pPlaneIds  Buffer to receive value.
* \return
*    QVR_SUCCESS upon success.
*    QVR_RESULT_PENDING if planes are not ready. The caller may call the function again to poll for the status of the
*    planes or wait for a callback when the planes are ready if registered.
*    QVR error code on failure.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    The pPlaneIds buffer will be filled in up to *pLen bytes.  This may result in truncation of the value if the
*    required length is larger than the size passed in pLen.
***********************************************************************************************************************/
static inline int32_t QVR3DR_GetPlanes(qvrservice_class_t* me, uint32_t* pLen, uint32_t* pPlaneIds)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->GetPlanes) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->GetPlanes(me, pLen, pPlaneIds);
}

/*******************************************************************************************************************//**
* Get the root plane that has subsumed this plane.  A large plane/surface may be discovered in segments.  In time, plane
* segments subsume other plane segements to form a larger planes.  Example:  there exist planes with id 5, 8, 11.  As
* 3DR algorithm learns more about the environment, it discovers that plane 5 and plane 8 are parts of the same surface
* Plane 8 subsumes plane 5.  Calling GetRootPlane() with plane id 5 returns plane id 8 as the root plane.  Later the 3DR
* algorithm discovers that planes 8 and 11 are parts of the same larger surface.  Plane 11 subsumes plane 8.  Calling
* GetRootPlane() with plane id 5 or 8 returns plane id 11 as the root plane.
*
* @param[in]    me            qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]    planeId       Plane identifier obtained from GetPlanes().
* @param[inout] pRootPlaneId  Buffer to receive value.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_GetRootPlane(qvrservice_class_t* me, uint32_t planeId, uint32_t* pRootPlaneId)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->GetRootPlane) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->GetRootPlane(me, planeId, pRootPlaneId);
}

/*******************************************************************************************************************//**
* Get points along the plane's contour.
*
* @param[in]    me       qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]    planeId  Plane identifier obtained from GetPlanes().
* @param[inout] pLen     If pPoints is NULL, pLen will be filled in with the number of bytes that may be returned.  If
*                        pPoints is non-NULL, pLen must point to an integer that represents the size of the buffer
*                        pointed to by pPoints. pLen must not be NULL.
* @param[inout] pPoints  Pointer to the root plane's plane id.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    The pPoints buffer will be filled in up to *pLen bytes.  This may result in truncation of the value if the required
*    length is larger than the size passed in pLen.
***********************************************************************************************************************/
static inline int32_t QVR3DR_GetPlaneGeometry(qvrservice_class_t* me, uint32_t planeId, uint32_t* pNum3DPoints,
    float* p3DPoints)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->GetPlaneGeometry) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->GetPlaneGeometry(me, planeId, pNum3DPoints, p3DPoints);
}

/*******************************************************************************************************************//**
* Get plane's orientation.
*
* @param[in]  me            qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]  planeId       Plane id obtained from GetPlanes().
* @param[out] pOrientation  Pointer to plane's orientation.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_GetPlaneOrientation(qvrservice_class_t* me, uint32_t planeId,
    QVR3DR_PLANE_ORIENTATION* pOrientation)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->GetPlaneOrientation) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->GetPlaneOrientation(me, planeId, pOrientation);
}

/*******************************************************************************************************************//**
* Check if a given point in world coordinates is enclosed by a surface.
*
* @param[in]  me                       qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]  pPoint                   Point in world coordinates.
* @param[in]  maxClearance             Maximum distance in meters of the surface from the test point for surface to be
*                                      detected.  If maxClearance <= 0, closest distance to surface is not searched if
*                                      there is no collision.
* @param[out] pCollisionWarningResult  Pointer to collision warning result.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_CheckPointIntersect(qvrservice_class_t* me, const XrVector3fQTI* pPoint,
    float maxClearance, qvr3dr_collision_warning_result_t* pCollisionWarningResult)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->CheckPointIntersect) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->CheckPointIntersect(me, pPoint, maxClearance, pCollisionWarningResult);
}

/*******************************************************************************************************************//**
* Check if an axis-aligned bounding box intersects a surface.
*
* @param[in]  me                       qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]  pCenter                  Bounding box center in world coordinates.
* @param[in]  pExtents                 Bounding box extents.
* @param[in]  maxClearance             Maximum distance in meters of the surface from the test point for surface to be
*                                      detected. If maxClearance <= 0, closest distance to surface is not searched if
*                                      there is no collision.
* @param[out] pCollisionWarningResult  Pointer to collision warning result.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_CheckAABBIntersect(qvrservice_class_t* me, const XrVector3fQTI* pCenter,
    const XrVector3fQTI* pExtents, float maxClearance, qvr3dr_collision_warning_result_t* pCollisionWarningResult)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->CheckAABBIntersect) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->CheckAABBIntersect(me, pCenter, pExtents, maxClearance,
        pCollisionWarningResult);
}

/*******************************************************************************************************************//**
* Check if a sphere intersects a surface.
*
* @param[in]  me                       qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]  pCenter                  Center of the sphere in world coordinates.
* @param[in]  radius                   Radius of the sphere.
* @param[in]  maxClearance             Maximum distance in meters of the surface from the test point for surface to be
*                                      detected. If maxClearance <= 0, closest distance to surface is not searched if
*                                      there is no collision.
* @param[out] pCollisionWarningResult  Pointer to collision warning result.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_CheckSphereIntersect(qvrservice_class_t* me, const XrVector3fQTI* pCenter, float radius,
    float maxClearance, qvr3dr_collision_warning_result_t* pCollisionWarningResult)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->CheckSphereIntersect) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->CheckSphereIntersect(me, pCenter, radius, maxClearance,
        pCollisionWarningResult);
}

/*******************************************************************************************************************//**
* Computes a polygon mesh from surface extraction of captured volume.
*
* @param[in]  me             qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[out] ppPolygonMesh  Address of pointer to qvrservice_polygon_mesh_t structure receive the most recent surface
*                            mesh. Output vertices are in world coordinates.
* \return
*    QVR_SUCCESS upon success.
*    QVR_RESULT_PENDING if mesh is not ready. The caller may call the function again to poll for the status of the mesh
*    or wait for a callback when the mesh is ready if registered.
*    QVR error code on failure.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_GetSurfaceMesh(qvrservice_class_t* me, qvr3dr_polygon_mesh_t** ppPolygonMesh)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->GetSurfaceMesh) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->GetSurfaceMesh(me, ppPolygonMesh);
}

/*******************************************************************************************************************//**
* Releases memory associated with surface mesh.
*
* @param[in]  me            qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[out] pPolygonMesh  Polygon mesh from GetSurfaceMesh.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_ReleaseSurfaceMesh(qvrservice_class_t* me, qvr3dr_polygon_mesh_t* pPolygonMesh)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->ReleaseSurfaceMesh) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->ReleaseSurfaceMesh(me, pPolygonMesh);
}

/*****************************************************************************************************************8**//**
* Computes the position of the closest surface along a given ray from the camera.
*
* @param[in]  me                 qvrservice_class_t returned by QVRServiceClient_GetClassHandle().
* @param[in]  pRayStartPosition  Position in world coordinates at which the ray begins.
* @param[in]  pRayDirection      Direction of the ray in world coordinates.
* @param[out] pPlacementInfo     Placement info containing the 3D position (in world coordinates) of the closest surface
*                                from the camera along the ray and related attributes for object placement.  There is no
*                                collision if the rotation is identity and position is zero vector.
* @param[in]  maxDistance        Maximum distance to the position where object can be placed.
* \return
*    Returns QVR_SUCCESS upon success, other QVR error code otherwise.
* \par API level
*    BETA 1
* \par Timing requirements
*    None
* \par Notes
*    None
***********************************************************************************************************************/
static inline int32_t QVR3DR_GetPlacementInfo(qvrservice_class_t* me, const XrVector3fQTI* pRayStartPosition,
    const XrVector3fQTI* pRayDirection, qvr3dr_placement_info_t* pPlacementInfo, float maxDistance)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->api_version < QVR3DR_API_VERSION_BETA_1) return QVR_API_NOT_SUPPORTED;
    if (!((qvr3dr_ops_t*)me->ops)->GetPlacementInfo) return QVR_API_NOT_SUPPORTED;
    return ((qvr3dr_ops_t*)me->ops)->GetPlacementInfo(me, pRayStartPosition, pRayDirection, pPlacementInfo,
        maxDistance);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVR3DR_H */
