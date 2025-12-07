/**********************************************************
 * FILE: svr3drCore.h
 *
 * Copyright (c) 2020 QUALCOMM Technologies Inc.
 **********************************************************/
#ifndef _SVR_3DR_CORE_H_
#define _SVR_3DR_CORE_H_

#include <glm/mat4x4.hpp>

#include <beta/QVR3DR.h>
#include <QVRServiceClient.h>

#include "sxrApi3dr.h"

/*!
 * Svr3drCore handles the QVR3DR function calls, processing input/output data to appropriate
 * coordinate space. Please be noted that all the input and output of all functions are in
 * right-handed coordinate space.
 */
class Svr3drCore {
public:
    /*!
     * Constructor.
     */
    Svr3drCore();

    /*!
     * Destructor.
     */
    virtual ~Svr3drCore();

    /*!
     * Initialize 3dr module.
     *
     * \param[in] helper qvrservice_client_helper_t handle.
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED if helper is null.
     *    - #SXR_3DR_ERROR_UNSUPPORTED if 3dr is not supported.
     */
    Sxr3drResult Initialize(qvrservice_client_helper_t *helper);

    /*!
     * Shutdown 3dr module.
     *
     * \param[in] helper qvrservice_client_helper_t handle.
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_VRMODE_NOT_INITIALIZED if helper is null.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     */
    Sxr3drResult Shutdown(qvrservice_client_helper_t *helper);

    /*!
     * Get surface mesh. The caller must call #ReleaseSurfaceMesh if the return code was
     * #SXR_3DR_SUCCESS to unlock the mesh data.
     *
     * \param[in,out] numVertices Number of vertices.
     * \param[in,out] vertices Pointer that points to the vertices data. Caller doesn't take the
     *                         ownership of the data.
     * \param[in,out] numIndices Number of indices.
     * \param[in,out] indices Pointer that points to the indices data. Caller doesn't take the
     *                        ownership of the data.
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     *    - #SXR_3DR_ERROR_PENDING if mesh is not ready.
     *    - #SXR_3DR_ERROR_INVALID_OPERATION if error occurs.
     */
    Sxr3drResult GetSurfaceMesh(uint32_t *numVertices, sxrVector3 **vertices, uint32_t *numIndices,
                                uint32_t **indices);

    /*!
     * Release surface mesh which previously locked via #GetSurfaceMesh.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_OPERATION the mesh was not locked.
     */
    Sxr3drResult ReleaseSurfaceMesh();

    /*!
     * Retrieve number of planes and plane ids.
     *
     * \param[in,out] numPlanes Number of planes.
     * \param[in,out] planeIds Buffer that holds the plane data.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     *    - #SXR_3DR_ERROR_PENDING if plane data is not ready.
     *    - #SXR_3DR_ERROR_SIZE_INSUFFICIENT if given planeIds is not able to hold the data.
     */
    Sxr3drResult GetPlanes(uint32_t *numPlanes, uint32_t *planeIds);

    /*!
     * Get the root plane id of given plane id.
     *
     * \param[in] planeId Id of target plane.
     * \param[in,out] rootPlaneId Id of root plane.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if rootPlaneId is null.
     *    - #SXR_3DR_ERROR_PENDING if plane data is not ready.
     */
    Sxr3drResult GetRootPlane(uint32_t planeId, uint32_t *rootPlaneId);

    /*!
     * Get the geometry data of given plane id.
     *
     * \param[in] planeId Id of target plane.
     * \param[in,out] numVertices Number of vertices.
     * \param[in,out] vertices Buffer that holds the vertices data.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     *    - #SXR_3DR_ERROR_PENDING if plane data is not ready.
     *    - #SXR_3DR_ERROR_SIZE_INSUFFICIENT if given vertices is not able to hold the data.
     */
    Sxr3drResult GetPlaneGeometry(uint32_t planeId, uint32_t *numVertices, sxrVector3 *vertices);

    /*!
     * Get the orientation of given plane id.
     *
     * \param[in] planeId Id of target plane.
     * \param[in,out] orientation Orientation of target plane.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if orientation is null.
     *    - #SXR_3DR_ERROR_PENDING if plane data is not ready.
     */
    Sxr3drResult GetPlaneOrientation(uint32_t planeId, SxrPlaneOrientation *orientation);

    /*!
     * Check if given point intersects with any surface.
     *
     * \param[in] point Point to be checked in right-handed coordinate space.
     * \param[in] maxClearance Maximum distance in meters of the surface from the test point for surface to be detected.
     * \param[in,out] collision Result of intersection test.
     * \param[in,out] distanceToSurface Distance to the surface.
     * \param[in,out] closestPointOnSurface Closest point on the surface.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     */
    Sxr3drResult CheckPointIntersect(const sxrVector3 *point, float maxClearance,
                                     SxrCollisionResult *collision, float *distanceToSurface,
                                     sxrVector3 *closestPointOnSurface);

    /*!
     * Check if given bounding box intersects with any surface.
     *
     * \param[in] center Center of bounding box in right-handed coordinate space.
     * \param[in] extents Extent of bounding box.
     * \param[in] maxClearance Maximum distance in meters of the surface from the test point for surface to be detected.
     * \param[in,out] collision Result of intersection test.
     * \param[in,out] distanceToSurface Distance to the surface.
     * \param[in,out] closestPointOnSurface Closest point on the surface.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     */
    Sxr3drResult CheckAABBIntersect(const sxrVector3 *center, const sxrVector3 *extents,
                                    float maxClearance, SxrCollisionResult *collision,
                                    float *distanceToSurface, sxrVector3 *closestPointOnSurface);

    /*!
     * Check if given sphere intersects with any surface.
     *
     * \param[in] center Center of sphere in right-handed coordinate space.
     * \param[in] radius Radius of sphere.
     * \param[in] maxClearance Maximum distance in meters of the surface from the test point for surface to be detected.
     * \param[in,out] collision Result of intersection test.
     * \param[in,out] distanceToSurface Distance to the surface.
     * \param[in,out] closestPointOnSurface Closest point on the surface.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     */
    Sxr3drResult CheckSphereIntersect(const sxrVector3 *center, float radius, float maxClearance,
                                      SxrCollisionResult *collision, float *distanceToSurface,
                                      sxrVector3 *closestPointOnSurface);

    /*!
     * Computes the position and orientation of the surface closest to the beginning of the given ray.
     *
     * \param[in] rayStartPosition Position in right-handed coordinate space at which the ray begins.
     * \param[in] rayDirection Direction of the ray in right-handed coordinate space.
     * \param[in] maxDistance Maximum distance to the position where object can be placed.
     * \param[in,out] surfaceNormalEstimated Indicates whether there were enough samples in the
     *                                       neighbourhood for a surface normal to be estimated.
     * \param[in,out] poseLocalToWorld Transformation from local coordinates to right-handed coordinates.
     *                                 In general, the local Y+ is the surface normal, X+ is
     *                                 perpendicular to the casted ray and parallel to the plane and
     *                                 Z+ is parallel to the plane and perpendicular to X+ and
     *                                 roughly in the direction of the user. This is so that the
     *                                 orientation is a best effort to face the user's device. For
     *                                 the degenerate case when the casted ray is parallel to the
     *                                 surface normal (front-parallel surface), then X+ is
     *                                 perpendicular to the cast ray and points right, Y+ point up
     *                                 and Z+ points roughly towards the user's device.
     *
     * \return
     *    - #SXR_3DR_SUCCESS upon success.
     *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_3DR_ERROR_INVALID_PARAMETER if any given parameter is invalid.
     */
    Sxr3drResult GetPlacementInfo(const sxrVector3 *rayStartPosition,
                                  const sxrVector3 *rayDirection, float maxDistance,
                                  bool *surfaceNormalEstimated, sxrHeadPose *poseLocalToWorld);

private:
    struct Transform {
        bool ready;
        glm::mat4 qvrToGlMatrix;
        glm::mat4 glToQvrMatrix;

        Transform() : ready(false), qvrToGlMatrix(1.0f), glToQvrMatrix(1.0f)
        {
        }
    };

    Sxr3drResult QvrErrorToSxrError(int error);

    SxrCollisionResult QvrCollisionToSxrCollision(QVR3DR_COLLISION_RESULT collision);

    void GetTransformMatrixIfNeeded();

    sxrVector3 Rotate(const XrVector3fQTI &src, const glm::mat4 &rotationMatrix);

    XrVector3fQTI Rotate(const sxrVector3 &src, const glm::mat4 &rotationMatrix);

    void FillCollisionResult(const qvr3dr_collision_warning_result_t *result,
                             SxrCollisionResult *collision, float *distanceToSurface,
                             sxrVector3 *closestPointOnSurface);

    qvrservice_class_t *m3drHandle;

    qvr3dr_polygon_mesh_t *mSurfaceMesh;

    Transform mTransform;
};

#endif // _SVR_3DR_CORE_H_
