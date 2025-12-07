/******************************************************************************/
/*! \file  QVRCameraClient.h  */
/*
* Copyright (c) 2017-2021 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#ifndef QVRCAMERA_CLIENT_H
#define QVRCAMERA_CLIENT_H

/**
 * @addtogroup qvr_camera_client
 * @{
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <limits.h>

#ifdef __hexagon__
typedef struct AHardwareBuffer AHardwareBuffer;
#else
#include <android/hardware_buffer.h>
#endif

#include "QVRTypes.h"
#include "QVRCameraDeviceParam.h"
#include "QXR.h"

typedef void* qvrcamera_client_handle_t;
typedef void* qvrcamera_device_handle_t;

/******************************************************************************
* Camera device names.
******************************************************************************/
#define QVRSERVICE_CAMERA_NAME_DEPTH        "depth"
#define QVRSERVICE_CAMERA_NAME_TRACKING     "tracking"
#define QVRSERVICE_CAMERA_NAME_EYE_TRACKING "eye-tracking"
#define QVRSERVICE_CAMERA_NAME_RGB          "rgb"
#define QVRSERVICE_CAMERA_NAME_RGB_LEFT     "rgb-left"

/******************************************************************************
* Error codes.
******************************************************************************/
#define QVR_CAM_SUCCESS                     0
#define QVR_CAM_ERROR                      -1
#define QVR_CAM_CALLBACK_NOT_SUPPORTED     -2
#define QVR_CAM_API_NOT_SUPPORTED          -3
#define QVR_CAM_INVALID_PARAM              -4
#define QVR_CAM_EXPIRED_FRAMENUMBER        -5
#define QVR_CAM_FUTURE_FRAMENUMBER         -6
#define QVR_CAM_DROPPED_FRAMENUMBER        -7
#define QVR_CAM_DEVICE_NOT_STARTED         -8
#define QVR_CAM_DEVICE_STOPPING            -9
#define QVR_CAM_DEVICE_DETACHING           -10
#define QVR_CAM_DEVICE_ERROR               -11
#define QVR_CAM_SIZE_INSUFFICIENT          -12
#define QVR_CAM_PARTIAL_FRAME_NOT_ENABLED  -13

/**************************************************************************//**
* \enum QVRCAMERA_API_VERSION
*
* Defines the API versions of this interface. The api_version member of the
* qvr_camera_t object retrieved from the external camera library must
* be set to the API version that matches its functionality.
******************************************************************************/
typedef enum QVRCAMERA_API_VERSION {
    QVRCAMERACLIENT_API_VERSION_1 = 1,
    QVRCAMERACLIENT_API_VERSION_2,
    QVRCAMERACLIENT_API_VERSION_3,
    QVRCAMERACLIENT_API_VERSION_4,
    QVRCAMERACLIENT_API_VERSION_5,
    QVRCAMERACLIENT_API_VERSION_6,
    QVRCAMERACLIENT_API_VERSION_7,
    QVRCAMERACLIENT_API_VERSION_8,
} QVRCAMERA_API_VERSION;

/**************************************************************************//**
* \enum QVRCAMERA_FRAME_FORMAT
*
*   \var QVRCAMERA_FRAME_FORMAT_UNKNOWN
*      The frame format is unknown.
*   \var QVRCAMERA_FRAME_FORMAT_Y8
*      The frame format for 8-bit RAW monochrome frames.
*   \var QVRCAMERA_FRAME_FORMAT_YUV420
*      The frame format for RGB or non-RAW monochrome frames.
*   \var QVRCAMERA_FRAME_FORMAT_RAW10_MONO
*      The frame format for 10-bit RAW monochrome frames.
*   \var QVRCAMERA_FRAME_FORMAT_DEPTH16
*      The frame format for the Depth16 depth image format.
*   \var QVRCAMERA_FRAME_FORMAT_RAW_DEPTH
*      The frame format for raw depth values with no confidence bits.
*   \var QVRCAMERA_FRAME_FORMAT_RAW16_MONO
*      The frame format for 16-bit RAW monochrome frames.
******************************************************************************/
typedef enum QVRCAMERA_FRAME_FORMAT {
    QVRCAMERA_FRAME_FORMAT_UNKNOWN = 0,
    QVRCAMERA_FRAME_FORMAT_Y8,
    QVRCAMERA_FRAME_FORMAT_YUV420,
    QVRCAMERA_FRAME_FORMAT_RAW10_MONO,
    QVRCAMERA_FRAME_FORMAT_DEPTH16,
    QVRCAMERA_FRAME_FORMAT_RAW_DEPTH,
    QVRCAMERA_FRAME_FORMAT_RAW16_MONO,
    // add new formats above this line
    QVRCAMERA_FRAME_FORMAT_LAST,
    QVRCAMERA_FRAME_FORMAT_MAX_FORMATS = INT_MAX
} QVRCAMERA_FRAME_FORMAT;

/**************************************************************************//**
* \enum QVRCAMERA_FRAME_FORMAT_YUV420_ARRANGEMENT
*
*   \var QVRCAMERA_FRAME_FORMAT_NOT_YUV420
*      The frame format is in not YUV420.
*   \var QVRCAMERA_FRAME_FORMAT_YUV420_NV12
*      The frame format is YUV420 NV12/CBCR.
*   \var QVRCAMERA_FRAME_FORMAT_YUV420_NV21
*      The frame format is YUV420 NV21/CRCB.
******************************************************************************/
typedef enum QVRCAMERA_FRAME_FORMAT_YUV420_ARRANGEMENT {
    QVRCAMERA_FRAME_FORMAT_NOT_YUV420,
    QVRCAMERA_FRAME_FORMAT_YUV420_NV12,
    QVRCAMERA_FRAME_FORMAT_YUV420_NV21
} QVRCAMERA_FRAME_FORMAT_YUV420_ARRANGEMENT;

/**************************************************************************//**
* The frame metadata provided to each client as a result of QVRCameraDevice_GetFrame() calls.
*
* \note
* The frame layout/size may change dynamically between frames due to
* changes in the crop settings made via calls to QVRCameraDevice_SetCropRegion().
* There are 3 supported frame cropping modes:
*
* Crop-off/single-crop: frame is a single image.
* \li width/height: size of image
* \li secondary_width/height: not used and should be 0
* \li len: width * height
* \li stride: stride of image
* Dual-crop: frame is two images that are sequential in memory.
* \li width/height: size of first image
* \li secondary_width/height: size of second image
* \li len: width * height + secondary_width * secondary_height
* \li stride: width+secondary_width
******************************************************************************/
typedef struct qvrcamera_frame_t
{
    uint32_t fn;                   /**< Frame number. */
    uint64_t start_of_exposure_ts; /**< Frame timestamp in ns (kernel boottime clk) . */
    uint32_t exposure;             /**< Frame exposure time in ns. */
    volatile uint8_t* buffer;      /**< Frame data. */
    uint32_t len;                  /**< Frame length = width * height + secondary_width * secondary_height. */
    uint32_t width;                /**< Width of frame or crop-image1. */
    uint32_t height;               /**< Height of frame or crop-image1. */
    uint32_t secondary_width;      /**< Width of crop-image2 or 0 (no crop-image2). */
    uint32_t secondary_height;     /**< Height of crop-image2 or 0 (no crop-image2). */
    uint32_t gain;                 /**< Frame gain (available as of QVRCAMERACLIENT_API_VERSION_4). */
    uint32_t stride;               /**< Stride of frame (available as of QVRCAMERACLIENT_API_VERSION_5). */
    uint32_t format;               /**< Frame format of type QVRCAMERA_FRAME_FORMAT (available as of QVRCAMERACLIENT_API_VERSION_5). */
} qvrcamera_frame_t;

/**************************************************************************//**
* Associates an AHardwareBuffer with an offset that describes where, in the
* context of a larger image, this AHardwareBuffer sits.
*/
typedef struct _qvrcamera_hwbuffer_t
{
    AHardwareBuffer *buf;    /**< AHardwareBuffer pointer. */
    XrOffset2DiQTI   offset; /**< offset into the larger image. */
} qvrcamera_hwbuffer_t;

/**************************************************************************//**
* \enum QVRCAMERA_PARAM_NUM_TYPE
*
*   Types used with GetParamNum() and SetParamNum().
******************************************************************************/
typedef enum QVRCAMERA_PARAM_NUM_TYPE {
    QVRCAMERA_PARAM_NUM_TYPE_UINT8  = 0,    /**< parameter is of type uint8_t */
    QVRCAMERA_PARAM_NUM_TYPE_UINT16,        /**< parameter is of type uint16_t */
    QVRCAMERA_PARAM_NUM_TYPE_UINT32,        /**< parameter is of type uint32_t */
    QVRCAMERA_PARAM_NUM_TYPE_UINT64,        /**< parameter is of type uint64_t */
    QVRCAMERA_PARAM_NUM_TYPE_INT8,          /**< parameter is of type int8_t */
    QVRCAMERA_PARAM_NUM_TYPE_INT16,         /**< parameter is of type int16_t */
    QVRCAMERA_PARAM_NUM_TYPE_INT32,         /**< parameter is of type int32_t */
    QVRCAMERA_PARAM_NUM_TYPE_INT64,         /**< parameter is of type int64_t */
    QVRCAMERA_PARAM_NUM_TYPE_FLOAT,         /**< parameter is of type float */
    QVRCAMERA_PARAM_NUM_TYPE_FLOAT_VECT,    /**< float vectors are not supported, do not use */
    QVRCAMERA_PARAM_NUM_TYPE_MAX = 0xFF
} QVRCAMERA_PARAM_NUM_TYPE;

/**************************************************************************//**
* \enum QVRCAMERA_CLIENT_STATUS
*
*   \var QVRCAMERA_CLIENT_DISCONNECTED
*      The client was unexpectedly disconnected from server. If this occurs,
*      the QVRCameraClient object must be deleted.
*   \var QVRCAMERA_CLIENT_CONNECTED
*      The client is connected.
******************************************************************************/
typedef enum QVRCAMERA_CLIENT_STATUS {
    QVRCAMERA_CLIENT_DISCONNECTED = 0,
    QVRCAMERA_CLIENT_CONNECTED,
    QVRCAMERA_CLIENT_STATUS_MAX = 0xff
} QVRCAMERA_CLIENT_STATUS;

/**************************************************************************//**
* \enum QVRCAMERA_CAMERA_STATUS
*
*   \var QVRCAMERA_CAMERA_CLIENT_DISCONNECTED
*      Client has been disconnected from the server.
*      It is expected that the user would then call DetachCamera to release
*      the Camera Handle.
*   \var QVRCAMERA_CAMERA_INITIALISING
*      Camera Device is being initialized as soon as its first client attaches.
*      The state will automatically transition to
*      QVRCAMERA_CAMERA_READY once initialization is complete.
*   \var QVRCAMERA_CAMERA_READY
*      Camera Device is ready and can be started by its Master Client.
*   \var QVRCAMERA_CAMERA_STARTING
*      Camera Device is starting after having receive a start command from
*      its Master client. The state will automatically transition to
*      QVRCAMERA_CAMERA_STARTED once startup is complete.
*   \var QVRCAMERA_CAMERA_STARTED
*      Camera Device is started/active. While in this state, only the client
*      that started the Camera DeviceVR Camera can modify its state.
*   \var QVRCAMERA_CAMERA_STOPPING
*      Camera Device is stopping. The state will automatically transition to
*      QVRCAMERA_CAMERA_READY once complete.
*   \var QVRCAMERA_CAMERA_ERROR
*      Camera Device has encountered an error.
******************************************************************************/
typedef enum QVRCAMERA_CAMERA_STATUS {
    QVRCAMERA_CAMERA_CLIENT_DISCONNECTED,
    QVRCAMERA_CAMERA_INITIALISING,
    QVRCAMERA_CAMERA_READY,
    QVRCAMERA_CAMERA_STARTING,
    QVRCAMERA_CAMERA_STARTED,
    QVRCAMERA_CAMERA_STOPPING,
    QVRCAMERA_CAMERA_ERROR,
    QVRCAMERA_CAMERA_STATUS_MAX = 0xff
} QVRCAMERA_CAMERA_STATUS;

/**************************************************************************//**
* \enum QVRCAMERA_BLOCK_MODE
*
*   QVRCAMERA_BLOCK_MODE is an option offered when invoking
*   QVRCameraDevice_GetFrame().
*
*   \var QVRCAMERA_MODE_BLOCKING
*     QVRCameraDevice_GetFrame() will block until the requested frame number
*     is available. If the requested frame number is available,
*     QVRCameraDevice_GetFrame() returns immediately.
*
*   \var QVRCAMERA_MODE_NON_BLOCKING
*     QVRCameraDevice_GetFrame() will not block. If the requested frame number
*     is not available, QVRCameraDevice_GetFrame() returns immediately with
*     return code QVR_CAM_FUTURE_FRAMENUMBER.
*
*   \var QVRCAMERA_MODE_NON_BLOCKING_SYNC
*     Behavior is identical to QVRCAMERA_MODE_NON_BLOCKING but indicates
*     that the QVRCameraDevice_GetFrame() call is the "source" action for
*     the synchronization framework. When specified, the sync framework
*     will measure QVRCameraDevice_GetFrame() call frequency and adjust camera
*     timing to minimize latency. Calls must be made at regular intervals
*     (i.e. ~30/45/60 Hz). Note that this blocking mode will only be available
*     if the caller obtained a qvrsync_ctrl_t object using
*     QVRCameraDevice_GetSyncCtrl() with the source specified as
*     QVR_SYNC_SOURCE_CAMERA_FRAME_CLIENT_READ.
******************************************************************************/
typedef enum QVRCAMERA_BLOCK_MODE {
    QVRCAMERA_MODE_BLOCKING,
    QVRCAMERA_MODE_NON_BLOCKING,
    QVRCAMERA_MODE_NON_BLOCKING_SYNC,
} QVRCAMERA_BLOCK_MODE;

/**************************************************************************//**
* \enum QVRCAMERA_DROP_MODE
*
*   QVRCAMERA_DROP_MODE is an option offered when invoking
*   QVRCameraDevice_GetFrame().
*
*   \var QVRCAMERA_MODE_NEWER_IF_AVAILABLE
*     QVRCameraDevice_GetFrame() will attempt to return at least the
*     requested frame number or a newer if available. If the requested frame
*     number is in the future, QVRCameraDevice_GetFrame() either blocks or
*     returns with error code QVR_CAM_FUTURE_FRAMENUMBER depending on the
*     QVRCAMERA_BLOCK_MODE specified.
*
*   \var QVRCAMERA_MODE_EXPLICIT_FRAME_NUMBER
*     QVRCameraDevice_GetFrame() will attempt to return the requested frame
*     number. If frame number is too old, QVRCameraDevice_GetFrame() returns
*     with error code QVR_CAM_EXPIRED_FRAMENUMBER. If frame number is in the
*     future, QVRCameraDevice_GetFrame() either blocks or returns with error
*     code QVR_CAM_FUTURE_FRAMENUMBER depending on the QVRCAMERA_BLOCK_MODE
*     specified.
******************************************************************************/
typedef enum QVRCAMERA_DROP_MODE {
    QVRCAMERA_MODE_NEWER_IF_AVAILABLE,
    QVRCAMERA_MODE_EXPLICIT_FRAME_NUMBER
} QVRCAMERA_DROP_MODE;

/**************************************************************************//**
*
* Callback for handling a camera client status event.
* @param[in] pCtx   The context passed in to SetClientStatusCallback().
* @param[in] state  client's new state (CONNECTED or DISCONNECTED).
*
* \note
*       SetClientStatusCallback() is deprecated. Use
*       RegisterForClientNotification() instead.
******************************************************************************/
typedef void (*camera_client_status_callback_fn)(void *pCtx,
        QVRCAMERA_CLIENT_STATUS state);

/**************************************************************************//**
*
* Callback for handling a camera status event.
*
* @param[in] pCtx   The context passed in to SetCameraStatusCallback().
* @param[in] state  Camera new state.
*
* @note
*       SetCameraStatusCallback() is deprecated. Use
*       RegisterForCameraNotification() instead.
******************************************************************************/
typedef void (*camera_status_callback_fn)(void *pCtx,
        QVRCAMERA_CAMERA_STATUS state);

/**************************************************************************//**
* \enum QVRCAMERA_CLIENT_NOTIFICATION
*
*   \var CAMERA_CLIENT_NOTIFICATION_STATE_CHANGED
*      The client state has changed. camera_client_notification_callback_fn
*      payload will contain
*      qvrservice_camera_client_state_change_notify_payload_t.
******************************************************************************/
typedef enum QVRCAMERA_CLIENT_NOTIFICATION {
    CAMERA_CLIENT_NOTIFICATION_STATE_CHANGED=0,
    CAMERA_CLIENT_NOTIFICATION_MAX
} QVRCAMERA_CLIENT_NOTIFICATION;

/**************************************************************************//**
* This structure is used to pass the payload to
* camera_client_notification_callback_fn.
*
******************************************************************************/
typedef struct qvrservice_camera_client_state_change_notify_payload_t {
    QVRCAMERA_CLIENT_STATUS new_state; /**< New QVRCAMERA_CLIENT_STATUS. */
    QVRCAMERA_CLIENT_STATUS previous_state; /**< previous QVRCAMERA_CLIENT_STATUS. */
} qvrservice_camera_client_state_change_notify_payload_t;

/**************************************************************************//**
* Callback for handling camera client notifications.
* Callback is registered using RegisterForClientNotification().
* @param[in] pCtx            The context passed to RegisterForClientNotification().
* @param[in] notification    Notification value specifying the reason for the
*                            callback.
* @param[in] payload        Pointer to payload. Payload type depends on
*                            notification. Memory for the payload is allocated
*                            by qvr service client and will be released when
*                            callback returns.
* @param[in] payload_length  Length of valid data in payload.
******************************************************************************/
typedef void (*camera_client_notification_callback_fn)(void *pCtx,
        QVRCAMERA_CLIENT_NOTIFICATION notification, void *payload,
        uint32_t payload_length);

/**************************************************************************//**
* \enum QVRCAMERA_DEVICE_NOTIFICATION
*
*   \var CAMERA_DEVICE_NOTIFICATION_STATE_CHANGED
*      The camera state has changed. camera_device_notification_callback_fn
*      payload will contain
*      qvrservice_camera_device_state_change_notify_payload_t.
******************************************************************************/
typedef enum QVRCAMERA_DEVICE_NOTIFICATION {
    CAMERA_DEVICE_NOTIFICATION_STATE_CHANGED=0,
    CAMERA_DEVICE_NOTIFICATION_MAX
} QVRCAMERA_DEVICE_NOTIFICATION;

/**************************************************************************//**
* This structure is used to pass the payload to
* camera_device_notification_callback_fn.
*
******************************************************************************/
typedef struct qvrservice_camera_device_state_change_notify_payload_t {
    QVRCAMERA_CAMERA_STATUS new_state; /**< New QVRCAMERA_CAMERA_STATUS. */
    QVRCAMERA_CAMERA_STATUS previous_state; /**< Previous QVRCAMERA_CAMERA_STATUS. */
} qvrservice_camera_device_state_change_notify_payload_t;

/**************************************************************************//**
* \enum QVRCAMERA_FRAME_PARAM
*
*   Enum to capture all the per frame params that can be retrieved using GetFrameParamNum
*   API. These params supplement the fixed frame data that can be retrieved via GetFrame API.
*   Any future frame param types should be appended to this enum.
*
*   \var QVRCAMERA_FRAME_PARAM_U64_ROLLING_SHUTTER_SKEW_NS
*      Rolling shutter skew in ns for full sensor output dimension.
*   \var QVRCAMERA_FRAME_PARAM_U64_TARGET_ROLLING_SHUTTER_SKEW_NS
*      Scaled rolling shutter skew in ns to match target dimension reported in qvrcamera_frame_t,
*      target dimension is less than or equal to sensor output dimensions.
*   \var QVRCAMERA_FRAME_PARAM_I8_AUTO_EXPOSURE_ACTIVE
*      Indicates if auto exposure is active for the current frame
*   \var QVRCAMERA_FRAME_PARAM_U32_PEER_FRAME_NUMBER
*      Indicates matching peer frame number when pair sync is active
*   \var QVRCAMERA_FRAME_PARAM_U64_ILLUMINATION_DURATION_NS
*      The duration in ns for which the illumination was turned on for the current frame
*      If illumination was not used for this frame then this will be 0
*   \var QVRCAMERA_FRAME_PARAM_I32_YUV420_COLOR_ARRANGEMENT
*      Param to indicate if YUV420 arrangement is NV12 or NV21.
******************************************************************************/
typedef enum {
    QVRCAMERA_FRAME_PARAM_U64_ROLLING_SHUTTER_SKEW_NS,
    QVRCAMERA_FRAME_PARAM_U64_TARGET_ROLLING_SHUTTER_SKEW_NS,
    QVRCAMERA_FRAME_PARAM_I8_AUTO_EXPOSURE_ACTIVE,
    QVRCAMERA_FRAME_PARAM_I32_PEER_FRAME_NUMBER,
    QVRCAMERA_FRAME_PARAM_U64_ILLUMINATION_DURATION_NS,
    QVRCAMERA_FRAME_PARAM_I32_YUV420_COLOR_ARRANGEMENT
} QVRCAMERA_FRAME_PARAM;

/**************************************************************************//**
* Callback for handling a camera status event.
* @param[in] pCtx   The context passed in to SetClientStatusCallback().
* @param[in] state  Camera new state.
******************************************************************************/
typedef void (*camera_device_notification_callback_fn)(void *pCtx,
        QVRCAMERA_DEVICE_NOTIFICATION notification, void *payload,
        uint32_t payload_length);

typedef struct qvrcamera_client_ops {

    qvrcamera_client_handle_t (*Create)();

    void (*Destroy)(qvrcamera_client_handle_t client);

    int32_t (*SetClientStatusCallback)(qvrcamera_client_handle_t client,
        camera_client_status_callback_fn cb, void *pCtx);

    qvrcamera_device_handle_t (*AttachCamera)(qvrcamera_client_handle_t client,
            const char* pCameraName);

    qvrcamera_client_handle_t (*CreateWithKey)(const char* szKey);

    int32_t (*InitRingBufferData)(qvrcamera_client_handle_t client,
        QVRSERVICE_RING_BUFFER_ID id);

    int32_t (*WriteRingBufferData)(qvrcamera_client_handle_t client,
        QVRSERVICE_RING_BUFFER_ID id, void* pData, uint32_t len);

    int32_t (*GetParam)(qvrcamera_client_handle_t client, const char* pName,
          uint32_t* pLen, char* pValue);

    int32_t (*SetParam)(qvrcamera_client_handle_t client, const char* pName,
        const char* pValue);

    int32_t (*RegisterForClientNotification)(qvrcamera_client_handle_t client,
        QVRCAMERA_CLIENT_NOTIFICATION notification,
        camera_client_notification_callback_fn cb, void *pCtx);

    qvrcamera_device_handle_t (*AttachCameraWithParams)(
        qvrcamera_client_handle_t client, const char* pCameraName,
        qvr_plugin_param_t pParams[], int32_t nParams);

    int32_t (*EnumerateCameras)(qvrcamera_client_handle_t client, uint32_t camerasCapacityInput,
        uint32_t* camerasCountOutput, XrCameraPropertiesQTI* pCameras);

    //Reserved for future use
    void* reserved[64 - 8];

} qvrcamera_client_ops_t;


typedef struct qvrcamera_ops {

    void (*DetachCamera)(qvrcamera_device_handle_t camera);

    int32_t (*GetCameraState)(qvrcamera_device_handle_t camera,
        QVRCAMERA_CAMERA_STATUS *pState);

    int32_t (*SetCameraStatusCallback)(qvrcamera_device_handle_t camera,
        camera_status_callback_fn cb, void *pCtx);

    int32_t (*GetParam)(qvrcamera_device_handle_t camera, const char* pName,
        uint32_t* pLen, char* pValue);

    int32_t (*SetParam)(qvrcamera_device_handle_t camera, const char* pName,
        const char* pValue);

    int32_t (*GetParamNum)(qvrcamera_device_handle_t camera, const char* pName,
        QVRCAMERA_PARAM_NUM_TYPE type, uint8_t size, char* pValue);

    int32_t (*SetParamNum)(qvrcamera_device_handle_t camera, const char* pName,
        QVRCAMERA_PARAM_NUM_TYPE type, uint8_t size, const char* pValue);

    int32_t (*Start)(qvrcamera_device_handle_t camera);

    int32_t (*Stop)(qvrcamera_device_handle_t camera);

    int32_t (*GetCurrentFrameNumber)(qvrcamera_device_handle_t camera,
        int32_t *fn);

    int32_t (*SetExposureAndGain)(qvrcamera_device_handle_t camera,
        uint64_t exposure_ns, int iso_gain);

    int32_t (*GetFrame)(qvrcamera_device_handle_t camera,
        int32_t *fn, QVRCAMERA_BLOCK_MODE block,
        QVRCAMERA_DROP_MODE drop, qvrcamera_frame_t* pframe);

    int32_t (*ReleaseFrame)(qvrcamera_device_handle_t client, int32_t fn);

    int32_t (*SetCropRegion)(qvrcamera_device_handle_t client,
        uint32_t l_top, uint32_t l_left, uint32_t l_width, uint32_t l_height,
        uint32_t r_top, uint32_t r_left, uint32_t r_width, uint32_t r_height);

    int32_t (*RegisterForCameraNotification)(qvrcamera_device_handle_t camera,
        QVRCAMERA_DEVICE_NOTIFICATION notification,
        camera_device_notification_callback_fn cb, void *pCtx);

    int32_t (*SetGammaCorrectionValue)(qvrcamera_device_handle_t camera,
        float gamma_correction);

    // Compatibility block
    void* compat_block[2];

    int32_t (*GetProperties)(qvrcamera_device_handle_t camera,
        XrCameraDevicePropertiesQTI* pProperties);

    qvrsync_ctrl_t* (*GetSyncCtrl)(qvrcamera_device_handle_t camera,
        QVR_SYNC_SOURCE syncSrc);

    int32_t (*ReleaseSyncCtrl)(qvrcamera_device_handle_t camera,
        qvrsync_ctrl_t* pSyncCtrl);

    int32_t (*FrameToHardwareBuffer) (qvrcamera_device_handle_t camera,
       qvrcamera_frame_t *pFrame, uint32_t *pNumHwBufs, qvrcamera_hwbuffer_t **ppHwBuf);

    int32_t (*GetFrameParamNum)(qvrcamera_device_handle_t camera,
        qvrcamera_frame_t *pFrame, QVRCAMERA_FRAME_PARAM param,
        uint8_t size, void* pValue);

    qvrservice_class_t* (*GetClassHandle)(qvrcamera_device_handle_t camera,
        uint32_t classId, const char* uri);

    void (*ReleaseClassHandle)(qvrcamera_device_handle_t camera,
        qvrservice_class_t* handle);

    int32_t (*GetFrameEx)(qvrcamera_device_handle_t camera,
        const XrCameraFrameRequestInfoInputQTI * getFrameExInput,
        XrCameraFrameRequestInfoOutputQTI * getFrameExOutput);

    //Reserved for future use
    void* reserved[64 - 13];

} qvrcamera_ops_t;

typedef struct qvrcamera_client{
    QVRCAMERA_API_VERSION api_version;
    qvrcamera_client_ops_t* ops;
} qvrcamera_client_t;

typedef struct qvrcamera {
    QVRCAMERA_API_VERSION api_version;
    qvrcamera_ops_t* ops;
} qvrcamera_device_t;

qvrcamera_client_t* getQvrCameraClientInstance(void);
qvrcamera_device_t* getQvrCameraDeviceInstance(void);


// QVRCameraClient helper
#ifdef __hexagon__
#define QVRCAMERA_CLIENT_LIB "libqvr_cam_dsp_driver_skel.so"
#define QVRCAMERA_CLIENT_LIB_LEGACY "libqvr_cam_dsp_driver_skel.so"
#else
#define QVRCAMERA_CLIENT_LIB "libqvrcamera_client.qti.so"
#define QVRCAMERA_CLIENT_LIB_LEGACY "libqvrcamera_client.so"
#endif

typedef struct qvrcamera_client_helper_t {
    void* libHandle;
    qvrcamera_client_t* client;
    qvrcamera_client_handle_t clientHandle;
} qvrcamera_client_helper_t;

typedef struct qvrcamera_device_helper_t {
    qvrcamera_client_t* client;
    qvrcamera_device_t* cam;
    qvrcamera_device_handle_t cameraHandle;
} qvrcamera_device_helper_t;


/**************************************************************************//**
* Returns the API version of the camera client.
*
* @param[in] me  qvrcamera_device_helper_t* returned by AttachCamera()
*
* @return
*    QVRCAMERA_API_VERSION
* \par API level
*    1 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline QVRCAMERA_API_VERSION QVRCameraClient_GetVersion(
    qvrcamera_client_helper_t* me)
{
    return me->client->api_version;
}

/**************************************************************************//**
* Creates a new camera client.
*
* \return
*    qvrcamera_client_helper_t*
* \par API level
*    1 or higher.
* \par Timing requirements
*    This function needs to be called first before calling any other
*    functions listed below. The client context received by calling
*    this function needs to be passed to the other functions listed
*    below.
* \par Notes
*    None.
**************************************************************************/
static inline qvrcamera_client_helper_t* QVRCameraClient_Create()
{
    qvrcamera_client_helper_t* me = (qvrcamera_client_helper_t*)
                                    malloc(sizeof(qvrcamera_client_helper_t));
    if(!me) return NULL;

    typedef qvrcamera_client_t* (*qvrcamera_client_wrapper_fn)(void);
    qvrcamera_client_wrapper_fn qvrCameraClient;

    me->libHandle = NULL;
    qvrCameraClient = (qvrcamera_client_wrapper_fn)dlsym(RTLD_DEFAULT,
                                                "getQvrCameraClientInstance");
    if (!qvrCameraClient) {
        me->libHandle = dlopen(QVRCAMERA_CLIENT_LIB, RTLD_NOW);
        if (!me->libHandle) {
            me->libHandle = dlopen(QVRCAMERA_CLIENT_LIB_LEGACY, RTLD_NOW);
            if (!me->libHandle) {
                free(me);
                me = NULL;
                return NULL;
            }
        }

        qvrCameraClient = (qvrcamera_client_wrapper_fn)dlsym(me->libHandle,
                                                    "getQvrCameraClientInstance");
    }

    if (!qvrCameraClient) {
        if (me->libHandle != NULL) {
            dlclose(me->libHandle);
            me->libHandle = NULL;
        }
        free(me);
        me = NULL;
        return NULL;
    }

    me->client = qvrCameraClient();
    me->clientHandle = me->client->ops->Create();
    if(!me->clientHandle){
        if (me->libHandle != NULL) {
            dlclose(me->libHandle);
            me->libHandle = NULL;
        }
        free(me);
        me = NULL;
        return NULL;
    }

    return me;
}

/**************************************************************************//**
* Creates a camera client with the specified key.
*
* @return
*    qvrcamera_client_helper_t*
* \param[in] szKey  String used to uniquely identify this client. See
*                   notes for more info.
* \par API level
*    2 or higher
* \par Timing requirements
*    This function needs to be called first before calling any other
*    functions listed below. The client context received by calling
*    this function needs to be passed to the other functions listed
*    below.
* \par Notes
*    CreateWithKey() must be used when creating a camera client that will
*    be used from both the ARM and DSP -- both sides must call
*    CreateWithKey() with the same key. The key may be user-defined, or it
*    may come from the VR service itself (i.e. via the plugin interface),
*    in which case they key may have associated priviliges built into it,
*    and only clients created with such a key would be granted those
*    priviliges (i.e. starting/stopping a camera, changing exposure/gain
*    settings, etc.).
**************************************************************************/
static inline qvrcamera_client_helper_t* QVRCameraClient_CreateWithKey(
        const char* szKey)
{
    qvrcamera_client_helper_t* me = (qvrcamera_client_helper_t*)
                                    malloc(sizeof(qvrcamera_client_helper_t));
    if(!me) return NULL;

    typedef qvrcamera_client_t* (*qvrcamera_client_wrapper_fn)(void);
    qvrcamera_client_wrapper_fn qvrCameraClient;

    me->libHandle = NULL;
    qvrCameraClient = (qvrcamera_client_wrapper_fn)dlsym(RTLD_DEFAULT,
                                                "getQvrCameraClientInstance");
    if (!qvrCameraClient) {
        me->libHandle = dlopen(QVRCAMERA_CLIENT_LIB, RTLD_NOW);
        if (!me->libHandle) {
            me->libHandle = dlopen(QVRCAMERA_CLIENT_LIB_LEGACY, RTLD_NOW);
            if (!me->libHandle) {
                free(me);
                me = NULL;
                return NULL;
            }
        }

        qvrCameraClient = (qvrcamera_client_wrapper_fn)dlsym(me->libHandle,
                                                    "getQvrCameraClientInstance");
    }

    if (!qvrCameraClient) {
        if (me->libHandle != NULL) {
            dlclose(me->libHandle);
            me->libHandle = NULL;
        }
        free(me);
        me = NULL;
        return NULL;
    }

    me->client = qvrCameraClient();
    if (me->client->api_version >= QVRCAMERACLIENT_API_VERSION_2) {
        me->clientHandle = me->client->ops->CreateWithKey(szKey);
    } else {
        me->clientHandle = NULL;
    }
    if(!me->clientHandle){
        if (me->libHandle != NULL) {
            dlclose(me->libHandle);
            me->libHandle = NULL;
        }
        free(me);
        me = NULL;
        return NULL;
    }
    return me;
}

/**************************************************************************//**
* Destroys a camera client.
*
* @param[in] me  qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \return
*    None
* \par API level
*    1 or higher
* \par Timing requirements
*    This function needs to be called when app shuts down. This
*    function will destroy the client context therefore the same client
*    context can't be used in any other functions listed below.
* \par Notes
*    None
**************************************************************************/
static inline void QVRCameraClient_Destroy(qvrcamera_client_helper_t* me)
{
    if(!me) return;

    if(me->client->ops->Destroy){
        me->client->ops->Destroy( me->clientHandle);
    }

    if(me->libHandle ){
        dlclose(me->libHandle);
        me->libHandle = NULL;
    }
    free(me);
    me = NULL;
}

/**************************************************************************//**
* Registers/deregisters a callback function with the camera client.
*
* \deprecated This API is deprecated and is replaced by
*    QVRCameraClient_RegisterForClientNotification()
*
* @param[in] helper  qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* @param[in] cb      Callback function to be called to handle status events
* @param[in] pCtx    Context to be passed to callback function
*
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    This function may be called at any time. The client will maintain only
*    one callback, so subsequent calls to this function will overwrite any
*    previous callbacks set. cb may be set to NULL to disable status
*    callbacks.
* \par Notes
*    SetClientStatusCallback() is deprecated. Use
*    RegisterForClientNotification() instead.
**************************************************************************/
static inline int32_t QVRCameraClient_SetClientStatusCallback(
   qvrcamera_client_helper_t* helper, camera_client_status_callback_fn cb,
   void *pCtx)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->client->ops->SetClientStatusCallback)
        return QVR_CAM_API_NOT_SUPPORTED;

    return helper->client->ops->SetClientStatusCallback(
        helper->clientHandle, cb, pCtx);
}

/**********************************************************************//**
* Creates a camera device object for the named camera.
*
* @param[in] helper       qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* @param[in] pCameraName  name of the camera to attach to (QVRSERVICE_CAMERA_NAME_*)
*
* \return
*    qvrcamera_device_helper_t*
* \par API level
*    1 or higher
* \par Timing requirements
*    This function needs to be called when the app needs to atttach to
*    a specific camera. This function will return a
*    qvrcamera_device_helper_t* to further be used with the camera device
*    API.
* \par Notes
*    None
**************************************************************************/
static inline qvrcamera_device_helper_t* QVRCameraClient_AttachCamera(
    qvrcamera_client_helper_t* helper, const char* pCameraName)
{
    qvrcamera_device_helper_t* me = (qvrcamera_device_helper_t*)
        malloc(sizeof(qvrcamera_device_helper_t));
    if(!me) return NULL;

    me->client = helper->client;

    if(!helper->client->ops->AttachCamera) {
        free(me);
        me = NULL;
        return NULL;
    }

    me->cameraHandle = helper->client->ops->AttachCamera(
                                  helper->clientHandle, pCameraName);
    if (!me->cameraHandle) {
        free(me);
        me = NULL;
        return NULL;
    }

    typedef qvrcamera_device_t* (*qvrcamera_device_wrapper_fn)(void);
    qvrcamera_device_wrapper_fn qvrCameraDevice;

    qvrCameraDevice = (qvrcamera_device_wrapper_fn)dlsym(RTLD_DEFAULT,
        "getQvrCameraDeviceInstance");
    if (!qvrCameraDevice) {
        qvrCameraDevice = (qvrcamera_device_wrapper_fn)dlsym(helper->libHandle,
            "getQvrCameraDeviceInstance");
    }

    if (!qvrCameraDevice) {
        free(me);
        me = NULL;
        return NULL;
    }

    me->cam = qvrCameraDevice();

    return me;
}

/**********************************************************************//**
* Same as QVRCameraClient_AttachCamera but allows passing in parameters
* that may control how the camera device object is configured.
*
* \param[in]
*    helper:          qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \param[in]
*    pCameraName      name of the camera to attach to (QVRSERVICE_CAMERA_NAME_*)
* \param[in]
*    pParams          An array of qvr_plugin_param_t elements
* \param[in]
*    nParams          Number of elements in the params array
* \return
*    qvrcamera_device_handle_t
* \par API level
*    3 or higher
* \par Timing requirements
*    This function needs to be called when the app needs to atttach to
*    a specific camera. This function will return a
*    qvrcamera_device_handle_t to further be used for camera control API.
* \par Notes
*    None
**************************************************************************/
static inline qvrcamera_device_helper_t* QVRCameraClient_AttachCameraWithParams(
    qvrcamera_client_helper_t* helper, const char* pCameraName,
    qvr_plugin_param_t pParams[], int32_t nParams)
{
    qvrcamera_device_helper_t* me = (qvrcamera_device_helper_t*)
        malloc(sizeof(qvrcamera_device_helper_t));
    if(!me) return NULL;

    me->client = helper->client;

    if ((helper->client->api_version < QVRCAMERACLIENT_API_VERSION_3) ||
        (!helper->client->ops->AttachCameraWithParams)) {
        free(me);
        me = NULL;
        return NULL;
    }

    me->cameraHandle = helper->client->ops->AttachCameraWithParams(
        helper->clientHandle, pCameraName, pParams, nParams);
    if (!me->cameraHandle) {
        free(me);
        me = NULL;
        return NULL;
    }

    typedef qvrcamera_device_t* (*qvrcamera_device_wrapper_fn)(void);
    qvrcamera_device_wrapper_fn qvrCameraDevice;

    qvrCameraDevice = (qvrcamera_device_wrapper_fn)dlsym(RTLD_DEFAULT,
        "getQvrCameraDeviceInstance");
    if (!qvrCameraDevice) {
        qvrCameraDevice = (qvrcamera_device_wrapper_fn)dlsym(helper->libHandle,
            "getQvrCameraDeviceInstance");
    }

    if (!qvrCameraDevice) {
        free(me);
        me = NULL;
        return NULL;
    }

    me->cam = qvrCameraDevice();

    return me;
}

/**********************************************************************//**
* Initializes ring buffer data.
*
* \param[in]
*    helper       qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \param[in]
*    id           ID of the ring buffer to write to
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    This function will fail if pData is NULL or if the len parameter
*    does not match the structure size of the specified ring buffer ID.
**************************************************************************/
static inline int32_t QVRCameraClient_InitRingBufferData(
    qvrcamera_client_helper_t* helper, QVRSERVICE_RING_BUFFER_ID id)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->client->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->client->ops->InitRingBufferData) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->client->ops->InitRingBufferData(
        helper->clientHandle, id);
}

/**********************************************************************//**
* Writes ring buffer data.
*
* \param[in]
*    helper       qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \param[in]
*    id           ID of the ring buffer to write to
* \param[in]
*    pData        Pointer to data to write to the ring buffer
* \param[in]
*    len          Length of data pointed to by pData
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    This function will fail if pData is NULL or if the len parameter
*    does not match the structure size of the specified ring buffer ID.
*    Additionally, this call must not be made in between GetFrame() and
*    ReleaseFrame().
**************************************************************************/
static inline int32_t QVRCameraClient_WriteRingBufferData(
    qvrcamera_client_helper_t* helper, QVRSERVICE_RING_BUFFER_ID id,
    void* pData, uint32_t len)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->client->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->client->ops->WriteRingBufferData) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->client->ops->WriteRingBufferData(
        helper->clientHandle, id, pData, len);
}

/**********************************************************************//**
* Reads a string parameter value from the camera client.
*
* \param[in]
*    helper       qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \param[in]
*    pName    NULL-terminated name of the parameter length/value to
*                  retrieve. Must not be NULL.
* \param[in,out]
*    pLen         If pValue is NULL, pLen will be filled in with the
*                  number of bytes (including the NUL terminator) required
*                  to hold the value of the parameter specified by pName. If
*                  pValue is non-NULL, pLen must point to an integer that
*                  represents the length of the buffer pointed to by pValue.
*                  pLen must not be NULL.
* \param[out]
*    pValue       Buffer to receive value.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The pValue buffer will be filled in up to *pLen bytes (including NUL),
*    so this may result in truncation of the value if the required length
*    is larger than the size passed in pLen.
*    See QVRCameraDeviceParam.h for a list of available parameters.
**************************************************************************/
static inline int32_t QVRCameraClient_GetParam(
    qvrcamera_client_helper_t* helper, const char* pName, uint32_t* pLen,
    char* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->client->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->client->ops->GetParam) return QVR_CAM_API_NOT_SUPPORTED;
    return helper->client->ops->GetParam(
        helper->clientHandle, pName, pLen, pValue);
}

/**********************************************************************//**
* Writes a string parameter value to the camera client.
*
* \param[in]
*    helper    qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \param[in]
*    pName     NUL-terminated name of parameter value to set. Must not
*              be NULL.
* \param[in]
*    pValue    NUL-terminated value. Must not be NULL.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    See QVRCameraDeviceParam.h for a list of available parameters.
**************************************************************************/
static inline int32_t QVRCameraClient_SetParam(
    qvrcamera_client_helper_t* helper, const char* pName, const char* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->client->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->client->ops->SetParam) return QVR_CAM_API_NOT_SUPPORTED;
    return helper->client->ops->SetParam(
        helper->clientHandle, pName, pValue);
}

/***********************************************************************//**
* Registers/deregisters a callback function with the camera client.
*
* \param[in]
*    helper           qvrcamera_client_helper_t* returned by QVRCameraClient_Create()
* \param[in]
*    notification     QVRCAMERA_CLIENT_NOTIFICATION to register for
* \param[in]
*    cb               call back function of type notification_callback_fn
* \param[in]
*    pCtx             Context to be passed to callback function
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    The client will maintain only one callback, so subsequent calls to
*    this function will overwrite the previous callback. cb may be set to
*    NULL to disable notification callbacks.
***************************************************************************/
static inline int32_t QVRCameraClient_RegisterForClientNotification(
   qvrcamera_client_helper_t* helper,
   QVRCAMERA_CLIENT_NOTIFICATION notification,
   camera_client_notification_callback_fn cb,
   void *pCtx)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->client->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->client->ops->RegisterForClientNotification)
        return QVR_CAM_API_NOT_SUPPORTED;

    return helper->client->ops->RegisterForClientNotification(
        helper->clientHandle, notification, cb, pCtx);
}

/**********************************************************************//**
* Returns a list of the available cameras.
*
* \param[in]
*    helper               qvrcamera_client_helper_t returned by QVRCameraClient_Create().
* \param[in]
*    camerasCapacityInput The number of elements available in the pCameras
*                         array. If the call is a size request, can be 0.
* \param[out]
*    camerasCountOutput   Will be set to the number of cameras available.
*                         Must not be NULL.
* \param[out]
*    pCameras             A pointer to an array of XrCameraPropertiesQTI
*                         structs, where the number of elements is
*                         specified by the camerasCapacityInput parameter.
*                         Can be NULL if performing a size request.
* \return
*    QVR_CAM_SUCCESS upon success, an error code otherwise
* \par API level
*    8 or higher
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    See XrCameraPropertiesQTI for more info.
**************************************************************************/
static inline int32_t QVRCameraClient_EnumerateCameras(
    qvrcamera_client_helper_t* helper, uint32_t camerasCapacityInput,
    uint32_t* camerasCountOutput, XrCameraPropertiesQTI* pCameras)
{
    if (!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->client->api_version < QVRCAMERACLIENT_API_VERSION_8) return QVR_CAM_API_NOT_SUPPORTED;
    if (!helper->client->ops->EnumerateCameras) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->client->ops->EnumerateCameras(helper->clientHandle, camerasCapacityInput,
        camerasCountOutput, pCameras);
}

/**********************************************************************//**
* Returns the API version of the camera device.
*
* \param[in]
*    me         qvrcamera_device_helper_t* returned by AttachCamera()
* \return
*    QVRCAMERA_API_VERSION
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
**************************************************************************/
static inline QVRCAMERA_API_VERSION QVRCameraDevice_GetVersion(
    qvrcamera_device_helper_t* me)
{
    return me->cam->api_version;
}

// QVRCameraDevice helper

/**********************************************************************//**
* Detaches from a camera device and destroys the device object.
*
* \param[in]
*    helper         qvrcamera_device_helper_t* returned by AttachCamera()
* \return
*    None
* \par API level
*    1 or higher
* \par Timing requirements
*    This function needs to be called when app is done with controlling
*    a given camera, or done accessing its frames.
*    This function will destroy the camera context therefore
*    the same camera context can't be used in any other functions listed
*    below.
* \par Notes
*    None
**************************************************************************/
static inline void QVRCameraDevice_DetachCamera(
    qvrcamera_device_helper_t* helper)
{
    if(!helper) return;

    if(helper->cam->ops->DetachCamera){
        helper->cam->ops->DetachCamera( helper->cameraHandle);
    }

    free(helper);
    helper = NULL;
}

/**********************************************************************//**
* Can be used to retrieve the state of the camera device.
*
* \param[in]
*    helper          qvrcamera_device_helper_t* returned by AttachCamera()
* \param[out]
*    pState          current camera state
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRCameraDevice_GetCameraState(
    qvrcamera_device_helper_t* helper, QVRCAMERA_CAMERA_STATUS *pState)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->GetCameraState) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetCameraState(
        helper->cameraHandle, pState);
}

/**********************************************************************//**
* Registers/deregisters a callback function with the camera device.
*
* \deprecated This API is deprecated and is replaced by
*    QVRCameraDevice_RegisterForCameraNotification()
*
* \param[in]
*    helper         qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    cb             Callback function to be called to handle status events
* \param[in]
*    pCtx           Context to be passed to callback function
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    This function may be called at any time. The device will maintain only
*    one callback, so subsequent calls to this function will overwrite any
*    previous callbacks set. cb may be set to NULL to disable status
*    callbacks.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRCameraDevice_SetCameraStatusCallback(
    qvrcamera_device_helper_t* helper, camera_status_callback_fn cb, void *pCtx)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->SetCameraStatusCallback)
        return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->SetCameraStatusCallback(
        helper->cameraHandle, cb, pCtx);
}

/**********************************************************************//**
* Reads a string parameter value from the camera device.
*
* \param[in]
*    helper        qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pName         NULL-terminated name of the parameter length/value to
*                  retrieve. Must not be NULL.
* \param[in,out]
*    pLen          If pValue is NULL, pLen will be filled in with the
*                  number of bytes (including the NUL terminator) required
*                  to hold the value of the parameter specified by pName. If
*                  pValue is non-NULL, pLen must point to an integer that
*                  represents the length of the buffer pointed to by pValue.
*                  pLen must not be NULL.
* \param[out]
*    pValue        Buffer to receive value.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The pValue buffer will be filled in up to *pLen bytes (including NUL),
*    so this may result in truncation of the value if the required length
*    is larger than the size passed in pLen.
*    See QVRCameraDeviceParam.h for a list of available parameters.
**************************************************************************/
static inline int32_t QVRCameraDevice_GetParam(
    qvrcamera_device_helper_t* helper, const char* pName, uint32_t* pLen,
    char* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->GetParam) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetParam(
        helper->cameraHandle, pName, pLen, pValue);
}

/**********************************************************************//**
* Writes a string parameter value to the camera device.
*
* \param[in]
*    helper         qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pName          NULL-terminated name of parameter value to set. Must not
*                   be NULL.
* \param[in]
*    pValue         NUL-terminated value. Must not be NULL.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    See QVRCameraDeviceParam.h for a list of available parameters.
**************************************************************************/
static inline int32_t QVRCameraDevice_SetParam(
    qvrcamera_device_helper_t* helper, const char* pName, const char* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->SetParam) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->SetParam(
        helper->cameraHandle, pName, pValue);
}

/**********************************************************************//**
* Reads a numeric parameter value from the camera device.
*
* \param[in]
*    helper        qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pName         NULL-terminated name of the parameter length/value to
*                  retrieve. Must not be NULL.
* \param[in]
*    type          type of the parameter requested.
* \param[in]
*    size          size of the numerical type which pointer is passed in
*                  pValue, this size must be equal to the parameter's type
*                  size.
* \param[out]
*    pValue        Buffer to receive value.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The pValue buffer will be filled in up to size bytes.
*    See QVRCameraDeviceParam.h for a list of available parameters.
**************************************************************************/
static inline int32_t QVRCameraDevice_GetParamNum(
    qvrcamera_device_helper_t* helper, const char* pName,
    QVRCAMERA_PARAM_NUM_TYPE type, uint32_t size, char* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->GetParamNum) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetParamNum(
        helper->cameraHandle, pName, type, size, pValue);
}

/**********************************************************************//**
* Writes a numeric parameter value to the camera device.
*
* \param[in]
*    helper       qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pName        NUL-terminated name of the parameter length/value to
*                 set. Must not be NULL.
* \param[in]
*    type         type of the parameter requested.
* \param[in]
*    size         size of the numerical type which pointer is passed in
*                 pValue, this size must be equal to the parameter's type
*                 size.
* \param[in]
*    pValue       Buffer holding the parameter's value to be set.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    See QVRCameraDeviceParam.h for a list of available parameters.
**************************************************************************/
static inline int32_t QVRCameraDevice_SetParamNum(
    qvrcamera_device_helper_t* helper, const char* pName,
    QVRCAMERA_PARAM_NUM_TYPE type, uint32_t size, char* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->SetParamNum) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->SetParamNum(
        helper->cameraHandle, pName, type, size, pValue);
}

/**********************************************************************//**
* Start the camera.
*
* \param[in]
*    helper         qvrcamera_device_helper_t* returned by AttachCamera()
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    This function may fail if the client associated with this device does
*    not have "master" privileges. In addition, if QRV_CAM_SUCCESS is
*    returned, it does not guarantee that the camera has begun delivering
*    frames, only that the request to start the camera was successful.
**************************************************************************/
static inline int32_t QVRCameraDevice_Start(qvrcamera_device_helper_t* helper)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->Start) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->Start(
        helper->cameraHandle);
}

/**********************************************************************//**
* Stop the camera.
*
* \param[in]
*    helper         qvrcamera_device_helper_t* returned by AttachCamera()
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    This function can only succeed if called after the camera has been
*    started
* \par Notes
*    This function may fail if the client associated with this device does
*    not have "master" privileges. In addition, if QRV_CAM_SUCCESS is
*    returned, it does not guarantee that the camera has stopped delivering
*    frames, only that the request to stop the camera was successful.
**************************************************************************/
static inline int32_t QVRCameraDevice_Stop(qvrcamera_device_helper_t* helper)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->Stop) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->Stop(
        helper->cameraHandle);
}

/**********************************************************************//**
* Apply exposure and gain settings to the camera.
*
* \param[in]
*    helper            qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    exposure_ns       exposure duration in nanoseconds to be applied
*                      to the attached camera.
* \param[in]
*    iso_gain          iso gain to be applied to the attached camera.
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    These exposure and gain settings are applied immediately after the
*    next start of frame, and will take effect a few frames later
*    depending on the ISP exposure pipeline (up to 6 frames).
**************************************************************************/
static inline int32_t QVRCameraDevice_SetExposureAndGain(
    qvrcamera_device_helper_t* helper, uint64_t exposure_ns, int iso_gain)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->SetExposureAndGain) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->SetExposureAndGain(
        helper->cameraHandle,exposure_ns,iso_gain);
}

/**********************************************************************//**
* Retrieves current / latest frame number received from the camera.
*
* \param[in]
*    helper         qvrcamera_device_helper_t* returned by AttachCamera()
* \param[out]
*    fn             current / latest frame number received
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRCameraDevice_GetCurrentFrameNumber(
    qvrcamera_device_helper_t* helper, int32_t* fn)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->GetCurrentFrameNumber)
        return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetCurrentFrameNumber(
        helper->cameraHandle, fn);
}

/**********************************************************************//**
* Allows the client to retrieve the camera frame metadata and buffer by
* frame number. When QVRCameraDevice_GetFrame() returns, the frame buffer is
* automatically locked and ready for use.
*
* \param[in]
*    helper            qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in,out]
*    fn                input: requested frame number
*                      output: *fn is updated with the actual
*                      returned frame number.
* \param[in]
*    block             refer to QVRCAMERA_BLOCK_MODE description
* \param[in]
*    drop              refer to QVRCAMERA_DROP_MODE description
* \param[out]
*    pframe            frame metadata and buffer pointer structure
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    QVRCameraDevice_ReleaseFrame() must be invoked in order to unlock the
*    frame buffer.
*
*    Buffer Layout Information
*    -------------------------
*    The qvrcamera_frame_t struct returned by GetFrame() provides a single
*    buffer that contains images from a logical camera (which may be comprised
*    of one or more physical cameras), or two cropped regions from within that
*    buffer.
*
*    When more than one phyiscal camera comprises the logical camera, the
*    the individual camera images are merged. This merging can be done in
*    hardware (e.g. via a MIPI combiner) or software, or possibly even both.
*    But GetFrame() always provides a merged image by default. For example,
*    assuming two 1280 x 720 cameras (CAM0 & CAM1), the full (non-cropped)
*    image returned from GetFrame() would be 2560x720 with the two images
*    arranged horizontally:
*
*        +------------------------------+------------------------------+
*        |                              |                              |
*        |                              |                              |
*        |                              |                              |
*        |             CAM0             |              CAM1            |
*        |                              |                              |
*        |                              |                              |
*        |                              |                              |
*        +------------------------------+------------------------------+
*
*    For backwards compatibilty reasons, the horizontal arrangement is the
*    default for merged images returned by GetFrame().
*
*    In some cases, the camera images may be arranged vertically. Continuing
*    the previous example, in this arrangement the image returned from
*    GetFrame() would be 1280 x 1440 with the two images stacked vertically:
*
*        +------------------------------+
*        |                              |
*        |                              |
*        |                              |
*        |             CAM0             |
*        |                              |
*        |                              |
*        |                              |
*        +------------------------------+
*        |                              |
*        |                              |
*        |                              |
*        |             CAM1             |
*        |                              |
*        |                              |
*        |                              |
*        +------------------------------+
*
*    A client can use QVRCameraDevice_GetParam() with
*    ::QVR_CAMDEVICE_UINT32_IMAGE_COUNT to get the number of physical camera
*    images contained within the GetFrame() image. An image count greater than
*    1 implies merging will occur. A client can also get the arrangement of
*    the merged images by using QVRCameraDevice_GetParam() with
*    ::QVR_CAMDEVICE_STRING_IMAGE_ARRANGEMENT.
*
*       ### A brief side-note on cropping ###
*
*       When crop regions are specified via QVRCameraDevice_SetCropRegion(),
*       the arrangement of the cropped images in memory are also stacked
*       vertically:
*
*           +----------------+
*           |                |
*           |     CROP 0     |
*           |                |
*           +-------------+--+
*           |             |
*           |    CROP 1   |
*           |             |
*           +-------------+
*
*       - Crop region 0:
*           + Begins at pframe.buffer.
*           + Dimensions: pframe.width x pframe.height
*
*       - Crop region 1:
*           + Begins at pframe.buffer + offset, where offset is equal to the
*             number of bytes in crop region 0.
*           + Dimensions: pframe.secondary_width x pframe.secondary_height
*
*    How Frame Format Affects Buffer Layout
*    --------------------------------------
*    When clients attach to a camera using QVRCameraClient_AttachCamera(),
*    GetFrame()  will return images via in its default format. If a client
*    wishes to receive images in a different format, it may use
*    QVRCameraClient_AttachCameraWithParams() and provide the
*    ::QVR_CAMCLIENT_ATTACH_STRING_PREF_FORMAT setting. See
*    ::QVR_CAMCLIENT_ATTACH_STRING_PREF_FORMAT for more info.
*
*    Once a client attaches to a camera device, it may use
*    QVRCameraDevice_GetParam() with QVR_CAMDEVICE_STRING_FRAME_FORMAT to learn
*    which format the images will be delivered in. This is useful to know the
*    format before calling GetFrame() (the format is also provided in the
*    qvrcamera_frame_t struct).
*
*    When multiple physical camera images are merged, the arrangement of the
*    merged images into the single image will depend on the format of the frames
*    received by this particular camera device. For example, a camera with
*    frames delivered in Y8 as its default may deliver horizontally-merged
*    images, but attaching to the same camera in a RAW format may deliver
*    vertically-stacked images.
*
*    GetFrame() vs. FrameToHardwareBuffer()
*    --------------------------------------
*    The QVRCameraDevice_FrameToHardwareBuffer() API was introduced in API level
*    6, which gives clients an alternate way of accessing the image(s)
*    associated with a camera frame.
*       - In the case of a single camera, or hardware-merged cameras,
*         QVRCameraDevice_FrameToHardwareBuffer() will return a single
*         AHardwareBuffer (AHB) whose underlying buffer would point to the same
*         image as the qvrcamera_frame_t's 'buffer' member.
*       - In the case of software-merged cameras,
*         QVRCameraDevice_FrameToHardwareBuffer() returns AHBs for each of the
*         individual physical camera images.
*
*    Optimizing Performance
*    ----------------------
*    The default behavior for GetFrame() is to return an image in the
*    qvrcamera_frame_t struct. For systems where multiple physical cameras feed
*    a single logical camera, software-merging will be enabled. But if a client
*    can consume the images as individual AHBs using
*    QVRCameraDevice_FrameToHardwareBuffer(), then the client has no need for
*    the (possibly merged) image in the qvrcamera_frame_t struct. If the system
*    can avoid providing the image in the qvrcamera_frame_t struct, then it may
*    also be able to disable software-merging, which can offer both latency and
*    power benefits.
*
*    Clients can indicate that they do not need the images provided by
*    GetFrame()'s qvrcamera_frame_t struct by using QVRCameraDevice_SetParam()
*    with ::QVR_CAMDEVICE_STRING_GET_FRAME_IMG_DISABLE. If the system determines
*    that no clients of a particular image format require the images offered by
*    GetFrame(), the system may disable image merging, if applicable. Note that
*    all clients of the same camera (and frame format) must indicate that they
*    do not need the images provided by GetFrame() before the GetFrame() images
*    can be disabled. Also note that the system may not disable GetFrame()
*    images if it determines that doing so would offer no benefit.
*
*    If GetFrame() images are disabled, the .buffer member of the
*    qvrcamera_frame_t struct will be NULL, and other members that describe the
*    buffer contents (e.g. dimensions, stride, etc.) will be set to 0. Clients
*    are expected to only use QVRCameraDevice_FrameToHardwareBuffer() and
*    AHardwareBuffer-related APIs to get access to the image and other buffer
*    information. See QVRCameraDevice_FrameToHardwareBuffer() for more info.
*
*    YUV420 format chroma channel arrangement
*    ----------------------------------------
*    When calling QVRCameraDevice_GetFrame() on YUV420 camera, the chroma channel
*    arrangement can either be NV12 or NV21 chroma channel, obtained arrangement by
*    calling QVRCameraDevice_GetFrameParamNum() with
*    QVRCAMERA_FRAME_PARAM_I32_YUV420_COLOR_ARRANGEMENT.
*
**************************************************************************/
static inline int32_t QVRCameraDevice_GetFrame(
    qvrcamera_device_helper_t* helper,
    int32_t *fn,
    QVRCAMERA_BLOCK_MODE block,
    QVRCAMERA_DROP_MODE drop,
    qvrcamera_frame_t* pframe)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->GetFrame) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetFrame(
        helper->cameraHandle,fn,block,drop,pframe);
}

/**********************************************************************//**
* Releases the lock on the frame buffer.
*
* @param[in] helper  qvrcamera_device_helper_t* returned by AttachCamera()
* @param[in] fn      frame number for which the frame buffer lock is to be released
*
* @return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    1 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRCameraDevice_ReleaseFrame(
    qvrcamera_device_helper_t* helper,
    int32_t fn)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if(!helper->cam->ops->ReleaseFrame) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->ReleaseFrame(
        helper->cameraHandle,fn);
}

/**********************************************************************//**
* Sets the crop region on the camera.
*
* \param[in]
*    helper            qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    l_top             Specifies the top (left) corner of the crop region in
*                      pixels. If the crop region starts at the first pixel,
*                      this value should be zero.
* \param[in]
*    l_left            Specifies the left (top) corner of the crop region in
*                      pixels. If the crop region starts at the first pixel,
*                      this value should be zero.
* \param[in]
*    l_width           Specifies the width of the crop region in pixels
* \param[in]
*    l_height          Specifies the height of the crop region in pixels
* \param[in]
*    r_top             Specifies the top (left) corner of the crop region in
*                      pixels. If the crop region starts at the first pixel,
*                      this value should be zero.
* \param[in]
*    r_left            Specifies the left (top) corner of the crop region in
*                      pixels. If the crop region starts at the first pixel,
*                      this value should be zero.
* \param[in]
*    r_width           Specifies the width of the crop region in pixels
* \param[in]
*    r_height          Specifies the height of the crop region in pixels
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRCameraDevice_SetCropRegion(
    qvrcamera_device_helper_t* helper,
    uint32_t l_top,
    uint32_t l_left,
    uint32_t l_width,
    uint32_t l_height,
    uint32_t r_top,
    uint32_t r_left,
    uint32_t r_width,
    uint32_t r_height)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->SetCropRegion) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->SetCropRegion(
        helper->cameraHandle, l_top, l_left, l_width, l_height,r_top, r_left, r_width, r_height);
}

/**********************************************************************//**
* Apply gamma correction value to the attached camera.
*
* \param[in]
*    helper            qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    gamma             gamma correction value to be applied
* \return
*    QVR_CAM_SUCCESS upon success, QVR_CAM_ERROR otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    This gamma correction setting is applied immediately after the next
*    start of frame, and will take effect immediately.
**************************************************************************/
static inline int32_t QVRCameraDevice_SetGammaCorrectionValue(
    qvrcamera_device_helper_t* helper, float gamma)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->SetGammaCorrectionValue) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->SetGammaCorrectionValue(
        helper->cameraHandle,gamma);
}

/***********************************************************************//**
* Registers/deregisters a callback function with the camera device.
*
* \param[in]
*    helper             qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    notification       QVRCAMERA_DEVICE_NOTIFICATION to register for.
* \param[in]
*    cb                 callback function of type notification_callback_fn.
* \param[in]
*    pCtx               Context to be passed to callback function.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    The device will maintain only one callback, so subsequent calls to
*    this function will overwrite the previous callback. cb may be set to
*    NULL to disable notification callbacks.
***************************************************************************/
static inline int32_t QVRCameraDevice_RegisterForCameraNotification(
    qvrcamera_device_helper_t* helper,
    QVRCAMERA_DEVICE_NOTIFICATION notification,
    camera_device_notification_callback_fn cb,
    void *pCtx)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_2) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->RegisterForCameraNotification) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->RegisterForCameraNotification(
        helper->cameraHandle, notification, cb, pCtx);
}

/**********************************************************************//**
* Returns the properties of the camera.
*
* \param[in]
*    helper      qvrcamera_device_helper_t returned by AttachCamera().
* \param[out]
*    pProperties  pointer to XrCameraDevicePropertiesQTI struct to be filled in.
* \return
*    0 upon success, error code otherwise.
* \par API level
*    6 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRCameraDevice_GetProperties(
    qvrcamera_device_helper_t* helper,
    XrCameraDevicePropertiesQTI* pProperties)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_6) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->GetProperties) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetProperties(
        helper->cameraHandle, pProperties);
}

/**********************************************************************//**
* Acquire the Sync Ctrl.
*
* \param[in]
*    helper   qvrcamera_device_helper_t returned by AttachCamera().
* \param[in]
*    syncSrc  Sync source for which to obtain a sync control.
*             See QVR_SYNC_SOURCE for list of valid sources.
* \return
*    Returns a qvrsync_ctrl_t* if successful, NULL otherwise.
* \par API level
*    6 or higher.
* \par Timing requirements
*    This function may be called at any time.
***************************************************************************/
static inline qvrsync_ctrl_t* QVRCameraDevice_GetSyncCtrl(
    qvrcamera_device_helper_t* helper, QVR_SYNC_SOURCE syncSrc)
{
    if(!helper) return NULL;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_6) return NULL;
    if(!helper->cam->ops->GetSyncCtrl) return NULL;

    return helper->cam->ops->GetSyncCtrl(
        helper->cameraHandle, syncSrc);
}

/**********************************************************************//**
* Release the Sync Ctrl.
*
* \param[in]
*    helper     qvrcamera_device_helper_t returned by AttachCamera().
* \param[in]
*    pSyncCtrl  qvrsync_ctrl_t* returned by QVRCameraDevice_GetSyncCtrl().
* \return
*    Returns 0 upon success, -1 otherwise.
* \par API level
*    6 or higher.
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    None.
***************************************************************************/
static inline int32_t QVRCameraDevice_ReleaseSyncCtrl(
    qvrcamera_device_helper_t* helper, qvrsync_ctrl_t* pSyncCtrl)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_6) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->ReleaseSyncCtrl) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->ReleaseSyncCtrl(
        helper->cameraHandle, pSyncCtrl);
}

/**********************************************************************//**
* Returns the associated AHardwareBuffer(s) for the given frame.
*
* \param[in]
*    helper     qvrcamera_device_helper_t returned by AttachCamera().
* \param[in]
*    pFrame     pointer to frame.
* \param[out]
*    pNumHwBufs number of hardware buffers in the ppHwBuf array.
* \param[out]
*    ppHwBuf    array of qvrcamera_hwbuffer_t elements.
* \return
*    Returns 0 upon success, -1 otherwise.
* \par API level
*    6 or higher.
* \par Timing requirements
*    This function must be called after calling GetFrame() and before
*    calling ReleaseFrame() on the given frame.
* \par Notes
*    The associated AHardwareBuffer(s) for the frame will become invalid once
*    ReleaseFrame() is called on the given frame.
*
*    Unlike QVRCameraDevice_GetFrame(), which returns a single image,
*    FrameToHardwareBuffer() may be used to get the individual camera image(s)
*    that comprise the single image returned by QVRCameraDevice_GetFrame().
*
*    For example, assuming two 1280x720 cameras (CAM0 & CAM1) that use a
*    MIPI combiner, the full-size (non-cropped) image returned from
*    QVRCameraDevice_GetFrame() would be 2560x720, and
*    FrameToHardwareBuffer() will return the following:
*
*        *pNumHwBufs = 1
*         ppHwBuf[0]->offset = 0,0
*         ppHwBuf[0]->buf    = AHardwareBuffer for the same image returned
*                              by GetFrame(). Dimensions = 2560x720.
*
*    Alternatively, if the two cameras deliver images individually, their images
*    may be software-merged in order to deliver the combined image via
*    QVRCameraDevice_GetFrame(). In that case, the combined image will be
*    identical to the above example, but FrameToHardwareBuffer() will return the
*    following:
*
*        *pNumHwBufs = 2
*         ppHwBuf[0]->offset = 0,0
*         ppHwBuf[0]->buf    = AHardwareBuffer for CAM0 image. Dimensions = 1280x720.
*         ppHwBuf[1]->offset = 1280,0
*         ppHwBuf[2]->buf    = AHardwareBuffer for CAM1 image. Dimensions = 1280x720.
*
*    The availability of mulitple hardware buffers will depend on the format
*    of the frames that are delivered by the physical camera(s) and the format
*    that the client is receiving. For example, if the camera delivers RAW
*    frames natively, but the client is receiving Y8 frames, then multiple
*    hardware buffers may be available for the RAW format, but only a single
*    hardware buffer may be available for the Y8 format.
*
*    See ::QVR_CAMCLIENT_ATTACH_STRING_PREF_FORMAT for selecting which format to
*    receive, and see QVRCameraDevice_GetFrame() for more info.
*
**************************************************************************/
static inline int32_t QVRCameraDevice_FrameToHardwareBuffer(
    qvrcamera_device_helper_t* helper, qvrcamera_frame_t *pFrame,
    uint32_t *pNumHwBufs, qvrcamera_hwbuffer_t **ppHwBuf)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_6) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->FrameToHardwareBuffer) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->FrameToHardwareBuffer(
        helper->cameraHandle, pFrame, pNumHwBufs, ppHwBuf);
}

/**********************************************************************//**
* Reads a numeric parameter value from the camera frame.
*
* \param[in]
*    helper     qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pFrame     pointer to frame.
* \param[in]
*    param      QVRCAMERA_FRAME_PARAM to specify the parameter value to
*               retrieve.
* \param[in]
*    size       size in bytes of the numerical type of pointer that is
*               passed in pValue. This size must be equal to the
*               parameter's type size.
* \param[out]
*    pValue     Buffer to receive value.
* \return
*    Returns 0 upon success, -1 otherwise.
* \par API level
*    7 or higher.
* \par Timing requirements
*    This function can only be called after calling GetFrame() and before
*    calling ReleaseFrame() on the given frame.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRCameraDevice_GetFrameParamNum(
    qvrcamera_device_helper_t* helper, qvrcamera_frame_t *pFrame,
    QVRCAMERA_FRAME_PARAM param, uint8_t size, void* pValue)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_7) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->GetFrameParamNum) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetFrameParamNum(
        helper->cameraHandle, pFrame, param, size, pValue);
}

/**********************************************************************//**
* Reads a int8_t parameter value from the camera frame.
*
* \param[in]
*    helper     qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pFrame     pointer to frame.
* \param[in]
*    param      QVRCAMERA_FRAME_PARAM to specify the parameter value to
*               retrieve.
* \param[out]
*    pValue     Buffer to receive value.
* \return
*    Returns 0 upon success, -1 otherwise.
* \par API level
*    7 or higher.
* \par Timing requirements
*    This function can only be called after calling GetFrame() and before
*    calling ReleaseFrame() on the given frame.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRCameraDevice_GetFrameParamI8(
    qvrcamera_device_helper_t* helper, qvrcamera_frame_t *pFrame,
    QVRCAMERA_FRAME_PARAM param, int8_t* pValue)
{
    return QVRCameraDevice_GetFrameParamNum(helper, pFrame, param,
        sizeof(int8_t), (void*) pValue);
}

/**********************************************************************//**
* Reads a int32_t parameter value from the camera frame.
*
* \param[in]
*    helper     qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pFrame     pointer to frame.
* \param[in]
*    param      QVRCAMERA_FRAME_PARAM to specify the parameter value to
*               retrieve.
* \param[out]
*    pValue     Buffer to receive value.
* \return
*    Returns 0 upon success, -1 otherwise.
* \par API level
*    7 or higher.
* \par Timing requirements
*    This function can only be called after calling GetFrame() and before
*    calling ReleaseFrame() on the given frame.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRCameraDevice_GetFrameParamI32(
    qvrcamera_device_helper_t* helper, qvrcamera_frame_t *pFrame,
    QVRCAMERA_FRAME_PARAM param, int32_t* pValue)
{
    return QVRCameraDevice_GetFrameParamNum(helper, pFrame, param,
        sizeof(int32_t), (void*) pValue);
}

/**********************************************************************//**
* Reads a uint64_t parameter value from the camera frame.
*
* \param[in]
*    helper     qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    pFrame     pointer to frame.
* \param[in]
*    param      QVRCAMERA_FRAME_PARAM to specify the parameter value to
*               retrieve.
* \param[out]
*    pValue     Buffer to receive value.
* \return
*    Returns 0 upon success, -1 otherwise.
* \par API level
*    7 or higher.
* \par Timing requirements
*    This function can only be called after calling GetFrame() and before
*    calling ReleaseFrame() on the given frame.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRCameraDevice_GetFrameParamU64(
    qvrcamera_device_helper_t* helper, qvrcamera_frame_t *pFrame,
    QVRCAMERA_FRAME_PARAM param, uint64_t* pValue)
{
    return QVRCameraDevice_GetFrameParamNum(helper, pFrame, param,
        sizeof(uint64_t), (void*) pValue);
}

/***************************************************************************//**
* Returns a handle to an object for the specified class ID.
*
* \param[in]
*    helper      qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    classId     Class Id of a QVRService component/module
* \param[in]
*    uri         Key/Value pair of the QVRService component/module.  This parameter
*                can be NULL if the classId can uniquely identify the component,
*                e.g., no subcomponents.
* \return
*    Returns a handle to a QVRService component if successful, NULL
*    otherwise
* \par API level
*    8 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The caller must call ReleaseClassHandle() when the component's
*    functionality is no longer needed. Failure to do so prior to calling
*    Detach() on the device may result in the device resources not being
*    completely deallocated.
*******************************************************************************/
static inline qvrservice_class_t* QVRCameraDevice_GetClassHandle(
    qvrcamera_device_helper_t* helper, uint32_t classId, const char* uri)
{
    if (!helper) return NULL;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_8) return NULL;
    if (!helper->cam->ops->GetClassHandle) return NULL;
    return helper->cam->ops->GetClassHandle(helper->cameraHandle, classId, uri);
}

/***************************************************************************//**
* Releases the handle returned by QVRCameraDevice_GetClassHandle()
*
* \param[in]
*    helper      qvrcamera_device_helper_t* returned by AttachCamera()
* \param[in]
*    handle      Handle returned by QVRCameraDevice_GetClassHandle().
* \return
*    None
* \par API level
*    8 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
*******************************************************************************/
static inline void QVRCameraDevice_ReleaseClassHandle(
    qvrcamera_device_helper_t* helper, qvrservice_class_t* handle)
{
    if (!helper) return;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_8) return;
    if (!helper->cam->ops->ReleaseClassHandle) return;
    return helper->cam->ops->ReleaseClassHandle(helper->cameraHandle, handle);
}

/**********************************************************************//**
* Allows the client to retrieve camera frame metadata and associated buffers.
* Use this function rather than QVRCameraDevice_GetFrame() for early access
* to partially filled buffers with minimal latency.
* When QVRCameraDevice_GetFrameEx() returns, the frame buffer is automatically
* locked and ready for use.
*
*
* \param[in]
*    helper            qvrcamera_device_helper_t* returned by QVRCameraClient_AttachCamera()
* \param[in]
*    getFrameExInput   refer to \ref XrCameraFrameRequestInfoInputQTI desciption.
*                      For partial frame delivery, have next point to a struct of type
*                      \ref XrCameraPartialFrameRequestInfoInputQTI,
*                      for QVRCameraDevice_GetFrame() alike frame delivery, set next to NULL.
* \param[in,out]
*    getFrameExOutput  refer to \ref XrCameraFrameRequestInfoOutputQTI description.
*                      For partial frame delivery, have next point to a struct of type
*                      \ref XrCameraPartialFrameRequestInfoOutputQTI.
* \return
*    QVR_CAM_SUCCESS upon success
*    QVR_CAM_SIZE_INSUFFICIENT if not enough buffers or hw buffers are provided
*    QVR_CAM_INVALID_PARAM if a parameter was invalid or an output struct could not be filled
*    QVR_CAM_ERROR otherwise
*
* \par Notes
* QVRCameraDevice_GetFrameEx() has two modes of operation:
* - Full, formatted frame retrieval returning actual meta data (same as when
* calling QVRCameraDevice_GetFrame())
* - Partial frame retrieval returning extrapolated meta data (the time stamp
* is calculated from the previous frame's time stamp plus the frame time and
* exposure and gain are copied over from the previous frame).
*
* Whether a camera supports partial frame mode can be queried by
* using the parameter QVR_CAMDEVICE_UINT8_ENABLE_PARTIAL_FRAME_READ in a
* QVRCameraDevice_GetParamNum() call.
*
* For full frame retrieval, the next pointer of getFrameExInput needs to be NULL.
* For partial frame retrieval, the next pointer of getFrameExInput needs to be
* pointing to a struct of type \ref XrCameraPartialFrameRequestInfoInputQTI
* which allows for passing in a desired partial fill level.
*
* When calling QVRCameraDevice_GetFrameEx() very regularly (in the
* exact same timely distance) with the block option
* XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING_SYNC, QVRCameraDevice_GetFrameEx()
* will sync the cameras concerning frame rate and phase to provide the
* requested data just in time. This yields the lowest possible latency.
* If additional calls are needed, for example to get a subsequent
* portion of the same frame or for other non time-critical purposes,
* use the block option XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING
* for those to avoid disturbing synchronization.
*
* Synchronization works for both full and partial frame retrieval.
* For both modes, after attaching, syncing needs to be enabled per
* camera using the function QVRCameraDevice_GetSyncCtrl() with syncSrc set to
* QVR_SYNC_SOURCE_CAMERA_FRAME_CLIENT_READ.
*
* The expected buffer fill time from 0 to 100% can be obtained as microseconds by
* using the parameter QVR_CAMDEVICE_INT16_CAMERA_BUFFER_FILL_TIME_US in a
* QVRCameraDevice_GetParamNum call.
*
* In partial frame mode, the returned hardware buffers will continue
* to be filled by the hardware until they are complete, independent
* of further calls to QVRCameraDevice_GetFrameEx().
*
* When calling QVRCameraDevice_GetFrameEx() on YUV420 cameras, chroma channel
* order NV12 vs NV21 can be identified using XrCameraFrameBufferInfoFlagsOutputQTI
* or XrCameraHwBufferFlagsOutputQTI in output struct.
*
* QVRCameraDevice_ReleaseFrame() must be invoked in order to unlock the
* frame buffer(s).
*
* Buffer Layout Information
* -------------------------
* Same as documented for QVRCameraDevice_GetFrame().
*
**************************************************************************/

static inline int32_t QVRCameraDevice_GetFrameEx(
    qvrcamera_device_helper_t* helper, XrCameraFrameRequestInfoInputQTI * getFrameExInput, XrCameraFrameRequestInfoOutputQTI * getFrameExOutput)
{
    if(!helper) return QVR_CAM_INVALID_PARAM;
    if (helper->cam->api_version < QVRCAMERACLIENT_API_VERSION_8) return QVR_CAM_API_NOT_SUPPORTED;
    if(!helper->cam->ops->GetFrameEx) return QVR_CAM_API_NOT_SUPPORTED;

    return helper->cam->ops->GetFrameEx(
        helper->cameraHandle, getFrameExInput, getFrameExOutput);
}

/* Debug helpers */

/**************************************************************************//**
* Returns the string value of a camera status
*
* \param[in]
*    state       QVRCAMERA_CAMERA_STATUS
* \return
*    String name of camera state
**************************************************************************/
static inline const char* QVRCameraHelper_ShowState(QVRCAMERA_CAMERA_STATUS state)
{
    switch(state){
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_CLIENT_DISCONNECTED);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_INITIALISING);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_READY);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_STARTING);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_STARTED);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_STOPPING);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_ERROR);
        QVR_CASE_RETURN_STR(QVRCAMERA_CAMERA_STATUS_MAX);
        default: return "unknown";
    }
}

/**************************************************************************//**
* Returns the string value of a camera error.
*
* @param[in]
*    error       Error code.
* @return
*    String name of camera error or "unknown".
**************************************************************************/
static inline const char* QVRCameraHelper_ShowCamError(int error)
{
    switch(error){
        QVR_CASE_RETURN_STR(QVR_CAM_SUCCESS);
        QVR_CASE_RETURN_STR(QVR_CAM_ERROR);
        QVR_CASE_RETURN_STR(QVR_CAM_CALLBACK_NOT_SUPPORTED);
        QVR_CASE_RETURN_STR(QVR_CAM_API_NOT_SUPPORTED);
        QVR_CASE_RETURN_STR(QVR_CAM_INVALID_PARAM);
        QVR_CASE_RETURN_STR(QVR_CAM_EXPIRED_FRAMENUMBER);
        QVR_CASE_RETURN_STR(QVR_CAM_FUTURE_FRAMENUMBER);
        QVR_CASE_RETURN_STR(QVR_CAM_DROPPED_FRAMENUMBER);
        QVR_CASE_RETURN_STR(QVR_CAM_DEVICE_STOPPING);
        QVR_CASE_RETURN_STR(QVR_CAM_DEVICE_NOT_STARTED);
        QVR_CASE_RETURN_STR(QVR_CAM_DEVICE_DETACHING);
        QVR_CASE_RETURN_STR(QVR_CAM_DEVICE_ERROR);
        QVR_CASE_RETURN_STR(QVR_CAM_SIZE_INSUFFICIENT);
        QVR_CASE_RETURN_STR(QVR_CAM_PARTIAL_FRAME_NOT_ENABLED);
        default: return "unknown";
    }
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVRCAMERA_CLIENT_H */
