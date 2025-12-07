/******************************************************************************/
/*! \file  QVRTypes.h */
/*
* Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/
#ifndef QVRTYPES_H
#define QVRTYPES_H

#include "QXR.h"

/**
 * @addtogroup qvr_types
 * @{
 */

/*
 * Error codes
 */
#define QVR_SUCCESS                     0
#define QVR_ERROR                      -1
#define QVR_CALLBACK_NOT_SUPPORTED     -2
#define QVR_API_NOT_SUPPORTED          -3
#define QVR_INVALID_PARAM              -4
#define QVR_BUSY                       -5
#define QVR_RESULT_PENDING             -6
#define QVR_ANCHOR_BUFFER_SIZE_INSUFFICIENT  -7
#define QVR_ANCHOR_HANDLE_INVALID      -8
#define QVR_ANCHOR_POSE_INVALID        -9
#define QVR_ANCHOR_ID_INVALID          -10
#define QVR_NO_AVAILABLE_BUFFERS       -11
#define QVR_ANCHOR_MAP_SIZE_INSUFFICIENT     -12
#define QVR_ANCHOR_MAPSERVICE_NOT_ENABLED    -13
#define QVR_ANCHOR_INVALID_CALIBRATION -14

#ifndef QVR_CASE_RETURN_STR
#define QVR_CASE_RETURN_STR(a)  case a: return #a
#endif

static inline const char* QVRErrorToString(int error)
{
    switch(error){
    QVR_CASE_RETURN_STR(QVR_SUCCESS);
    QVR_CASE_RETURN_STR(QVR_ERROR);
    QVR_CASE_RETURN_STR(QVR_CALLBACK_NOT_SUPPORTED);
    QVR_CASE_RETURN_STR(QVR_API_NOT_SUPPORTED);
    QVR_CASE_RETURN_STR(QVR_INVALID_PARAM);
    QVR_CASE_RETURN_STR(QVR_BUSY);
    QVR_CASE_RETURN_STR(QVR_RESULT_PENDING);
    QVR_CASE_RETURN_STR(QVR_ANCHOR_BUFFER_SIZE_INSUFFICIENT);
    QVR_CASE_RETURN_STR(QVR_ANCHOR_HANDLE_INVALID);
    QVR_CASE_RETURN_STR(QVR_ANCHOR_POSE_INVALID);
    QVR_CASE_RETURN_STR(QVR_NO_AVAILABLE_BUFFERS);
    QVR_CASE_RETURN_STR(QVR_ANCHOR_MAP_SIZE_INSUFFICIENT);
    QVR_CASE_RETURN_STR(QVR_ANCHOR_MAPSERVICE_NOT_ENABLED);
    QVR_CASE_RETURN_STR(QVR_ANCHOR_INVALID_CALIBRATION);
    default: return "unknown";
    }
}

/** This structure contains a quaternion (x,y,z,w) representing rotational pose
  * and position vector representing translational pose in the Android Portrait
  * coordinate system. */
typedef struct qvrservice_head_tracking_data_t {
    float rotation[4];                       /*!< Rotational pose. */
    float translation[3];                    /*!< Translational pose. */
    uint32_t reserved;
    uint64_t ts;                             /*!< Timestamp of the corresponding sensor value
                                                  originating from the sensor stack. */
    uint64_t reserved1;
    float prediction_coff_s[3];              /*!< Used for computing a forward-predicted pose. */
    uint32_t reserved2;
    float prediction_coff_b[3];              /*!< Used for computing a forward-predicted pose. */
    uint32_t reserved3;
    float prediction_coff_bdt[3];            /*!< Used for computing a forward-predicted pose. */
    uint32_t reserved4;
    float prediction_coff_bdt2[3];           /*!< Used for computing a forward-predicted pose. */
    uint16_t tracking_state;                 /*!< Bitmask contains the state of 6DOF tracking.
                                                  If no bits are high : UNINITIALIZED
                                                  bit 0     : RELOCATION_IN_PROGRESS
                                                  bit 1     : TRACKING_SUSPENDED
                                                  bit 2     : TRACKING
                                                  bit 3     : FATAL_ERROR
                                                  bits 4-15 : reserved */
    uint16_t tracking_warning_flags;         /*!< Bitmask contains the 6DOF tracking warnings.
                                                  These warnings are only valid when pose_quality is 0.
                                                  bit 0     : LOW_FEATURE_COUNT_ERROR
                                                  bit 1     : LOW_LIGHT_ERROR
                                                  bit 2     : BRIGHT_LIGHT_ERROR
                                                  bit 3     : STEREO_CAMERA_CALIBRATION_ERROR
                                                  bit 4     : TIMESTAMP_ERROR
                                                  bit 5     : CONFIG_FILE_ERROR
                                                  bit 6     : CALIBRATION_FILE_ERROR
                                                  bit 7     : EVA_ERROR
                                                  bit 8     : EVA_FATAL
                                                  bits 9-15 : reserved. */
    uint32_t flags_3dof_mag_used : 1;        /*!< Will be set only when TRACKING_MODE_ROTATIONAL_MAG
                                                  mode is used. */
    uint32_t flags_3dof_mag_calibrated : 1;  /*!< Will be set only when mag data is calibrated.
                                                  If the flag is not set, user needs to be notified
                                                  to move the device for calibrating mag sensor
                                                  (i.e. figure 8 movement). */
    float pose_quality;                      /*!< Filled only for TRACKING_MODE_POSITIONAL mode.
                                                  It is binary with 0.0 (bad) or 1.0 (good). */
    float sensor_quality;                    /*!< This value is deprecated and set to 0. */
    float camera_quality;                    /*!< This value is deprecated and set to 0. */
    float prediction_coff_ts[3];             /*!< Used for computing a forward-predicted pose. */
    uint32_t reserved6;
    float prediction_coff_tb[3];             /*!< used for computing a forward-predicted pose. */
    uint32_t reserved7;
    uint8_t reserved8[32];
} qvrservice_head_tracking_data_t;


typedef enum QVRSERVICE_LATE_LATCHING_SLOT_STATUS {
    QVRSERVICE_LATE_LATCHING_SLOT_UNUSED = 0, /**< Indicates the slot is unused and not being tracked. */
    QVRSERVICE_LATE_LATCHING_SLOT_INITIATED, /** Indicates the slot has been allocated, but is not being tracked. */
    QVRSERVICE_LATE_LATCHING_SLOT_TRACKED, /** Indicates the slot is being tracked by tracker. */
} QVRSERVICE_LATE_LATCHING_SLOT_STATUS;


/**
* @par QVR_EYE_TRACKING_DATA_ENABLE_SYNC
*     When set, the sync framework will measure GetEyeTrackingDataWithFlags()
*     call frequency and adjust the timing of eye pose generation to minimize
*     latency. Calls must be made at regular intervals (i.e. ~30/45/60 Hz).
*     This flag will only be available for use if the caller obtained a
*     qvrsync_ctrl_t object using QVRServiceClient_GetSyncCtrl() with the source
*     specified as QVR_SYNC_SOURCE_EYE_POSE_CLIENT_READ.
*/
typedef uint64_t qvr_eye_tracking_data_flags_t;

static const qvr_eye_tracking_data_flags_t QVR_EYE_TRACKING_DATA_ENABLE_SYNC  = 0x00000001;

/** This structure contains matrices (4x4 floats) representing predictive 6DOF head pose. */
typedef struct qvrservice_predicted_head_tracking_data_t {
    uint64_t start;                /*!< Start and end values can be utilized to ensure atomicity.
                                        For instance, consumer should make a deep copy of the content
                                        and check if start and end value are equal or not. If not
                                        equal, then it should be discarded and pose should be copied
                                        again from shared memory. */
    uint64_t prediction_target_ns; /*!< Specified by ActivatePredictedHeadTrackingPoseElement() API
                                        and used to compute forward_pred_delay_ms (i.e. prediction
                                        target_ns - now). */
    uint64_t last_update_ts_ns;    /*!< Represents timestamp in NS resolution (QTIMER base) when
                                        the struct is updated. */
    uint32_t forward_pred_delay_ms;/*!< forward prediction delay in MS */
    uint32_t slot_status;          /*!< Represents one of the values from
                                        QVRSERVICE_LATE_LATCHING_SLOT_STATUS and indicates whether
                                        this pose is being updated or not by the tracker */
    float    originalPredictedPoseMat44F[16];
                                   /*!< Initial predictive pose when an element is activated by
                                        calling ActivatePredictedHeadTrackingPoseElement() API */
    float    curPredictedPoseMat44F[16];
                                   /*!< Subsequent updated predictive head pose over time and the
                                        applied forward prediction delay is specified in forward
                                        pred_delay_ms */
    uint8_t  reserved[24];
    uint64_t end;                  /*!< Start and end values can be utilized to ensure atomicity.
                                        For instance, consumer should make a deep copy of the content
                                        and check if start and end value are equal or not. If not
                                        equal, then it should be discarded and pose should be copied
                                        again from shared memory. */
} qvrservice_predicted_head_tracking_data_t;

/**
* \enum QVRSERVICE_RING_BUFFER_ID
*
*   \var RING_BUFFER_POSE
*      ID to indicate the ring buffer for the pose data in shared memory.
*      This ring buffer contains qvrservice_head_tracking_data_t elements.
*   \var RING_BUFFER_EYE_POSE
*      ID to indicate the ring buffer for the eye pose data in shared memory.
*      This ring buffer contains qvrservice_eye_tracking_data_t elements.
*   \var RING_BUFFER_PREDICTED_HEAD_POSE
*      ID to indicate the ring buffer for the predicted pose data in shared
*      memory. This ring buffer contains
*      qvrservice_predicted_head_tracking_data_t elements.
*      These elements are activated by calling
*      ActivatePredictedHeadTrackingPoseElement API.
*   \var RING_BUFFER_FRAME_POSE
*      ID to indicate the ring buffer for the frame pose data in shared memory.
*      This ring buffer contains XrFramePoseQTI elements.
*/
typedef enum QVRSERVICE_RING_BUFFER_ID {
    RING_BUFFER_POSE=1,
    RING_BUFFER_EYE_POSE,
    RING_BUFFER_PREDICTED_HEAD_POSE,
    RING_BUFFER_FRAME_POSE,
    RING_BUFFER_MAX
} QVRSERVICE_RING_BUFFER_ID;

/** This structure describes the ring buffer attributes for the provided ring
    buffer ID (see QVRSERVICE_RING_BUFFER_ID). */
typedef struct qvrservice_ring_buffer_desc_t {
    int32_t fd;            /*!< File descriptor of shared memory block. */
    uint32_t size;         /*!< Size in bytes of shared memory block. */
    uint32_t index_offset; /*!< Offset in bytes to the ring index. The index is a 4-byte integer. */
    uint32_t ring_offset;  /*!< Offset in bytes to the first element of the ring buffer. */
    uint32_t element_size; /*!< Size in bytes of each element in the ring buffer. This value
                                should match the structure size for a given ring buffer ID. */
    uint32_t num_elements; /*!< Total number of elements in the ring buffer. */
    uint32_t reserved[2];  /*!<  Reserved for future use. */
} qvrservice_ring_buffer_desc_t;

/**
* \enum QVRSERVICE_HW_COMP_ID
*   \var QVRSERVICE_HW_COMP_ID_HMD
*      Virtual HMD reference point
*   \var QVRSERVICE_HW_COMP_ID_IMU
*      IMU
*   \var QVRSERVICE_HW_COMP_ID_EYE_TRACKING_CAM_L
*      Left eye tracking camera
*   \var QVRSERVICE_HW_COMP_ID_EYE_TRACKING_CAM_R
*      Right eye tracking camera
*/
typedef enum QVRSERVICE_HW_COMP_ID {
    QVRSERVICE_HW_COMP_ID_INVALID,
    QVRSERVICE_HW_COMP_ID_HMD,
    QVRSERVICE_HW_COMP_ID_IMU,
    QVRSERVICE_HW_COMP_ID_EYE_TRACKING_CAM_L,
    QVRSERVICE_HW_COMP_ID_EYE_TRACKING_CAM_R,
    QVRSERVICE_HW_COMP_ID_MAX
} QVRSERVICE_HW_COMP_ID;

/** Used to get/set hardware transforms. */
typedef struct _qvrservice_hw_transform_t {
    int from;    /*!<  Must be set to a valid QVRSERVICE_HW_COMP_ID value. */
    int to;      /*!<  Must be set to a valid QVRSERVICE_HW_COMP_ID value. */
    float m[16]; /*!< 4x4 row major float matrix that represents the spatial transform
                      between hardware components represented by the from and to members. */
} qvrservice_hw_transform_t;


/**************************************************************************//**
* QVR eye-tracking data section.
******************************************************************************/
#define QVR_MAX_VIEWPORTS 6

/**
* \enum QVRSERVICE_EYE_ID
*   \var QVR_EYE_LEFT
*      Left eye index.
*   \var QVR_EYE_RIGHT
*      Right eye index.
*/
typedef enum QVRSERVICE_EYE_ID {
    QVR_EYE_LEFT,
    QVR_EYE_RIGHT,
    QVR_EYE_MAX,
} QVRSERVICE_EYE_ID;

#define qvrservice_eye_id QVRSERVICE_EYE_ID;

/**
*   Type representing a bitmask to convey which signals the system is capable
*   of delivering. Unlike struct-specific flags (e.g. qvr_gaze_flags_t), which
*   may be set or cleared depending on run-time conditions, these bits reflect
*   whether a particular signal may ever be delivered by the system.
*/
typedef uint64_t qvr_capabilities_flags_t;

/// Flag bits for qvr_capabilities_flags_t for eye tracking
/// Note: an eye tracking plugin must support the QVR_CAPABILITY_GAZE_COMBINED_GAZE
///       capability for the system to consider that stereo eye tracking is supported
///       (QVRSERVICE_EYE_TRACKING_MODE_DUAL)
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_COMBINED_GAZE = 0x00000001;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_CONVERGENCE_DISTANCE = 0x00000002;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_FOVEATED_GAZE = 0x00000004;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_GAZE_ORIGIN = 0x00000008;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_GAZE_DIRECTION = 0x00000010;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_2D_GAZE_POINT = 0x00000020;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_EYE_OPENNESS = 0x00000040;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_PUPIL_DILATION = 0x00000080;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_POSITION_GUIDE = 0x00000100;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_PER_EYE_BLINK = 0x00000200;
static const qvr_capabilities_flags_t QVR_CAPABILITY_GAZE_SUGGESTED_EYE_CAMERA_OFFSET_X = 0x00000400;

/**
*   Type representing a bitmask used by the eye tracking plugin to convey
*   the availability of the combined eye gaze data.
*   A valid value for a variable of type qvr_gaze_flags_t is
*   either zero or a bitwise OR of the individual valid flags shown below:
*   - QVR_GAZE_ORIGIN_COMBINED_VALID
*   - QVR_GAZE_DIRECTION_COMBINED_VALID
*   - QVR_GAZE_CONVERGENCE_DISTANCE_VALID
*/
typedef uint64_t qvr_gaze_flags_t;

/// Flag bits for qvr_gaze_flags_t
static const qvr_gaze_flags_t QVR_GAZE_ORIGIN_COMBINED_VALID = 0x00000001;
static const qvr_gaze_flags_t QVR_GAZE_DIRECTION_COMBINED_VALID = 0x00000002;
static const qvr_gaze_flags_t QVR_GAZE_CONVERGENCE_DISTANCE_VALID = 0x00000004;

/**
*   Type representing a bitmask used by the plugin to convey
*   the availability of the per-eye gaze data.
*   A valid value for a variable of type qvr_gaze_per_eye_flags_t is
*   either zero or a bitwise OR of the individual valid flags shown below:
*   - QVR_GAZE_PER_EYE_GAZE_ORIGIN_VALID
*   - QVR_GAZE_PER_EYE_GAZE_DIRECTION_VALID
*   - QVR_GAZE_PER_EYE_GAZE_POINT_VALID
*   - QVR_GAZE_PER_EYE_EYE_OPENNESS_VALID
*   - QVR_GAZE_PER_EYE_PUPIL_DILATION_VALID
*   - QVR_GAZE_PER_EYE_POSITION_GUIDE_VALID
*   - QVR_GAZE_PER_EYE_BLINK_VALID
*   - QVR_GAZE_PER_EYE_SUGGESTED_EYE_CAMERA_OFFSET_X_VALID
*/
typedef uint64_t qvr_gaze_per_eye_flags_t;

/** Flag bits for qvr_gaze_per_eye_flags_t. */
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_GAZE_ORIGIN_VALID = 0x00000001;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_GAZE_DIRECTION_VALID = 0x00000002;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_GAZE_POINT_VALID = 0x00000004;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_EYE_OPENNESS_VALID = 0x00000008;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_PUPIL_DILATION_VALID = 0x00000010;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_POSITION_GUIDE_VALID = 0x00000020;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_BLINK_VALID = 0x00000040;
static const qvr_gaze_per_eye_flags_t QVR_GAZE_PER_EYE_SUGGESTED_EYE_CAMERA_OFFSET_X_VALID = 0x00000080;

/** Used to report the per-eye eye-tracking information. */
typedef struct qvrservice_gaze_per_eye_t {
    qvr_gaze_per_eye_flags_t flags;           /*!< Validity bitmask for the per-eye eye gaze
                                                   attributes. */
    uint32_t                 reserved1[2];
    float                    gazeOrigin[3];   /*!< Contains the origin (x, y, z) of the eye
                                                   gaze vector in meters from the HMD center-eye
                                                   coordinate system's origin. */
    uint32_t                 reserved2;
    float                    gazeDirection[3];/*!< Contains the unit vector of the eye gaze direction
                                                   in the HMD center-eye coordinate system. */
    uint32_t                 reserved3;
    float                    viewport2DGazePoint[QVR_MAX_VIEWPORTS][2];
                                              /*!< For each supported viewport, an unsigned
                                                   normalized 2D gaze point where origin is top left
                                                   of the view port. */
    float                    eyeOpenness;     /*!< Value between 0.0 and 1.0 where 1.0 means fully
                                                   open and 0.0 closed. */
    float                    pupilDilation;   /*!< Value in millimeter indicating the pupil
                                                   dilation. */
    uint32_t                 reserved4[2];
    float                    positionGuide[3];/*!< Normalized (0.0-1.0) position of pupil in relation
                                                   to optical axis where 0.5, 0.5 is on the optical axis.
                                                   This information is useful to convey if the user is
                                                   wearing the headset correctly and if the user's
                                                   interpupillary distance matches the lens separation. */
    uint32_t                 blink;           /*!< Indicates whether the eye of the user is closed (1) or
                                                   not closed (0). */
    float                    suggestedEyeCameraOffsetX;
                                              /*!< Contains the suggested eye camera x-axis offset
                                                   from the center-eye when calculating the view matrix.
                                                   Measured in meters from the HMD center-eye coordinate
                                                   system's origin. */
    uint8_t                  reserved[ 256
                                      -sizeof(qvr_gaze_per_eye_flags_t)
                                      -sizeof(float) * 24
                                      -sizeof(uint32_t) * 7];
} qvrservice_gaze_per_eye_t;

/**
*   Type representing a bitmask used by the plugin to convey
*   infomation about the frame data used.
*   A valid value for a variable of type qvr_timing_flags_t is
*   either zero or a bitwise OR of the individual valid flags shown below:
*   - QVR_TIMING_CAMERA_DATA_VALID
*/
typedef uint64_t qvr_timing_flags_t;

/** Flag bits for qvr_gaze_per_eye_flags_t. */
static const qvr_timing_flags_t QVR_TIMING_CAMERA_DATA_VALID = 0x00000001;

/** Used to report eye-tracking data source. */
typedef union qvrservice_timing_t {
    struct {
        uint32_t frameNumber;
        uint32_t exposureNs;
        uint64_t startOfExposureNs; /*!< In ns using Android BOOTTIME clock. */
    } camera;
} qvrservice_timing_t;

/**
* \enum QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE
*   \var QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_TRACKING
*      The user is being tracked and the value has been updated.
*   \var QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_EXTRAPOLATED
*      The user is not being tracked and the value has been extrapolated from
*      historical data.
*   \var QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_LAST_KNOWN
*      The user is not being tracked and the value is a repeat of the last
*      last tracked value.
*/
typedef enum QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE {
    QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_TRACKING,
    QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_EXTRAPOLATED,
    QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_LAST_KNOWN,
    QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE_MAX = 0x7fffffff
} QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE;

/** Used to report the combined eye-tracking information. */
typedef struct qvrservice_eye_tracking_data_t {
    int64_t                   timestamp;               /**< In ns using Android BOOTTIME clock. */
    qvr_gaze_flags_t          flags;                   /**< qvr_gaze_flags_t validity bitmask for the
                                                            combined eye gaze attributes. */
    qvrservice_gaze_per_eye_t eye[QVR_EYE_MAX];        /**< Array of 2 qvrservice_gaze_per_eye_t
                                                            structures. representing the left eye
                                                            then right eye, per-eye gaze information. */
    float                     gazeOriginCombined[3];   /**< Contains the origin (x, y, z) of the
                                                            combined gaze vector in meters from the
                                                            HMD center-eye coordinate system origin. */
    float                     reserved1;               /**< Reserved field. */
    float                     gazeDirectionCombined[3];/**< Contains the origin (x, y, z) of the
                                                            combined gaze vector in meters from the
                                                            HMD center-eye coordinate system's origin. */
    float                     gazeConvergenceDistance; /**< Distance in meters from gazeOriginCombined
                                                            where the vectors converge. */
    qvr_timing_flags_t        timingFlags;             /**< Indicates if timing info is valid and the
                                                            source of the information. */
    qvrservice_timing_t       timing;                  /**< Data source timing information (ex: camera
                                                            exposure and timestamp). */
    float                     foveatedGazeDirection[3]; /**< Contains the unit vector of the gaze
                                                            direction in the HMD center-eye coordinate
                                                            system. Origin of this vector is the same
                                                            as the origin for the HMD center-eye
                                                            coordinate system. This signal is
                                                            optimized for foveated rendering or other
                                                            use cases gazeDirectionCombined is
                                                            preferred. */
    QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE foveatedGazeTrackingState;
                                                        /**< Contains the current state of the
                                                            foveatedGazeDirection signal. */
    // NOTE: struct is 64-bit aligned, add fields accordingly.
    uint8_t                   reserved[ 640
                                       -sizeof(uint64_t)
                                       -sizeof(qvr_gaze_flags_t)
                                       -sizeof(qvrservice_gaze_per_eye_t) * QVR_EYE_MAX
                                       -sizeof(float) * 11
                                       -sizeof(qvr_timing_flags_t)
                                       -sizeof(qvrservice_timing_t)
                                       -sizeof(QVRSERVICE_FOVEATED_GAZE_TRACKING_STATE)];
                                                        /**< Reserved field. */
} qvrservice_eye_tracking_data_t;

/** This structure contains plugin information. */
typedef struct qvr_plugin_info_t {
    char      pluginVendor[256]; /*!< NUL-terminated string identifying the plugin vendor. */
    uint32_t  pluginVersion;     /*!< integer identifying the plugin version. */
    uint32_t  pluginDataVersion; /*!< integer identifying the pluginData version, most
                                      useful for the plugin to convey to its clients any evolution
                                      of the data format or control/payload protocol. */
} qvr_plugin_info_t;

/** Used to set parameters. */
typedef struct _qvr_plugin_param {
    const char* name;
    const char* val;
} qvr_plugin_param_t;

/**************************************************************************//**
* \enum QVR_PLUGIN_DATA_FD_MODE
*   Type of plug-in data fd.
******************************************************************************/
typedef enum QVR_PLUGIN_DATA_FD_MODE {
    QVR_PLUGIN_DATA_FD_MODE_READ,
    QVR_PLUGIN_DATA_FD_MODE_WRITE,
} QVR_PLUGIN_DATA_FD_MODE;


typedef enum QVR_SYNC_SOURCE {
    QVR_SYNC_SOURCE_EYE_POSE_CLIENT_READ, /**< Synchronization is driven by the eye pose being read by a client. */
    QVR_SYNC_SOURCE_CAMERA_FRAME_CLIENT_READ, /** Synchronization is driven by the camera frame being read by a client. */
    QVR_SYNC_SOURCE_DISPLAY_VSYNC, /** Synchronization is driven by the periodic VSYNC signal of the display. */
    QVR_SYNC_SOURCE_MAX
} QVR_SYNC_SOURCE;

typedef struct qvrsync_ctrl_t qvrsync_ctrl_t;

/**************************************************************************//**
* qvrservice_class_t
* -----------------------------------------------------------------------------
* Description
*   Type to a qvrservice component API
******************************************************************************/
typedef struct qvrservice_class_t {
    int api_version;
    XrStructureTypeQTI type;
    void* ops;
} qvrservice_class_t;

/**
 * @}
 */

#endif /* QVRTYPES_H */
