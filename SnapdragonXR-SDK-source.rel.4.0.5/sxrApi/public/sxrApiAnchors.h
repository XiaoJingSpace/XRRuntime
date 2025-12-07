//=============================================================================
//! \file sxrApiAnchors.h
//
//                  Copyright (c) 2020 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================

#ifndef _SXR_API_ANCHORS_H_
#define _SXR_API_ANCHORS_H_

#include "sxrApi.h"

#ifdef __cplusplus
extern "C" {
#endif

// 3D pose of an anchor

struct sxrAnchorUuid {
    uint8_t uuid[16];
};

struct sxrAnchorPose {
    sxrQuaternion orientation;
    sxrVector3    position;
    float         poseQuality;
};

struct sxrAnchorInfo {
    sxrAnchorUuid id;
    uint32_t      revision;
    sxrAnchorPose pose;
};

/*!
* Initialize Anchors module.
*
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_VRMODE_NOT_INITIALIZED if helper is null.
*    - #SXR_ERROR_UNSUPPORTED if Anchors is not supported.
*/
SXRP_EXPORT SxrResult sxrAnchorsInitialize();

/*!
* Shutdown Anchors module.
*
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_VRMODE_NOT_INITIALIZED if helper is null.
*    - #SXR_ERROR_NOT_INITIALIZED if Anchors is not initialized.
*/
SXRP_EXPORT SxrResult sxrAnchorsShutdown();

/*!
* Create an XrAnchorPose. The caller may call #Destroy to remove the anchor.
*
* \param[in] anchor pose.
* \param[out] anchor id.
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
*    - #SXR_ERROR_INVALID_OPERATION if error occurs.
*/
SXRP_EXPORT SxrResult sxrAnchorsCreate(sxrAnchorPose *anchorPose, sxrAnchorUuid *anchorId);

/*!
* Destroy anchor pose previously created via #Create.
*
* \param[in] anchor id.
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
*    - #SXR_ERROR_INVALID_OPERATION the anchor was not destroyed.
*/
SXRP_EXPORT SxrResult sxrAnchorsDestroy(sxrAnchorUuid& anchorId);

/*!
* Get anchors data. The caller must call #ReleaseData if the return code was
* #SXR_SUCCESS to unlock the anchors data.
*
* \param[in, out] numAnchors Number of anchors.
* \param[in, out] anchors Pointer that points to the anchors data.Caller doesn't take the
*                         ownership of the data.
* \return
*   - #SXR_ERROR_NONE upon success.
*   - #SXR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
*   - #SXR_ERROR_INVALID_OPERATION if error occurs.
*/
SXRP_EXPORT SxrResult sxrAnchorsGetData(uint32_t *numAnchors, sxrAnchorInfo **anchors);

/*!
* Release anchors data which previously locked via #GetData.
*
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
*    - #SXR_ERROR_INVALID_OPERATION the mesh was not locked.
*/
SXRP_EXPORT SxrResult sxrAnchorsReleaseData();

SXRP_EXPORT sxrAnchorPose sxrAnchorsGetPose(sxrAnchorPose anchor_pose, sxrMatrix conversion);

SXRP_EXPORT const char* sxrAnchorsToString(sxrAnchorUuid& anchorId);

/*!
* Saves the anchor into folder ./anchors with uuid as the filename.
*
* \param[in] anchor id.
* \param[in] file folder
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
*    - #SXR_ERROR_INVALID_OPERATION if error occurs.
*/
SXRP_EXPORT SxrResult sxrAnchorsSave(sxrAnchorUuid& anchorId, const char* fmapFolder);

/*!
* Loads all files in ./anchors into the relocator.
* Assumption that each file in there is an anchor and name is the anchor uuid
*
* \param[in] file folder
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
*    - #SXR_ERROR_INVALID_OPERATION if error occurs.
*/
SXRP_EXPORT SxrResult sxrAnchorsStartRelocating(const char* fmapFolder);

/*!
* Unloads all the anchors loaded in the startrelocating call
*
* \return
*    - #SXR_ERROR_NONE upon success.
*    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
*    - #SXR_ERROR_INVALID_OPERATION if error occurs.
*/
SXRP_EXPORT SxrResult sxrAnchorsStopRelocating();

#ifdef __cplusplus
}
#endif

#endif // _SXR_API_3DR_H_
