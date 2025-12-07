/******************************************************************************/
/*! \file  QVRAnchors.h */
/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

/*******************************************************************************************************************//**
* \file
*
* This API is considered BETA and is therefore subject to change and/or removal entirely. Use at your own risk!
*
***********************************************************************************************************************/

#ifndef QVRANCHORS_H
#define QVRANCHORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "QVRTypes.h"
#include "QXR.h"

#define CLASS_ID_ANCHORS_BETA_2 0xff000003

/*******************************************************************************************************************//**
* 3D pose of an anchor
***********************************************************************************************************************/
typedef struct XrAnchorPosefQTI {
    XrQuaternionfQTI orientation;
    XrVector3fQTI    position;
    float            poseQuality;
} XrAnchorPosefQTI;

/*******************************************************************************************************************//**
* Universally unique identifier for an anchor
***********************************************************************************************************************/
typedef struct XrAnchorUuidQTI {
    uint8_t uuid[16];
} XrAnchorUuidQTI;

/*******************************************************************************************************************//**
* A single anchor within an anchor data array
***********************************************************************************************************************/
typedef struct XrAnchorInfoQTI {
    XrAnchorUuidQTI  id;
    uint32_t         revision;
    XrAnchorPosefQTI pose;
} XrAnchorInfoQTI;

/*******************************************************************************************************************//**
* Anchor data
***********************************************************************************************************************/
typedef struct XrAnchorDataQTI {
    uint32_t        numAnchors;    //!< Number of valid anchors in the anchors array
    XrAnchorInfoQTI anchors[100];  //!< Anchors
} XrAnchorDataQTI;

/***************************************************************************************************
* \enum QVRANCHORS_API_VERSION
*
* Defines the API versions of this interface. The api_version member of the anchors structure must
* be set to accurately reflect the version of the API that the implementation supports.
***************************************************************************************************/
typedef enum {
    QVRANCHORS_API_VERSION_BETA_1 = 0xffff,
    QVRANCHORS_API_VERSION_BETA_2
} QVRANCHORS_API_VERSION;

typedef struct qvr_anchors_ops {
    int32_t (*CreateAnchor)(qvrservice_class_t *me, const XrAnchorPosefQTI* anchorPose, XrAnchorUuidQTI* anchorId);

    int32_t (*DestroyAnchor)(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId);

    int32_t (*GetAnchorData)(qvrservice_class_t *me, XrAnchorDataQTI** anchorData);

    int32_t (*ReleaseAnchorData)(qvrservice_class_t *me, XrAnchorDataQTI* anchorData);

    int32_t (*SaveSerializedAnchor)(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId, const int32_t fd, uint32_t *size);

    int32_t (*AddSerializedAnchor)(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId, const int32_t fd, const uint32_t size);

    int32_t (*RemoveSerializedAnchor)(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId);

    /* Reserved for future use */
    void* reserved[64];
} qvr_anchors_ops_t;

/***************************************************************************
* Create an anchor
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[in]
*    anchorPose: The position & orientation for the anchor.
* \param[out]
*    anchorId: If successful, the UUID of the new anchor.
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRAnchors_Create(qvrservice_class_t *me, const XrAnchorPosefQTI* anchorPose, XrAnchorUuidQTI* anchorId)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->CreateAnchor) return QVR_API_NOT_SUPPORTED;
    return ops->CreateAnchor(me, anchorPose, anchorId);
}

/***************************************************************************
* Destroy an anchor
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[in]
*    anchorId: The UUID of the anchor to destroy.
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRAnchors_Destroy(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->DestroyAnchor) return QVR_API_NOT_SUPPORTED;
    return ops->DestroyAnchor(me, anchorId);
}

/***************************************************************************
* Get updated data about all current anchors
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[out]
*    anchorData: pointer to be filled in with the XrAnchorDataQTI containing the data.
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    ReleaseAnchorData must be used to unlock the buffer once finished.
**************************************************************************/
static inline int32_t QVRAnchors_GetAnchorData(qvrservice_class_t *me, XrAnchorDataQTI** anchorData)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->GetAnchorData) return QVR_API_NOT_SUPPORTED;
    return ops->GetAnchorData(me, anchorData);
}

/***************************************************************************
* Release the buffer received by GetAnchorData
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[in]
*    anchorData: The buffer to release
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    This must be called after the client is finished with the data provided
*    by GetAnchorData.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRAnchors_ReleaseAnchorData(qvrservice_class_t *me, XrAnchorDataQTI* anchorData)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->ReleaseAnchorData) return QVR_API_NOT_SUPPORTED;
    return ops->ReleaseAnchorData(me, anchorData);
}

/***************************************************************************
* Save an anchor created by QVRAnchors_Create for offline storage
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[in]
*    anchorId: The UUID of the anchor to save.
* \param[in]
*    fd: A file descriptor to save the anchor to.  The FD must be open for writing.
* \param[out]
*    size: If successful, the size of the anchor data written to the FD, in bytes.
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The FD will be written to directly.  It will not be erased or deleted prior
*    to writing, so passing an empty FD is expected.  The FD, size and ID can be
*    provided to QVRAnchors_Add to load it in a different session.
**************************************************************************/
static inline int32_t QVRAnchors_Save(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId, const int32_t fd, uint32_t *size)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->SaveSerializedAnchor) return QVR_API_NOT_SUPPORTED;
    return ops->SaveSerializedAnchor(me, anchorId, fd, size);
}

/***************************************************************************
* Load one anchor from offline storage into the current session
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[in]
*    anchorId: The UUID of the anchor to load.
* \param[in]
*    fd: The file descriptor containing the data from QVRAnchors_Save.
* \param[in]
*    size: The size of the anchor data in bytes.
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    After calling this function, the anchor is added to the loading list.
*    As the device maps its surroundings during motion, the service will
*    search for matching data and attempt to load the anchor.
**************************************************************************/
static inline int32_t QVRAnchors_AddToSearch(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId, const int32_t fd, const uint32_t size)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->AddSerializedAnchor) return QVR_API_NOT_SUPPORTED;
    return ops->AddSerializedAnchor(me, anchorId, fd, size);
}

/***************************************************************************
* Remove a serialized anchor from the loading list
*
* \param[in]
*    me: qvrservice_class_t returned by QVRService_GetClassHandle().
* \param[in]
*    anchorId: The UUID of the anchor to stop loading.
* \return
*    0 upon success, error code otherwise
* \par API level
*    BETA 2
* \par Timing requirements
*    This should only be called when an anchor provided to QVRAnchors_Add is
*    still loading.
* \par Notes
*    After calling QVRAnchors_Add, the anchor is added to the loading list.
*    As the device maps its surroundings during motion, the service will
*    attempt to load the anchor.  If QVRAnchors_Remove is called before the
*    anchor is successfully loaded into the current map session, the anchor
*    loading process will be aborted for this anchor.  Calling this function
*    after the anchor is loaded will have no effect.
**************************************************************************/
static inline int32_t QVRAnchors_RemoveFromSearch(qvrservice_class_t *me, const XrAnchorUuidQTI* anchorId)
{
    qvr_anchors_ops_t *ops;
    if(!me || !me->ops) return QVR_INVALID_PARAM;
    if (me->api_version != QVRANCHORS_API_VERSION_BETA_2) return QVR_API_NOT_SUPPORTED;
    ops = (qvr_anchors_ops_t *)me->ops;
    if(!ops->RemoveSerializedAnchor) return QVR_API_NOT_SUPPORTED;
    return ops->RemoveSerializedAnchor(me, anchorId);
}

#ifdef __cplusplus
}
#endif

#endif // QVRANCHORS_H
