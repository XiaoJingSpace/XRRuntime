/******************************************************************************/
/*! \file QXR.h */
/*
* Copyright (c) 2019-2021 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
* Not a contribution.
*
* Copyright (c) 2017-2020 The Khronos Group Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
******************************************************************************/
#ifndef QXR_H
#define QXR_H

/**
 * @addtogroup qxr
 * @{
 */

#include <stdint.h>
#ifdef __hexagon__
typedef struct AHardwareBuffer AHardwareBuffer;
#else
#include <android/hardware_buffer.h>
#endif

#if !defined(XR_MAY_ALIAS)
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4))
#define XR_MAY_ALIAS __attribute__((__may_alias__))
#else
#define XR_MAY_ALIAS
#endif
#endif

#define XR_DEVICE_NAME_MAX_LENGTH      64
#define XR_COMPONENT_NAME_MAX_LENGTH   64
#define XR_CAMERA_NAME_MAX_LENGTH_QTI  64

#define XR_STRUCT_TYPE_QTI_BASE        0x51544900  // 'QTI'

typedef int64_t XrDuration;
typedef uint64_t XrFlags64;
typedef uint32_t XrHandleQTI;

typedef struct XrOffset2DiQTI {
    int32_t x;
    int32_t y;
} XrOffset2DiQTI;

typedef struct XrExtent2DiQTI {
    int32_t width;
    int32_t height;
} XrExtent2DiQTI;

typedef struct XrVector3fQTI {
    float x;
    float y;
    float z;
} XrVector3fQTI;

typedef struct XrQuaternionfQTI {
    float x;
    float y;
    float z;
    float w;
} XrQuaternionfQTI;

typedef struct XrVector2dQTI {
    double x;
    double y;
} XrVector2dQTI;

typedef struct XrVector3dQTI {
    double x;
    double y;
    double z;
} XrVector3dQTI;

typedef struct XrQuaterniondQTI {
    double x;
    double y;
    double z;
    double w;
} XrQuaterniondQTI;

typedef struct XrPosedQTI {
    XrQuaterniondQTI orientation;
    XrVector3dQTI    position;
} XrPosedQTI;

typedef struct XrPosefQTI {
   XrQuaternionfQTI orientation;                            /**< Pose orientation. */
   XrVector3fQTI    position;                               /**< Pose position. */
} XrPosefQTI;

/** A single point within a point cloud. */
typedef struct XrMapPointQTI {
   XrVector3fQTI position;                                  /**< Point position. */
   uint32_t      id;                                        /**< Point ID. */
} XrMapPointQTI;

/** Point cloud. */
typedef struct XrPointCloudQTI {
    uint64_t      timestamp;                              /**< Timestamp in Android boot time ns of when the points were updated. */
    uint32_t      maxPoints;                              /**< Maximum number of elements in the points array. */
    uint32_t      numPoints;                              /**< Number of valid points in the points array. */
    XrMapPointQTI points[];                               /**< Map points. */
} XrPointCloudQTI;

/** QTI structure types. */
typedef enum XrStructureTypeQTI {
    XR_TYPE_QTI_CAM_SENSOR_PROPS = XR_STRUCT_TYPE_QTI_BASE,
    XR_TYPE_QTI_CAM_DEVICE_PROPS,
    XR_TYPE_QTI_CAM_PROPS,
    XR_TYPE_QTI_CAM_FRAME_REQ_INFO_INPUT,
    XR_TYPE_QTI_CAM_FRAME_REQ_INFO_OUTPUT,
    XR_TYPE_QTI_CAM_PARTIAL_FRAME_REQ_INFO_INPUT,
    XR_TYPE_QTI_CAM_PARTIAL_FRAME_REQ_INFO_OUTPUT,
    XR_TYPE_QTI_CAM_HW_FRAME_BUFFER_INFO_OUTPUT,
    XR_TYPE_QTI_CAM_HW_BUFFER_OUTPUT,
    XR_TYPE_QTI_CAM_BUFFER_OUTPUT,
    XR_TYPE_QTI_CAM_FRAME_INFO_OUTPUT,
    XR_TYPE_QTI_CAM_FRAME_BUFFER_INFO_OUTPUT,
    XR_TYPE_QTI_CAM_FRAME_CALIBRATION_INFO,
    XR_TYPE_QTI_CAM_FRAME_CROP_INFO,
    XR_TYPE_QTI_MAX_ENUM = 0x7fffffff
} XrStructureTypeQTI;
#define XR_STRUCTURE_TYPE_QTI_MAX_ENUM XR_TYPE_QTI_MAX_ENUM

/** Hardware component types. */
typedef enum XrHwComponentTypeQTI {
    XR_HW_COMP_TYPE_QTI_CAMERA_SENSOR,                    /**< Physical camera sensor. */
    XR_HW_COMP_TYPE_QTI_MAX_ENUM = 0x7fffffff
} XrHwComponentTypeQTI;

/** Hardware device types. */
typedef enum XrHwDeviceTypeQTI {
    XR_HW_DEVICE_TYPE_QTI_CAMERA,                         /**< A camera device, comprised of one or more camera sensor components. */
    XR_HW_DEVICE_TYPE_QTI_MAX_ENUM = 0x7fffffff
} XrHwDeviceTypeQTI;

/** Distortion models. */
typedef enum XrDistortionModelQTI {                       /**< ENUM to select the specific lens distortion model used. */
    XR_DISTORTION_MODEL_QTI_LINEAR,                       /**< Only the linear projection parameters (principal_point and focal_length) are used. */
    XR_DISTORTION_MODEL_QTI_RADIAL_2_PARAMS,              /**< A radial distortion model using 2 parameters of radial_distortion to form a second-order polynomial. */
    XR_DISTORTION_MODEL_QTI_RADIAL_3_PARAMS,              /**< A radial distortion model using 3 parameters of radial_distortion to form a third-order polynomial. */
    XR_DISTORTION_MODEL_QTI_RADIAL_6_PARAMS,              /**< A radial distortion model using 6 parameters of radial_distortion to form a rational function. */
    XR_DISTORTION_MODEL_QTI_FISHEYE_1_PARAM,              /**< A radial distortion model using the first parameter of radial_distortion, combining arctan with a linear model. */
    XR_DISTORTION_MODEL_QTI_FISHEYE_4_PARAMS,             /**< A radial distortion model using 4 parameters of radial_distortion, combining arctan with a forth-order polynomial. */
    XR_DISTORTION_MODEL_QTI_MAX_ENUM = 0x7fffffff
} XrDistortionModelQTI;

typedef XrFlags64 XrHardwareComponentFlagsQTI;

// Flag bits for XrHardwareComponentFlagsQTI
static const XrHardwareComponentFlagsQTI XR_HARDWARE_COMPONENT_EXTRINSIC_VALID_BIT = 0x00000001;
static const XrHardwareComponentFlagsQTI XR_HARDWARE_COMPONENT_EXTRINSIC_DYNAMIC_BIT = 0x00000002;

typedef XrFlags64 XrHardwareDeviceFlagsQTI;

// Flag bits for XrHardwareDeviceFlagsQTI
static const XrHardwareDeviceFlagsQTI XR_HARDWARE_DEVICE_REMOVABLE_BIT = 0x00000001;

typedef XrFlags64 XrCameraSensorCalibrationFlagsQTI;

// Flag bits for XrCameraSensorCalibrationFlagsQTI
static const XrCameraSensorCalibrationFlagsQTI XR_CAMERA_SENSOR_CALIBRATION_DYNAMIC_BIT = 0x00000001;

/** Hardware component descriptor. */
typedef struct XR_MAY_ALIAS XrHardwareComponentBaseQTI {
    XrStructureTypeQTI           type;
    const void* XR_MAY_ALIAS     next;
    XrHwComponentTypeQTI         componentType;           /**< Type of this component. */
    XrHandleQTI                  handle;                  /**< Component handle. */
    XrHandleQTI                  parentHandle;            /**< Handle of this component's parent. */
    char                         componentName[XR_COMPONENT_NAME_MAX_LENGTH];   /**< Name of the component. */
    XrHardwareComponentFlagsQTI  flags;                   /**< Flags for this component. */
    XrPosedQTI                   extrinsic;               /**< Extrinsic of component in world coordinates, relative to the reference hardware component (IMU). */
    char                         reserved[32];            /**< reserved for future use */
} XrHardwareComponentBaseQTI;

/** Hardware device descriptor. */
typedef struct XR_MAY_ALIAS XrHardwareDeviceBaseQTI {
    XrStructureTypeQTI           type;
    const void* XR_MAY_ALIAS     next;
    XrHwDeviceTypeQTI            deviceType;              /**< Type of this device. */
    XrHandleQTI                  handle;                  /**< Device handle. */
    char                         deviceName[XR_DEVICE_NAME_MAX_LENGTH];      /**< Name of the device. */
    XrHardwareDeviceFlagsQTI     flags;                   /**< Flags for this component. */
    uint32_t                     componentCount;          /**< Number of components in this device. */
    XrHardwareComponentBaseQTI*  components;              /**< Components. */
    char                         reserved[32];            /**< reserved for future use */
} XrHardwareDeviceBaseQTI;

/** Describes the intrinsic calibration parameters of a lens. */
typedef struct XrIntrinsicCalibrationQTI {
    XrExtent2DiQTI       size;                            /**< Size of the image viewed through the lens as [columns, rows]. */
    XrVector2dQTI        principalPoint;                  /**< Location of the principal point in the image in pixels. */
    XrVector2dQTI        focalLength;                     /**< Focal length in x and y direction in pixels. */
    double               skew;                            /**< Axis skew. */
    double               radialDistortion[7];             /**< Up to 7 radial distortion coefficients. */
    double               tangentialDistortion[2];         /**< Up to 2 tangential distortion coefficients. */
    XrDistortionModelQTI distortionModel;                 /**< Lens distortion model used for this calibration. */
} XrIntrinsicCalibrationQTI;

/** Camera sensor crop data. */
typedef struct XrCameraSensorCropQTI {
    XrExtent2DiQTI nativeSensorSize;                      /**< Active sensor dimension. */
    int32_t        crop_left;                             /**< Left crop pixels within active sensor dimension */
    int32_t        crop_right;                            /**< Right crop pixels within active sensor dimension */
    int32_t        crop_top;                              /**< Top crop pixels within active sensor dimension */
    int32_t        crop_bottom;                           /**< Bottom crop pixels within active sensor dimension */
    int32_t        horizontal_binning;                    /**< Horizontal binning factor */
    int32_t        vertical_binning;                      /**< Vertical binning factor */
} XrCameraSensorCropQTI;

/** Camera sensor calibration data. */
typedef struct XrCameraSensorCalibrationQTI {
    XrCameraSensorCalibrationFlagsQTI flags;              /**< Flags for this camera sensor calibration. */
    XrIntrinsicCalibrationQTI         intrinsics;         /**< Intrinsic lens and sensor parameters. */
    XrDuration                        lineTime;           /**< Line time for rolling shutter cameras in nanoseconds; for global shutter cameras, set to 0. */
    int32_t                           timestampRow;       /**< Row of the image that corresponds to the time stamp associated with the image; for global shutter cameras, set to 0. */
} XrCameraSensorCalibrationQTI;

/** Camera sensor properties. */
typedef struct XrCameraSensorPropertiesQTI {
    XrHardwareComponentBaseQTI   base;
    XrOffset2DiQTI               frameOffset;             /**< Offset into frame image for this sensor, together with size in XrIntrinsicCalibrationQTI can be used to define the full rectangle. */
    XrCameraSensorCalibrationQTI calibrationInfo;         /**< Camera sensor calibration information. */
} XrCameraSensorPropertiesQTI;

/** Camera device properties. */
typedef struct XrCameraDevicePropertiesQTI {
    XrHardwareDeviceBaseQTI      base;
    XrExtent2DiQTI               fullResExtent;           /**< Dimensions of full resolution image. */
} XrCameraDevicePropertiesQTI;

/** Camera properties. */
typedef struct XrCameraPropertiesQTI {
    XrStructureTypeQTI       type;                        /**< Set to XR_TYPE_QTI_CAM_PROPS */
    const void* XR_MAY_ALIAS next;
    char                     cameraName[XR_CAMERA_NAME_MAX_LENGTH_QTI]; /**< Camera name. */
} XrCameraPropertiesQTI;

/** The frame pose contains a quaternion (x,y,z,w) representing rotational pose
*   and position vector representing translational pose in the Android Portrait
*   coordinate system.
*/
typedef struct XrFramePoseQTI {
    XrPosefQTI                   pose;                    /**< Rotation and translation pose. */
    uint16_t                     tracking_state;          /**< Bitmask contains the state of 6DOF tracking. */
                                                          /**< If no bits are high : UNINITIALIZED. */
                                                          /**< bit 0     : RELOCATION_IN_PROGRESS. */
                                                          /**< bit 1     : TRACKING_SUSPENDED. */
                                                          /**< bit 2     : TRACKING. */
                                                          /**< bit 3     : FATAL_ERROR. */
                                                          /**< bits 4-15 : reserved. */
    uint16_t                     tracking_warning_flags;  /**< Bitmask contains the 6DOF tracking warnings. */
                                                          /**< These warnings are only valid when pose_quality is 0. */
                                                          /**< Bit 0 : LOW_FEATURE_COUNT_ERROR. */
                                                          /**< Bit 1 : LOW_LIGHT_ERROR. */
                                                          /**< Bit 2 : BRIGHT_LIGHT_ERROR. */
                                                          /**< Bit 3 : STEREO_CAMERA_CALIBRATION. */
                                                          /**< Bit 4 : TIMESTAMP_ERROR. */
                                                          /**< Bit 5 : CONFIG_FILE_ERROR. */
                                                          /**< Bit 6 : CALIBRATION_FILE_ERROR. */
                                                          /**< Bit 7 : EVA_ERROR. */
                                                          /**< Bit 8 : EVA_FATAL. */
                                                          /**< Bits 9-15 : reserved. */
    uint64_t                     ts;                      /**< Timestamp corresponding to the frame. */
    float                        pose_quality;            /**< Binary, with 0.0 (bad) or 1.0 (good). */
    float                        sensor_quality;          /**< This value is deprecated and set to 0. */
    float                        camera_quality;          /**< This value is deprecated and set to 0. */
    uint8_t                      reserved[12];            /**< Reserved. */
} XrFramePoseQTI;

/** Base struct for OpenXR style parameters */
typedef struct XrBaseStructQTI {
    XrStructureTypeQTI                            type; /**< use one of XrStructureTypeQTI */
    const struct XrBaseStructQTI * XR_MAY_ALIAS   next; /**< for future expansion, set to NULL */
} XrBaseStructQTI;

/**************************************************************************//**
* \enum XrCameraBlockModeQTI
*
*   XrCameraBlockModeQTI is an option offered when invoking
*   QVRCameraDevice_GetFrameEx().
*
*   \var XR_CAMERA_BLOCK_MODE_QTI_BLOCKING
*     QVRCameraDevice_GetFrameEx() will block until the requested frame number
*     is available at the requested fill level. If the requested frame number
*     is already filled to the requested level,
*     QVRCameraDevice_GetFrameEx() returns immediately.
*
*   \var XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING
*     QVRCameraDevice_GetFrameEx() will not block.
*     If the requested frame number is not available,
*     QVRCameraDevice_GetFrameEx() returns immediately with
*     return code QVR_CAM_FUTURE_FRAMENUMBER.
*
*   \var XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING_SYNC
*     Behavior is identical to XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING but indicates
*     that the QVRCameraDevice_GetFrameEx() call
*     is the "source" action for the synchronization framework. When specified,
*     the sync framework will measure
*     QVRCameraDevice_GetFrameEx() call frequency and adjust camera
*     timing to minimize latency. Calls must be made at regular intervals
*     (i.e. ~30/45/60/90 Hz). Note that this blocking mode will only be available
*     if the caller obtained a qvrsync_ctrl_t object using
*     QVRCameraDevice_GetSyncCtrl() with the source specified as
*     QVR_SYNC_SOURCE_CAMERA_FRAME_CLIENT_READ.
******************************************************************************/
typedef enum XrCameraBlockModeQTI {
    XR_CAMERA_BLOCK_MODE_QTI_BLOCKING,
    XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING,
    XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING_SYNC,
} XrCameraBlockModeQTI;

/**************************************************************************//**
* \enum XrCameraDropModeQTI
*
*   XrCameraDropModeQTI is an option offered when invoking
*   QVRCameraDevice_GetFrameEx().
*
*   \var XR_CAMERA_DROP_MODE_QTI_NEWER_IF_AVAILABLE
*     QVRCameraDevice_GetFrameEx() will attempt
*     to return at least the requested frame number or a newer if available.
*     If the requested frame number is in the future,
*     QVRCameraDevice_GetFrameEx() either blocks or
*     returns with error code QVR_CAM_FUTURE_FRAMENUMBER depending on the
*     XrCameraBlockModeQTI specified.
*
*   \var XR_CAMERA_DROP_MODE_QTI_EXPLICIT_FRAME_NUMBER
*     QVRCameraDevice_GetFrameEx() will attempt
*     to return the requested frame number. If frame number is too old,
*     QVRCameraDevice_GetFrameEx() return
*     with error code QVR_CAM_EXPIRED_FRAMENUMBER. If frame number is in the
*     future, QVRCameraDevice_GetFrameEx() either
*     blocks or returns with error code QVR_CAM_FUTURE_FRAMENUMBER depending
*     on the XrCameraBlockModeQTI specified.
******************************************************************************/
typedef enum XrCameraDropModeQTI {
    XR_CAMERA_DROP_MODE_QTI_NEWER_IF_AVAILABLE,
    XR_CAMERA_DROP_MODE_QTI_EXPLICIT_FRAME_NUMBER
} XrCameraDropModeQTI;

/** QVRCameraDevice_GetFrameEx input struct
*/
typedef struct XrCameraFrameRequestInfoInputQTI {
    XrStructureTypeQTI      type;           /**< [in] set to XR_TYPE_QTI_CAM_FRAME_REQ_INFO_INPUT */
    XrBaseStructQTI*        next;           /**< [in] set to NULL for full frame retrieval, set to a pointer to a XrCameraPartialFrameRequestInfoInputQTI struct for partial frame retrieval  */
    int                     frameNum;       /**< [in] set to requested frame number (0 may be used together with drop mode XR_CAMERA_DROP_MODE_QTI_NEWER_IF_AVAILABLE for the latest frame) */
    XrCameraBlockModeQTI    blockMode;      /**< [in] refer to \ref XrCameraBlockModeQTI */
    XrCameraDropModeQTI     dropMode;       /**< [in] refer to \ref XrCameraDropModeQTI */
} XrCameraFrameRequestInfoInputQTI;

/** QVRCameraDevice_GetFrameEx additional input struct
*   for early partial frame buffer access
*/
typedef struct XrCameraPartialFrameRequestInfoInputQTI {
    XrStructureTypeQTI      type;           /**< [in] set to XR_TYPE_QTI_CAM_PARTIAL_FRAME_REQ_INFO_INPUT */
    XrBaseStructQTI*        next;           /**< [in] set to NULL */
    uint32_t                fillPercentage; /**< [in] the requested minimum fill level in percent (in increments of 5 from 5% to 100%) */
} XrCameraPartialFrameRequestInfoInputQTI;

typedef XrFlags64 XrCameraHwBufferFlagsOutputQTI;

/** \var XrCameraHwBufferFlagsOutputQTI
*   Flag bits for XrCameraHwBufferInfoOutputFlagsQTI
*
*   XR_CAMERA_HW_BUFFER_INFO_OUTPUT_QTI_BUFFER_LOCKED_BIT:    indicating the buffer has been locked upon calling QVRCameraDevice_GetFrameEx(), call QVRCameraDevice_ReleaseFrame() to unlock it
*   XR_CAMERA_HW_BUFFER_INFO_OUTPUT_QTI_YUV420NV12_VALID_BIT: indicating the yuv420 frame format nv12/CBCR, if bit not set YUV420 format is nv21/CRCB.
*/
static const XrCameraHwBufferFlagsOutputQTI XR_CAMERA_HW_BUFFER_INFO_OUTPUT_QTI_BUFFER_LOCKED_BIT     = 0x00000001;
static const XrCameraHwBufferFlagsOutputQTI XR_CAMERA_HW_BUFFER_INFO_OUTPUT_QTI_YUV420NV12_VALID_BIT  = 0x00000002;

/** Android Hardware Buffer-related info.
*/
typedef struct XrCameraHwBufferOutputQTI {
    XrStructureTypeQTI              type;       /**< [in] set to XR_TYPE_QTI_CAM_HW_BUFFER_OUTPUT */
    XrBaseStructQTI*                next;       /**< [in] set to NULL */
    XrCameraHwBufferFlagsOutputQTI  flags;      /**< [out] refer to \ref XrCameraHwBufferFlagsOutputQTI */
    AHardwareBuffer*                buf;        /**< [out] AHardwareBuffer pointer (use this handle to get more info via the AHardwareBuffer APIs) */
    XrOffset2DiQTI                  offset;     /**< [out] target offset coordinates for use in assembling this buffer into a merged image */
    volatile uint8_t*               bufVAddr;   /**< [out] hardware frame data pointer */
} XrCameraHwBufferOutputQTI;

typedef XrFlags64 XrCameraFrameBufferInfoFlagsOutputQTI;

/** \var XrCameraFrameBufferInfoFlagsOutputQTI
*   Flag bits for XrCameraHwBufferInfoOutputFlagsQTI
*
*   XR_CAMERA_FRAME_BUFFER_INFO_OUTPUT_QTI_CROP_VALID_BIT:       indicating the buffer has been cropped
*   XR_CAMERA_FRAME_BUFFER_INFO_OUTPUT_QTI_YUV420NV12_VALID_BIT: indicating the yuv420 frame format nv12/CBCR, if bit not set YUV420 format is nv21/CRCB.
*/
static const XrCameraFrameBufferInfoFlagsOutputQTI XR_CAMERA_FRAME_BUFFER_INFO_OUTPUT_QTI_CROP_VALID_BIT        = 0x00000001;
static const XrCameraFrameBufferInfoFlagsOutputQTI XR_CAMERA_FRAME_BUFFER_INFO_OUTPUT_QTI_YUV420NV12_VALID_BIT  = 0x00000002;

/* QVR buffer-related info. Contains potentially merged/cropped images.
*/
typedef struct XrCameraFrameBufferOutputQTI {
    XrStructureTypeQTI                      type;       /**< [in] set to XR_TYPE_QTI_CAM_BUFFER_OUTPUT */
    XrBaseStructQTI*                        next;       /**< [in] set to NULL */
    XrCameraFrameBufferInfoFlagsOutputQTI   flags;      /**< [out] refer to \ref XrCameraFrameBufferInfoFlagsOutputQTI */
    uint32_t                                format;     /**< [out] refer to \ref QVRCAMERA_FRAME_FORMAT QVRCAMERA_FRAME_FORMAT */
    uint32_t                                len;        /**< [out] frame length = width * height */
    uint32_t                                width;      /**< [out] width of the frame */
    uint32_t                                height;     /**< [out] height of the frame */
    uint32_t                                stride;     /**< [out] stride of the frame */
    XrOffset2DiQTI                          offset;     /**< [out] target offset coordinates for using in assembling this buffer into a merged image */
    volatile uint8_t*                       bufVAddr;   /**< [out] frame data pointer */
} XrCameraFrameBufferOutputQTI;

typedef XrFlags64 XrCameraFrameInfoFlagsOutputQTI;
/** \var XrCameraFrameInfoFlagsOutputQTI
*   Reserved, no flags defined yet
*/

/** Formated Buffer Information (8-bit buffers / stereo combined buffers / cropped buffers)
*/
typedef struct XrCameraFrameBufferInfoOutputQTI {
    XrStructureTypeQTI              type;               /**< [in] set to XR_TYPE_QTI_CAM_FRAME_BUFFER_INFO_OUTPUT */
    XrBaseStructQTI*                next;               /**< [in] set to NULL */
    uint32_t                        bufferCapacityIn;   /**< [in] set to the number of XrCameraFrameBufferOutputQTI structs being provided */
    uint32_t                        bufferCount;        /**< [out] how many XrCameraFrameBufferOutputQTI structs are needed / returned */
    XrCameraFrameBufferOutputQTI*   buffers;            /**< [in,out] set to an allocated array of XrCameraFrameBufferOutputQTI structs */
} XrCameraFrameBufferInfoOutputQTI;

/** Hardware Buffer Information
*/
typedef struct XrCameraHwFrameBufferInfoOutputQTI {
    XrStructureTypeQTI          type;               /**< [in] set to XR_TYPE_QTI_CAM_HW_FRAME_BUFFER_INFO_OUTPUT */
    XrBaseStructQTI*            next;               /**< [in] set to NULL */
    uint32_t                    hwBufferCapacityIn; /**< [in] set to the number of XrCameraHwBufferOutputQTI structs being provided */
    uint32_t                    hwBufferCount;      /**< [out] how many XrCameraHwBufferOutputQTI structs are needed / returned */
    XrCameraHwBufferOutputQTI*  hwBuffers;          /**< [in,out] set to an allocated array of XrCameraHwBufferOutputQTI structs */
} XrCameraHwFrameBufferInfoOutputQTI;

/** Camera info for the image
*/
typedef struct XrCameraFrameInfoOutputQTI {
    XrStructureTypeQTI              type;                       /**< [in] set to XR_TYPE_QTI_CAM_FRAME_INFO_OUTPUT */
    XrBaseStructQTI*                next;                       /**< [in] set to NULL */
    XrCameraFrameInfoFlagsOutputQTI flags;                      /**< [out] refer to \ref XrCameraFrameInfoFlagsOutputQTI */
    uint64_t                        startOfExposureNs;          /**< [out] frame timestamp in ns (kernel boottime clk) */
    uint32_t                        exposureNs;                 /**< [out] frame exposure time in ns. */
    uint32_t                        gain;                       /**< [out] frame gain */
    uint64_t                        rollingShutterSkewNs;       /**< [out] frame rolling shutter skew in ns */
    uint64_t                        targetRollingShutterSkewNs; /**< [out] frame rolling shutter skew time in ns based on scaled target dimension */
    uint8_t                         autoExposureMode;           /**< [out] indicates whether the camera stack performed auto-exposure for this frame */
} XrCameraFrameInfoOutputQTI;

typedef XrFlags64 XrCameraFrameRequestInfoFlagsOutputQTI;

/** \var XrCameraFrameRequestInfoFlagsOutputQTI
*   Flag bits for XrGetFrameInputFlagsQTI
*
*   XR_CAMERA_REQUEST_INFO_OUTPUT_QTI_PEER_FRAME_NUM_VALID_BIT: indicating the peer frame number is valid
*
*   XR_CAMERA_REQUEST_INFO_OUTPUT_QTI_SETTINGS_ESTIMATED_BIT: indicating the meta data is extrapolated (time stamp is estimated from previous frame plus frame time, exposure and gain are copied from previous frame)
*
*   XR_CAMERA_REQUEST_INFO_OUTPUT_QTI_BUFFER_INFO_VALID_BIT: indicating bufferInfo is valid
*/
static const XrCameraFrameRequestInfoFlagsOutputQTI XR_CAMERA_REQUEST_INFO_OUTPUT_QTI_PEER_FRAME_NUM_VALID_BIT = 0x00000001;
static const XrCameraFrameRequestInfoFlagsOutputQTI XR_CAMERA_REQUEST_INFO_OUTPUT_QTI_SETTINGS_ESTIMATED_BIT   = 0x00000002;
static const XrCameraFrameRequestInfoFlagsOutputQTI XR_CAMERA_REQUEST_INFO_OUTPUT_QTI_BUFFER_INFO_VALID_BIT    = 0x00000004;

/** Top level output struct of QVRCameraDevice_GetFrameEx().
*/
typedef struct XrCameraFrameRequestInfoOutputQTI {
    XrStructureTypeQTI                      type;           /**< [in] set to XR_TYPE_QTI_CAM_FRAME_REQ_INFO_OUTPUT */
    XrBaseStructQTI*                        next;           /**< [in] may be set to point to other structs for supplemental frame info (i.e. XrCameraPartialFrameRequestInfoOutputQTI or XrCameraFrameCalibrationInfoOutputQTI), or NULL if not needed */
    XrCameraFrameRequestInfoFlagsOutputQTI  flags;          /**< [out] refer to \ref XrCameraFrameRequestInfoFlagsOutputQTI */
    uint32_t                                frameNum;       /**< [out] frame number since start of camera */
    uint32_t                                peerFrameNum;   /**< [out] Indicates matching peer frame number when pair sync is active */
    XrCameraFrameInfoOutputQTI              frameInfo;      /**< [out] refer to XrCameraFrameInfoOutputQTI */
    XrCameraFrameBufferInfoOutputQTI        bufferInfo;     /**< [out] refer to XrCameraFrameBufferInfoOutputQTI (needed in full frame mode unless option ::QVR_CAMDEVICE_STRING_GET_FRAME_IMG_DISABLE is set)) */
    XrCameraHwFrameBufferInfoOutputQTI      hwBufferInfo;   /**< [out] refer to XrCameraHwFrameBufferInfoOutputQTI (needed in partial frame mode) */
} XrCameraFrameRequestInfoOutputQTI;

/** \enum XrCameraPartialFrameTypeQTI
*   Frame type
*   If a partially filled frame with the requested fill level is not yet available,
*   QVRCameraDevice_GetFrameEx() falls back to full frame retrieval of the most recent
*   frame similar to QVRCameraDevice_GetFrame(). If that is the case, the meta data
*   is reliable, while meta data for partially filled buffers is extrapolated.
*/
typedef enum XrCameraPartialFrameTypeQTI {
    XR_CAMERA_PARTIAL_FRAME_TYPE_QTI_REGULAR,   /**< full frame with reliable meta data */
    XR_CAMERA_PARTIAL_FRAME_TYPE_QTI_PARTIAL    /**< low latency (partially filled) frame with extrapolated meta data */
} XrCameraPartialFrameTypeQTI;

/** Partial frame output struct of QVRCameraDevice_GetFrameEx().
*/
typedef struct XrCameraPartialFrameRequestInfoOutputQTI {
    XrStructureTypeQTI              type;           /**< [in] set to XR_TYPE_QTI_CAM_PARTIAL_FRAME_REQ_INFO_OUTPUT */
    XrBaseStructQTI*                next;           /**< [in] set to NULL i */
    uint32_t                        fillPercentage; /**< [out] The current fill level of the buffers */
    XrCameraPartialFrameTypeQTI     frameType;      /**< [out] refer to \ref XrCameraPartialFrameTypeQTI */
} XrCameraPartialFrameRequestInfoOutputQTI;

/** Calibration info for a camera frame that is adjusted to fit the
*   resolution of the camera frame. This may be different than the calibration
*   information returned by QVRCameraDevice_GetProperties(), which returns
*   calibration information for the resolution that the calibration was
*   performed at. This struct can be chained onto the output struct during
*   a call to GetFrameEx().
*/
typedef struct XrCameraFrameCalibrationInfoOutputQTI {
    XrStructureTypeQTI            type;                  /**< [in] set to XR_TYPE_QTI_CAM_FRAME_CALIBRATION_INFO */
    XrBaseStructQTI*              next;
    uint32_t                      calibrationCapacityIn; /**< [in] Set to the number of XrCameraSensorCalibrationQTI structs being provided or 0 for a size request. */
    uint32_t                      calibrationCount;      /**< [out] how many XrCameraSensorCalibrationQTI structs are needed / returned */
    XrCameraSensorCalibrationQTI* calibrations;          /**< [in,out] Set to an array of \ref XrCameraSensorCalibrationQTI structs or NULL for a size request */
} XrCameraFrameCalibrationInfoOutputQTI;

/** Crop info for a camera frame. This struct can be chained onto the output struct during
*   a call to GetFrameEx().
*/
typedef struct XrCameraFrameCropInfoOutputQTI {
    XrStructureTypeQTI            type;           /**< [in] set to XR_TYPE_QTI_CAM_FRAME_CROP_INFO */
    XrBaseStructQTI*              next;
    uint32_t                      cropCapacityIn; /**< [in] Set to the number of XrCameraSensorCropQTI structs being provided or 0 for a size request. */
    uint32_t                      cropCount;      /**< [out] how many XrCameraSensorCropQTI structs are needed / returned */
    XrCameraSensorCropQTI*        crops;          /**< [in,out] Set to an array of \ref XrCameraSensorCropQTI structs or NULL for a size request */
} XrCameraFrameCropInfoOutputQTI;

/**
 * @}
 */

#endif /* QXR_H */
