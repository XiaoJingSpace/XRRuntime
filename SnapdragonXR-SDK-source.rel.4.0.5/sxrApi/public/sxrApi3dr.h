//=============================================================================
//! \file sxrApi3dr.h
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================

#ifndef _SXR_API_3DR_H_
#define _SXR_API_3DR_H_

#include "sxrApi.h"

#ifdef __cplusplus
extern "C" {
#endif

//! \brief Enum used to indicate the result of 3dr function call
enum Sxr3drResult
{
    SXR_3DR_SUCCESS = 0,                    /*!< Return code for success. */
    SXR_3DR_ERROR_UNSUPPORTED,              /*!< Return code to indicate that the device doesn't support 3DR feature. */
    SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED,   /*!< Return code for calls to functions made without first calling sxrInitialize. */
    SXR_3DR_ERROR_NOT_INITIALIZED,          /*!< Return code for calls to functions made without first calling sxr3drInitialize. */
    SXR_3DR_ERROR_PENDING,                  /*!< Return code to indicate that the data is still pending and not available now. */
    SXR_3DR_ERROR_SIZE_INSUFFICIENT,        /*!< Return code to indicate that the size of given buffer is too small to hold the data. */
    SXR_3DR_ERROR_INVALID_PARAMETER,        /*!< Return code to indicate that given parameter is invalid. */
    SXR_3DR_ERROR_INVALID_OPERATION         /*!< Return code to indicate that operation is invalid. */
};

//! \brief Enum used to indicate the orientation of plane
enum SxrPlaneOrientation
{
    SXR_3DR_PLANE_HORIZONTAL = 0,   /*!< Plane is horizontal. */
    SXR_3DR_PLANE_VERTICAL,         /*!< Plane is vertical. */
    SXR_3DR_PLANE_SLANT             /*!< Plane is neither horizontal nor vertical. */
};

//! \brief Enum used to indicate the result of intersection test
enum SxrCollisionResult
{
    SXR_3DR_NO_COLLISION = 0,   /*!< No collision. */
    SXR_3DR_COLLISION,          /*!< Collision with surface. */
    SXR_3DR_COLLISION_UNKNOWN   /*!< The object (point, box or sphere) is not near any observed surface. */
};

//! \brief Initialize 3DR module.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED if #sxrInitialize was not called.
//!     - #SXR_3DR_ERROR_UNSUPPORTED if the device doesn't support 3dr functionality.
SXRP_EXPORT Sxr3drResult sxr3drInitialize();

//! \brief Shutdown 3DR module.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED if #sxrInitialize was not called.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED #sxr3drInitialize was not called.
SXRP_EXPORT Sxr3drResult sxr3drShutdown();

//! \brief Get the surface mesh. The caller must call ReleaseSurfaceMesh if the return code was
//!        #SXR_3DR_SUCCESS to unlock the mesh data. Please be noted that the mesh data is in 3dr
//!        coordinate space instead of right-handed coordinate space. To render the mesh properly,
//!        use #sxrGetQvrDataTransform as model matrix.
//! \param [in,out] numVertices Number of mesh vertices.
//! \param [in,out] vertices Pointer to a pointer of sxrVector3, this will point to the internal
//!                          memory that holds the vertices data. Caller doesn't take the ownership
//!                          of data.
//! \param [in,out] numIndices Number of mesh indices.
//! \param [in,out] indices Pointer to a pointer of uint32_t, this will point to the internal
//!                         memory that holds the indices data. Caller doesn't take the ownership
//!                         of data.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_PENDING if the data was not ready yet.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
SXRP_EXPORT Sxr3drResult sxr3drGetSurfaceMesh(uint32_t *numVertices, sxrVector3 **vertices,
                                              uint32_t *numIndices, uint32_t **indices);

//! \brief Release the surface mesh previously acquired by #sxrGetSurfaceMesh.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_INVALID_OPERATION if #sxrGetSurfaceMesh was not called.
SXRP_EXPORT Sxr3drResult sxr3drReleaseSurfaceMesh();

//! \brief Get the number of planes and its id.
//! \param [in,out] numPlanes Number of planes.
//! \param [in,out] planeIds Pointer to an array of uint32_t; can be null which means to retrieve
//!                          the number of planes.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_PENDING if the data was not ready yet.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
//!     - #SXR_3DR_ERROR_SIZE_INSUFFICIENT if planeIds is too small hold the data.
SXRP_EXPORT Sxr3drResult sxr3drGetPlanes(uint32_t *numPlanes, uint32_t *planeIds);

//! \brief Get the root plane of given plane id.
//! \param [in] planeId Id of target plane.
//! \param [in,out] rootPlaneId Id of root plane.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_PENDING if the data was not ready yet.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if rootPlaneId is null.
SXRP_EXPORT Sxr3drResult sxr3drGetRootPlane(uint32_t planeId, uint32_t *rootPlaneId);

//! \brief Get the geometry of given plane id. The output geometry is a convex hull. Please be noted
//!        that the geometry data is in 3dr coordinate space instead of right-handed coordinate
//!        space. To render the geometry properly, use #sxrGetQvrDataTransform as model matrix.
//! \param [in] planeId Id of target plane.
//! \param [in,out] numVertices Number of vertices.
//! \param [in,out] vertices Pointer to an array of sxrVector3, the first vertex and the last vertex
//!                          is the same to form a convex hull; can be null which means to retrieve
//!                          the number of vertices.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_PENDING if the data was not ready yet.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
//!     - #SXR_3DR_ERROR_SIZE_INSUFFICIENT if vertices is too small to hold the data.
SXRP_EXPORT Sxr3drResult sxr3drGetPlaneGeometry(uint32_t planeId, uint32_t *numVertices,
                                                sxrVector3 *vertices);

//! \brief Get the orientation of given plane id.
//! \param [in] planeId Id of target plane.
//! \param [in,out] orientation Orientation of plane.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_PENDING if the data was not ready yet.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if orientation is null.
SXRP_EXPORT Sxr3drResult sxr3drGetPlaneOrientation(uint32_t planeId,
                                                   SxrPlaneOrientation *orientation);

//! \brief Check if given point intersects with any surface.
//! \param [in] point Point for intersection test, in right-handed coordinate space
//! \param [in] maxClearance Maximum distance in meters of the surface from the test point for
//!                          surface to be detected. If maxClearance <= 0, closest distance to
//!                          surface is not searched for if there is no collision
//! \param [in,out] collision Result of intersection test.
//! \param [in,out] distanceToSurface Distance to the surface.
//! \param [in,out] closestPointOnSurface Closest point on the surface.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
SXRP_EXPORT Sxr3drResult sxr3drCheckPointIntersect(const sxrVector3 *point, float maxClearance,
                                                   SxrCollisionResult *collision,
                                                   float *distanceToSurface,
                                                   sxrVector3 *closestPointOnSurface);

//! \brief Check if given bounding box intersects with any surface.
//! \param [in] center Center of bounding box, in right-handed coordinate space.
//! \param [in] extents Extent of bounding box.
//! \param [in] maxClearance Maximum distance in meters of the surface from the test point for
//!                          surface to be detected. If maxClearance <= 0, closest distance to
//!                          surface is not searched for if there is no collision
//! \param [in,out] collision Result of intersection test.
//! \param [in,out] distanceToSurface Distance to the surface.
//! \param [in,out] closestPointOnSurface Closest point on the surface.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
SXRP_EXPORT Sxr3drResult sxr3drCheckAABBIntersect(const sxrVector3 *center,
                                                  const sxrVector3 *extents,
                                                  float maxClearance, SxrCollisionResult *collision,
                                                  float *distanceToSurface,
                                                  sxrVector3 *closestPointOnSurface);

//! \brief Check if given sphere intersects with any surface.
//! \param [in] center Center of sphere, in right-handed coordinate space.
//! \param [in] radius Radius of sphere.
//! \param [in] maxClearance Maximum distance in meters of the surface from the test point for
//!                          surface to be detected. If maxClearance <= 0, closest distance to
//!                          surface is not searched for if there is no collision
//! \param [in,out] collision The result of intersection test.
//! \param [in,out] distanceToSurface Distance to the surface.
//! \param [in,out] closestPointOnSurface Closest point on the surface.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
SXRP_EXPORT Sxr3drResult sxr3drCheckSphereIntersect(const sxrVector3 *center, float radius,
                                                    float maxClearance,
                                                    SxrCollisionResult *collision,
                                                    float *distanceToSurface,
                                                    sxrVector3 *closestPointOnSurface);

//! \brief Computes the position and orientation of the surface closest to the beginning of the
//!        given ray.
//! \param [in] rayStartPosition Position in right-handed coordinates at which the ray begins.
//! \param [in] rayDirection Direction of the ray in right-handed coordinates.
//! \param [in] maxDistance Maximum distance to the position where object can be placed.
//! \param [in,out] surfaceNormalEstimated Indicates whether there were enough samples in the
//!                                        neighbourhood for a surface normal to be estimated.
//! \param [in,out] poseLocalToWorld Transformation from local coordinates to world coordinates. In
//!                                  general, the local Y+ is the surface normal, X+ is perpendicular
//!                                  to the casted ray and parallel to the plane and Z+ is parallel
//!                                  to the plane and perpendicular to X+ and roughly in the direction
//!                                  of the user. This is so that the orientation is a best effort to
//!                                  face the user's device.  For the degenerate case when the casted
//!                                  ray is parallel to the surface normal (front-parallel surface),
//!                                  then X+ is perpendicular to the cast ray and points right,
//!                                  Y+ point up and Z+ points roughly towards the user's device.
//! \return
//!     - #SXR_3DR_SUCCESS upon success.
//!     - #SXR_3DR_ERROR_NOT_INITIALIZED if #sxr3drInitialize was not called.
//!     - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
SXRP_EXPORT Sxr3drResult sxr3drGetPlacementInfo(const sxrVector3 *rayStartPosition,
                                                const sxrVector3 *rayDirection, float maxDistance,
                                                bool *surfaceNormalEstimated,
                                                sxrHeadPose *poseLocalToWorld);

#ifdef __cplusplus
}
#endif

#endif // _SXR_API_3DR_H_
