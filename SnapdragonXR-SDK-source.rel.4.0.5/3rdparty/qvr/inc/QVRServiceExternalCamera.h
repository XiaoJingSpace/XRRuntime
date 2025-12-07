/******************************************************************************/
/*! \file  QVRServiceExternalCamera.h */
/*
* Copyright (c) 2016,2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#ifndef QVRSERVICEEXTERNALCAMERA_H
#define QVRSERVICEEXTERNALCAMERA_H

/**
 * @addtogroup qvr_service_external_camera
 * @{
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

/**************************************************************************//**
* \enum QVRSERVICEEXTERNALCAMERA_API_VERSION
* Defines the API versions of this interface. The api_version member of the
* qvr_external_camera_t object retrieved from the external camera library must
* be set to the API version that matches its functionality.
******************************************************************************/
typedef enum QVRSERVICEEXTERNALCAMERA_API_VERSION {
    QVRSERVICEEXTERNALCAMERA_API_VERSION_1 = 1,
    QVRSERVICEEXTERNALCAMERA_API_VERSION_2 = 2,
    QVRSERVICEEXTERNALCAMERA_API_VERSION_3 = 3
} QVRSERVICEEXTERNALCAMERA_API_VERSION;

/**************************************************************************//**
* \enum error_e
* Defines the types of errors that may be encountered while using external
* camera
******************************************************************************/
typedef enum error_e
{
    UNKNOWN_ERROR
} error_e;

/**
* (DEPRECATED in API2+) Callback for handling frame received from the external camera.
* @return
*    Returns 0 upon success, -1 otherwise
* @par API level
*    1 (DEPRECATED for 2 or higher)
*/
typedef int (*frame_ready_callback_fn)
(
    void*       pCtx,      /**< The context passed to Start(). */
    uint32_t    width,     /**< Width of the camera frame buffer. */
    uint32_t    height,    /**< Height of the camera frame buffer. */
    uint64_t    timestamp, /**< Frame timestamp. */
    const char* buffer,    /**< Pointer to the camera frame. */
    uint32_t    buf_len    /**< Size of the buffer. */
);

/**
*(DEPRECATED in API2+) Callback for handling an error from the external camera.
* @par API level
*    1 (DEPRECATED for 2 or higher)
*/
typedef void (*handle_error_callback_fn)
(
    void*   pCtx,  /**< The context passed to Start(). */
    error_e code   /**< Error code. */
);

/**
*  (DEPRECATED in API2+) Callback for registering camera buffer to ensure zero copy of camera frame buf.

* @return
*    Returns 0 upon success, -1 otherwise
* @par API level
*    1 (DEPRECATED for 2 or higher)
******************************************************************************/
typedef int (*buffer_register_callback_fn)
(
    void* pCtx,     /**< The context passed to Start(). */
    void* buffer,   /**< Pointer to the camera frame buffer; must be ION/Gralloc buffer. */
    int   buf_len,  /**< Size of the buffer. */
    int   fd        /**< Corresponding file descriptor of the buffer. */
);

/**
* Camera frame information to allow setup and allocation.
*/
typedef struct qvr_external_camera_info
{
    uint32_t  width;            /*!< Frame width in pixels. */
    uint32_t  height;           /*!< Frame height in pixels. */
    uint32_t  bits_per_pixel;   /*!< Pixel size in bits. */
    uint32_t  interval_ns;      /*!< Frame default interval in ns. */
    int32_t   reserved[64 - 4]; /*!< Future use bits. */
} qvr_external_camera_info_t;

/**
* Frame metadata provided with each callback.
*
* @note
* The frame layout/size may change dynamically between frames due to
* changes in the crop settings made via calls to SetCropRegion().
* There are 3 supported frame cropping modes:
*
* - Crop-off/single-crop: frame is a single image.
*   - width/height: size of image.
*   - secondary_width/height: not used and should be 0.
*   - len: width*height.
* - Dual-crop: frame is 2 images that are sequential in memory
*   - width/height: size of first image.
*   - secondary_width/height: size of second image.
*   - len: width*height + secondary_width*secondary_height.
******************************************************************************/
typedef struct qvr_external_camera_frame
{
    uint32_t fn;                        /*!< Frame number. */
    uint64_t start_of_exposure_ts;      /*!< Frame timestamp in ns (kernel boottime clk). */
    uint32_t exposure;                  /*!< Frame exposure time in ns. */
    volatile uint8_t* buffer;           /*!< Frame data. */
    uint32_t len;                       /*!< Frame length = width*height+
                                             secondary_width*secondary_height. */
    uint32_t width;                     /*!< Width of frame or crop-image1. */
    uint32_t height;                    /*!< Height of frame or crop-image1. */
    uint32_t secondary_width;           /*!< Width of crop-image2 or 0 (no crop-image2). */
    uint32_t secondary_height;          /*!< Height of crop-image2 or 0 (no crop-image2). */
    uint32_t gain;                      /*!< ISO gain (ie 100 = 1x, 400 = 4x). */
    uint64_t rolling_shutter_skew_ns;   /*!< Frame rolling shutter skew in ns. */

    int32_t  reserved[128 - 13];    /*!< Future use bits. */
} qvr_external_camera_frame_t;


typedef struct qvr_external_camera_callback_ops {

    int32_t (*FrameReady) (void*                        pCtx,
                           int32_t                      cam_id,
                           qvr_external_camera_frame_t *pFrame);

    void (*HandleError) (void*   pCtx,
                         int32_t cam_id,
                         error_e code);

    int32_t (*RegisterBuffer) (void*    pCtx,
                               int32_t  cam_id,
                               void*    buffer,
                               int32_t  buf_len,
                               int32_t  fd);

    int32_t (*GetFrameBuffer) (void*     pCtx, /**< The context passed to Start(). */
                               int32_t   cam_id, /**< Camera ID. */
                               int32_t   frame_number, /**< Frame number of the new frame. */
                               void**    new_frame_buffer, /**< Buffer to be used to capture new frame. */
                               int32_t*  new_frame_buffer_size); /**< Frame buffer size. */

    //Reserved for future use
    void* reserved[64 - 4];

} qvr_external_camera_callback_ops_t;

typedef struct qvr_external_camera_callbacks {
    QVRSERVICEEXTERNALCAMERA_API_VERSION  api_version;
    void                                 *pCtx;
    qvr_external_camera_callback_ops_t   *ops;
} qvr_external_camera_callback_t;


typedef struct qvr_external_camera_ops {

    int32_t (*Init)(void);

    int32_t (*Deinit)(void);

    int32_t (*Start)(
        int32_t                     cam_id,
        frame_ready_callback_fn     frame_ready_cb,           /* DEPRECATED in API2+ */
        buffer_register_callback_fn buffer_register_callback, /* DEPRECATED in API2+ */
        handle_error_callback_fn    handle_error_cb,          /* DEPRECATED in API2+ */
        void                       *pCtx);                    /* DEPRECATED in API2+ */

    int32_t (*Stop)(void);

    int32_t (*GetCameraInfo) (
        int32_t                     cam_id,
        qvr_external_camera_info_t *cam_info);

    int32_t (*SetCallbacks) (
        int32_t                         cam_id,
        qvr_external_camera_callback_t *callbacks);

    int32_t (*SetExposureAndGain) (
        int32_t  cam_id,
        uint64_t exposure_ns,
        int32_t  iso_gain);

    int32_t (*SetGammaCorrectionValue) (
        int32_t cam_id,
        float   gamma);

    int32_t (*SetCropRegion) (
        int32_t  cam_id,
        uint32_t l_top,
        uint32_t l_left,
        uint32_t l_width,
        uint32_t l_height,
        uint32_t r_top,
        uint32_t r_left,
        uint32_t r_width,
        uint32_t r_height);

    //Reserved for future use
    void* reserved[64 - 5];

} qvr_external_camera_ops_t;

typedef struct qvr_external_camera{
    QVRSERVICEEXTERNALCAMERA_API_VERSION api_version;
    qvr_external_camera_ops_t *ops;
} qvr_external_camera_t;

qvr_external_camera_t* getInstance(void);


/**
* The following are QVRServiceExternalCameraCallbacks helpers. They are used during the
* callback for handling the frame received from the external camera.
*
* \param[in]
*    me        The context passed to QVRServiceExternalCamera_Start().
* \param[in]
*    cam_id    Camera Id corresponding to this frame.
* \param[in]
*    pFrame    Frame data and info.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher.
******************************************************************************/
static __inline int32_t QVRServiceExternalCameraCallbacks_FrameReady(
    qvr_external_camera_callback_t *me,
    int32_t                         cam_id,
    qvr_external_camera_frame_t    *pFrame)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->FrameReady)           return -1;
    return me->ops->FrameReady(me->pCtx, cam_id, pFrame);
}

/**
* Callback for handling an error from the external camera.
*
* \param[in]
*    me     The context passed to QVRServiceExternalCamera_Start().
* \param[in]
*    cam_id Camera Id corresponding to this frame.
* \param[in]
*    code   Error code.
* \par API level
*    2 or higher
*/
static __inline void QVRServiceExternalCameraCallbacks_HandleError(
    qvr_external_camera_callback_t *me,
    int32_t                         cam_id,
    error_e                         code)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return;
    if (NULL == me->ops || NULL == me->ops->HandleError)          return;
    me->ops->HandleError(me->pCtx, cam_id, code);
}

/**
* Callback for registering the camera buffer to ensure zero copy of camera frame buf.
*
* \param[in]
*    me       The context passed to QVRServiceExternalCamera_Start().
* \param[in]
*    cam_id   Camera ID corresponding to this frame.
* \param[in]
*    buffer   Pointer to the camera frame buffer - must be ION/Gralloc buffer.
* \param[in]
*    buf_len  Size of the buffer.
* \param[in]
*    fd       Corresponding file descriptor of the buffer.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher.
******************************************************************************/
static __inline int32_t QVRServiceExternalCameraCallbacks_RegisterBuffer(
    qvr_external_camera_callback_t *me,
    int32_t                         cam_id,
    void*                           buffer,
    int                             buf_len,
    int                             fd)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->RegisterBuffer)       return -1;
    return me->ops->RegisterBuffer(me->pCtx, cam_id, buffer, buf_len, fd);
}

/**
* Callback to get buffer to ensure zero copy of camera frame and should be
* called for every frame.
*
* \param[in]
*    me                    Context passed to QVRServiceExternalCamera_Start().
* \param[in]
*    cam_id                Camera ID corresponding to this frame.
* \param[in]
*    frame_number          Frame number of the camera frame.
* \param[in]
*    new_frame_buffer      Frame buffer.
* \param[in]
*    new_frame_buffer_size Frame buffer size.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    3 or higher.
*/
static __inline int32_t QVRServiceExternalCameraCallbacks_GetFrameBuffer(
    qvr_external_camera_callback_t *me,
    int32_t                         cam_id,
    int32_t                         frame_number,
    void**                          new_frame_buffer,
    int32_t*                        new_frame_buffer_size)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_3) return -1;
    if (NULL == me->ops || NULL == me->ops->GetFrameBuffer)       return -1;
    return me->ops->GetFrameBuffer(me->pCtx, cam_id, frame_number, new_frame_buffer, new_frame_buffer_size);
}

/**
* The follwoing are QVRServiceExternalCamera helpers.
* This function will be called during VR service initialization.
*
* \param[in]
*    me       Pointer to qvr_external_camera_t.
* \return
*    Returns 0 upon success, -1 otherwise
*/
static __inline int32_t QVRServiceExternalCamera_Init(
    qvr_external_camera_t *me)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_1) return -1;
    if (NULL == me->ops || NULL == me->ops->Init)                 return -1;
    return me->ops->Init();
}

/**
* This function will be called during VR service deinitialization.
*
* \param[in]
*    me       Pointer to qvr_external_camera_t
* \return
*    Returns 0 upon success, -1 otherwise
*/
static __inline int32_t QVRServiceExternalCamera_Deinit(
    qvr_external_camera_t *me)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_1) return -1;
    if (NULL == me->ops || NULL == me->ops->Deinit)               return -1;
    return me->ops->Deinit();
}

/**
* This function will be called when an application calls StartVRMode() on
* the VR service.
*
* \param[in]
*    me       Pointer to qvr_external_camera_t
* \param[in]
*    cam_id                   Camera ID for opening the camera
* \param[in]
*    frame_ready_cb           Function to be called by the external
*                              camera module when new camera frame
*                              is available
* \param[in]
*    buffer_register_callback Function to be called by the external
*                              camera module to register camera buffer
*                              for avoiding deep copy
* \param[in]
*    handle_error_cb          Function to be called by the external
*                              camera module if an error is detected
* \param[in]
*    pCtx                     Context to be passed to callback functions
* \return
*    Returns 0 upon success, -1 otherwise
**************************************************************************/
static __inline int32_t QVRServiceExternalCamera_Start(
    qvr_external_camera_t      *me,
    int32_t                     cam_id,
    frame_ready_callback_fn     frame_ready_cb,           /* DEPRECATED in API2+ */
    buffer_register_callback_fn buffer_register_callback, /* DEPRECATED in API2+ */
    handle_error_callback_fn    handle_error_cb,          /* DEPRECATED in API2+ */
    void                       *pCtx)                     /* DEPRECATED in API2+ */
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_1) return -1;
    if (NULL == me->ops || NULL == me->ops->Start)                return -1;
    /* API2+ "SetCallbacks" should be used to set callbacks and context */
    return me->ops->Start(cam_id,
                          frame_ready_cb,
                          buffer_register_callback,
                          handle_error_cb,
                          pCtx);
}

/**
* This function will be called when an application calls StopVRMode() on
* the VR service.
*
* \param[in]
*    me       Pointer to qvr_external_camera_t.
* \return
*    Returns 0 upon success, -1 otherwise
**************************************************************************/
static __inline int32_t QVRServiceExternalCamera_Stop(
    qvr_external_camera_t      *me)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_1) return -1;
    if (NULL == me->ops || NULL == me->ops->Stop)                 return -1;
    return me->ops->Stop();
}

/**
* Get camera information.
*
* \param[in]
*    me       Pointer to qvr_external_camera_t.
* \param[in]
*    cam_id   Camera ID.
* \param[out]
*    cam_info Camera intrinsic settings.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher.
**************************************************************************/
static __inline int32_t QVRServiceExternalCamera_GetCameraInfo(
    qvr_external_camera_t      *me,
    int32_t                     cam_id,
    qvr_external_camera_info_t *cam_info)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->GetCameraInfo)        return -1;
    return me->ops->GetCameraInfo(cam_id, cam_info);
}

/**
* Set callbacks.
*
* \param[in]
*    me        Pointer to qvr_external_camera_t.
* \param[in]
*    cam_id    Camera ID.
* \param[in]
*    callbacks Callbacks object including camera specific context.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher.
**************************************************************************/
static __inline int32_t QVRServiceExternalCamera_SetCallbacks(
    qvr_external_camera_t          *me,
    int32_t                         cam_id,
    qvr_external_camera_callback_t *callbacks)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->SetCallbacks)         return -1;
    return me->ops->SetCallbacks(cam_id, callbacks);
}

/**
* Set exposure and gain.
*
* \param[in]
*    me          Pointer to qvr_external_camera_t.
* \param[in]
*    cam_id      Camera ID.
* \param[in]
*    exposure_ns Camera exposure in ns.
* \param[in]
*    iso_gain    Camera ISO gain (ie 100 = 1x, 400 = 4x).
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher.
*/
static __inline int32_t QVRServiceExternalCamera_SetExposureAndGain(
    qvr_external_camera_t *me,
    int32_t                cam_id,
    uint64_t               exposure_ns,
    int32_t                iso_gain)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->SetExposureAndGain)   return -1;
    return me->ops->SetExposureAndGain(cam_id, exposure_ns, iso_gain);
}

/**
* Set gamma correction value.
*
* \param[in]
*    me          Pointer to qvr_external_camera_t.
* \param[in]
*    cam_id      Camera ID.
* \param[in]
*    gamma       Camera gamma factor (Vout = Vin^gamma).
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher.
*/
static __inline int32_t QVRServiceExternalCamera_SetGammaCorrectionValue(
    qvr_external_camera_t *me,
    int32_t                cam_id,
    float                  gamma)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->SetExposureAndGain)   return -1;
    return me->ops->SetGammaCorrectionValue(cam_id, gamma);
}

/**
* Set crop region.
*
* \param[in]
*    me          Pointer to qvr_external_camera_t.
* \param[in]
*    cam_id      Camera ID.
* \param[in]
*    l_top       Specifies the top (left) corner of the crop region in
*                 pixels. If the crop region starts at the first pixel,
*                 this value should be zero.
* \param[in]
*    l_left      Specifies the left (top) corner of the crop region in
*                 pixels. If the crop region starts at the first pixel,
*                 this value should be zero.
* \param[in]
*    l_width     Specifies the width of the crop region in pixels.
* \param[in]
*    l_height    Specifies the height of the crop region in pixels.
* \param[in]
*    r_top       Specifies the top (left) corner of the crop region in
*                 pixels. If the crop region starts at the first pixel,
*                 this value should be zero.
* \param[in]
*    r_left      Specifies the left (top) corner of the crop region in
*                 pixels. If the crop region starts at the first pixel,
*                 this value should be zero.
* \param[in]
*    r_width     Specifies the width of the crop region in pixels.
* \param[in]
*    r_height    Specifies the height of the crop region in pixels.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher
*/
static __inline int32_t QVRServiceExternalCamera_SetCropRegion(
    qvr_external_camera_t *me,
    int32_t                cam_id,
    uint32_t               l_top,
    uint32_t               l_left,
    uint32_t               l_width,
    uint32_t               l_height,
    uint32_t               r_top,
    uint32_t               r_left,
    uint32_t               r_width,
    uint32_t               r_height)
{
    if (me->api_version < QVRSERVICEEXTERNALCAMERA_API_VERSION_2) return -1;
    if (NULL == me->ops || NULL == me->ops->SetExposureAndGain)   return -1;
    return me->ops->SetCropRegion(cam_id, l_top, l_left, l_width, l_height, r_top, r_left, r_width, r_height);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVRSERVICEEXTERNALCAMERA_H */
