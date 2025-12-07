/**********************************************************************************************************************/
/*! \file  QVRCameraStream.h */
/*
* Copyright (c) 2021 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
***********************************************************************************************************************/
#ifndef QVRCAMERASTREAM_H
#define QVRCAMERASTREAM_H

/**
 * @addtogroup qvrstreams
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "QVRCameraClient.h"
#include "QVRTypes.h"
#include "QXR.h"

#define CLASS_ID_STREAMS_BETA     0xff000004

typedef void* qvrcamera_stream_handle_t;

/*******************************************************************************************************************//**
* \enum QVRSTREAMS_API_VERSION
*
* Defines the API versions of this interface. The api_version member of the qvrservice_class_t structure must be set to
* the version of the API that the implementation supports.
***********************************************************************************************************************/
typedef enum {
    QVRSTREAMS_API_VERSION_BETA_1 = 0xfff0,
} QVRSTREAMS_API_VERSION;

/**************************************************************************//**
* QVRCAMERA_STREAM_NOTIFICATION
* -----------------------------------------------------------------------------
*   CAMERA_STREAM_NOTIFICATION_STATE_CHANGED
*      The camera stream's state has changed. camera_stream_notification_callback_fn
*      payload will contain
*      qvrservice_camera_stream_state_change_notify_payload_t.
******************************************************************************/
typedef enum {
    CAMERA_STREAM_NOTIFICATION_STATE_CHANGED=0,
    CAMERA_STREAM_NOTIFICATION_MAX
} QVRCAMERA_STREAM_NOTIFICATION;

/**************************************************************************//**
* qvrservice_camera_stream_state_change_notify_payload_t
* -----------------------------------------------------------------------------
* This structure is used to pass the payload to
* camera_device_notification_callback_fn.
*   new_state     : new QVRCAMERA_CAMERA_STATUS
*   previous_state: previous QVRCAMERA_CAMERA_STATUS
******************************************************************************/
typedef struct {
    QVRCAMERA_CAMERA_STATUS new_state;
    QVRCAMERA_CAMERA_STATUS previous_state;
} qvrservice_camera_stream_state_change_notify_payload_t;

/**************************************************************************//**
* camera_stream_notification_callback_fn
* -----------------------------------------------------------------------------
* Callback for handling a camera stream status event
*    pCtx:    [in] The context passed in to SetClientStatusCallback()
*    state:   [in] The camera stream's new state
******************************************************************************/
typedef void (*camera_stream_notification_callback_fn)(void *pCtx,
        QVRCAMERA_STREAM_NOTIFICATION notification, void *payload,
        uint32_t payload_length);

/*******************************************************************************************************************//**
* Create a camera frame stream with the requested parameters.
*
* \param[in]
*    streamHandle   qvrcamera_device_helper_t returned by AttachCamera().
* \param[in]
*    uri            Stream properties in the form of "property=value&property=value ...".  Any property which is not
*                   specified will use the camera device's default value.
*                   Thus, an empty or NULL string is a request to create the default stream for the camera device.
*                   Valid values for property: format, res.
*                   Valid values for property values: as listed in header file QVRCameraDeviceParam.h.  For example,
*                   valid values for res would be QVR_CAMDEVICE_RESOLUTION_MODE_FULL and
*                   QVR_CAMDEVICE_RESOLUTION_MODE_FULL, corresponding to "full" and "quarter", respectively.
*
*                   A string to specify a stream with a depth stream at quarter resolution would be
*                   "format=depth16&res=quarter"
*
* \return
*    Returns qvrcamera_stream_handle_t upon success, NULL otherwise
* \par API level
*    8 or higher
***********************************************************************************************************************/
static inline qvrcamera_stream_handle_t QVRCameraDevice_CreateStream(
    qvrcamera_device_helper_t* helper,
    const char* uri)
{
    return QVRCameraDevice_GetClassHandle(helper, CLASS_ID_STREAMS_BETA, uri);
}

/**********************************************************************//**
* Destroy the camera stream
*
* \param[in]
*    helper   qvrcamera_device_helper_t returned by AttachCamera().
* \param[in]
*    stream   qvrcamera_stream_helper_t returned by CreateStream().
* \return
*    None
* \par API level
*    Beta 1
* \par Timing requirements
*    This function needs to be called when app is done with controlling
*    a given camera stream, or done accessing its frames.
*    This function will destroy the camera stream context. Therefore,
*    the same stream context can't be used in any other functions listed
*    below.
**************************************************************************/
static inline void QVRCameraDevice_DestroyStream(
    qvrcamera_device_helper_t* helper,
    qvrcamera_stream_handle_t  streamHandle)
{
    QVRCameraDevice_ReleaseClassHandle(helper, (qvrservice_class_t*)streamHandle);
}

typedef struct qvrcamera_stream_ops {
    int32_t (*Start)(qvrcamera_stream_handle_t streamHandle);

    int32_t (*Stop)(qvrcamera_stream_handle_t streamHandle);

    int32_t (*GetCurrentFrameNumber)(qvrcamera_stream_handle_t streamHandle, int32_t *pFn);

    int32_t (*GetFrame)(qvrcamera_stream_handle_t streamHandle, int32_t *pFn, QVRCAMERA_BLOCK_MODE block,
        QVRCAMERA_DROP_MODE drop, qvrcamera_frame_t* pFrame);

    int32_t (*ReleaseFrame)(qvrcamera_stream_handle_t streamHandle, int32_t fn);

    int32_t (*RegisterForNotification)(qvrcamera_stream_handle_t streamHandle,
        QVRCAMERA_STREAM_NOTIFICATION notification,
        camera_stream_notification_callback_fn cb, void *pCtx);

    int32_t (*GetStreamState)(qvrcamera_stream_handle_t streamHandle,
        QVRCAMERA_CAMERA_STATUS *pState);

    void* reserved[64 - 10];
} qvrcamera_stream_ops_t;

/**********************************************************************//**
* Start the stream of camera frames.  If the camera device is not running,
* it will also start.
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \return
*    returns QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    Beta 1
**************************************************************************/
static inline int32_t QVRCameraStream_Start(qvrcamera_stream_handle_t streamHandle)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->Start) return QVR_CAM_API_NOT_SUPPORTED;

    return ((qvrcamera_stream_ops_t*)p->ops)->Start(streamHandle);
}

/**********************************************************************//**
* Stop the stream of camera frames.  If there are no other clients of the
* camera device, the camera device will also stop.
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \return
*    returns QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    Beta 1
**************************************************************************/
static inline int32_t QVRCameraStream_Stop(qvrcamera_stream_handle_t streamHandle)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->Stop) return QVR_CAM_API_NOT_SUPPORTED;

    return ((qvrcamera_stream_ops_t*)p->ops)->Stop(streamHandle);
}

/**********************************************************************//**
* Retrieves current/latest frame number received from the attached stream.
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \param[in]
*    pFn             current/latest frame number received from requested stream.
* \return
*    returns QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    Beta 1
**************************************************************************/
static inline int32_t QVRCameraStream_GetCurrentFrameNumber(qvrcamera_stream_handle_t streamHandle, int32_t* pFn)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->GetCurrentFrameNumber) {
        return QVR_CAM_API_NOT_SUPPORTED;
    }

    return ((qvrcamera_stream_ops_t*)p->ops)->GetCurrentFrameNumber(streamHandle, pFn);
}

/**********************************************************************//**
* GetFrame allows the client to retrieve the camera frame metadata and
* buffer for the requested frame number.
* When GetFrame returns, the frame buffer is automatically locked and ready
* for use.
* ReleaseFrame must be invoked in order to release the frame buffer lock.
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \param[inout]
*    pFn             input:  requested frame number
*                    output: *pFn is updated with the actual
*                            returned frame number.
* \param[in]
*    block           refer to QVRCAMERA_BLOCK_MODE description
* \param[in]
*    drop            refer to QVRCAMERA_DROP_MODE description.
* \param[out]
*    pFrame          frame metadata and buffer pointer structure
* \return
*    returns QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    Beta 1
**************************************************************************/
static inline int32_t QVRCameraStream_GetFrame(
   qvrcamera_stream_handle_t streamHandle,
   int32_t *pFn,
   QVRCAMERA_BLOCK_MODE block,
   QVRCAMERA_DROP_MODE drop,
   qvrcamera_frame_t* pFrame)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->GetFrame) return QVR_CAM_API_NOT_SUPPORTED;

    return ((qvrcamera_stream_ops_t*)p->ops)->GetFrame(streamHandle, pFn, block, drop, pFrame);
}

/**********************************************************************//**
* Releases the lock on the frame buffer
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \param[in]
*    fn              frame number for which the frame buffer lock is
*                    to be released.
* \return
*    returns QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    8 or higher
**************************************************************************/
static inline int32_t QVRCameraStream_ReleaseFrame(qvrcamera_stream_handle_t streamHandle, int32_t fn)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->ReleaseFrame) return QVR_CAM_API_NOT_SUPPORTED;

    return ((qvrcamera_stream_ops_t*)p->ops)->ReleaseFrame(streamHandle, fn);
}

/***********************************************************************//**
* Register for event notifications for the camera stream.
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \param[in]
*    notification    QVRCAMERA_STREAM_NOTIFICATION to register for.
* \param[in]
*    cb              call back function of type camera_stream_notification_callback_fn.
* \param[in]
*    pCtx            Context to be passed to the callback function.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    Beta 1
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    The client will maintain only one callback, so subsequent calls to
*    this function will overwrite the previous callback. cb may be set to
*    NULL to disable notification callbacks.
***************************************************************************/
static inline int32_t QVRCameraStream_RegisterForNotification(
   qvrcamera_stream_handle_t streamHandle,
   QVRCAMERA_STREAM_NOTIFICATION notification,
   camera_stream_notification_callback_fn cb,
   void *pCtx)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->RegisterForNotification)
        return QVR_CAM_API_NOT_SUPPORTED;

    return ((qvrcamera_stream_ops_t*)p->ops)->RegisterForNotification(
        streamHandle, notification, cb, pCtx);
}

/**********************************************************************//**
* Get the current state of the camera stream.
*
* \param[in]
*    streamHandle    stream handle returned by GetClassHandle().
* \param[in]
*    pState          current stream state
* \return
*    returns QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    Beta 1
**************************************************************************/
static inline int32_t QVRCameraStream_GetStreamState(qvrcamera_stream_handle_t streamHandle, QVRCAMERA_CAMERA_STATUS *pState)
{
    if (!streamHandle) return QVR_CAM_INVALID_PARAM;
    qvrservice_class_t* p = static_cast<qvrservice_class_t*>(streamHandle);
    if (p->api_version < QVRSTREAMS_API_VERSION_BETA_1) return QVR_CAM_API_NOT_SUPPORTED;
    if (!((qvrcamera_stream_ops_t*)p->ops)->GetStreamState) return QVR_CAM_API_NOT_SUPPORTED;

    return ((qvrcamera_stream_ops_t*)p->ops)->GetStreamState(streamHandle, pState);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* QVRCAMERASTREAM_H */
