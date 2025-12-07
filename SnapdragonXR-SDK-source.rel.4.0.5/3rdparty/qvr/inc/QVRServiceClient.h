/*****************************************************************************/
/*! \file  QVRServiceClient.h */
/*
* Copyright (c) 2016-2021 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/
#ifndef QVRSERVICE_CLIENT_H
#define QVRSERVICE_CLIENT_H

/**
 * @addtogroup qvr_service_client
 * @{
 */

/**
Client APIs communicate with QVRService. Typical call flow is as follows:
-# Call QVRServiceClient_Create()
-# Call QVRServiceClient_GetVRMode() to verify VR mode is supported and in the VRMODE_STOPPED state
-# Call QVRServiceClient_RegisterForNotification() to get notified of events
-# Call QVRServiceClient_GetTrackingMode()/QVRServiceClient_SetTrackingMode() to configure head tracking
-# Call QVRServiceClient_GetEyeTrackingMode()/QVRServiceClient_SetEyeTrackingMode() to configure eye tracking
-# Call QVRServiceClient_SetDisplayInterruptConfig() or QVRServiceClient_SetDisplayInterruptCapture() to handle display interrupts or
-# Call QVRServiceClient_StartVRMode() to start VR mode
   - Handle display interrupt events
   - Call QVRServiceClient_GetHeadTrackingData() to read latest head tracking data
   - Call QVRServiceClient_GetEyeTrackingData() to read latest eye tracking data
-# Call QVRServiceClient_StopVRMode() to end VR mode
-# Call QVRServiceClient_Destroy()
*/


#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "QVRTypes.h"
#include "QVRPluginData.h"
#include "QXR.h"



/**************************************************************************//**
* Defines the API versions of this interface. The api_version member of the
* qvrservice_client_t structure must be set to accurately reflect the version
* of the API that the implementation supports.
******************************************************************************/
typedef enum QVRSERVICECLIENT_API_VERSION {
    QVRSERVICECLIENT_API_VERSION_1 = 1, /*!< API version 1 */
    QVRSERVICECLIENT_API_VERSION_2,     /*!< API version 2 */
    QVRSERVICECLIENT_API_VERSION_3,     /*!< API version 3 */
    QVRSERVICECLIENT_API_VERSION_4,     /*!< API version 4 */
    QVRSERVICECLIENT_API_VERSION_5,     /*!< API version 5 */
    QVRSERVICECLIENT_API_VERSION_6,     /*!< API version 6 */
    QVRSERVICECLIENT_API_VERSION_7,     /*!< API version 7 */
    QVRSERVICECLIENT_API_VERSION_8,     /*!< API version 8 */
} QVRSERVICECLIENT_API_VERSION;

/**************************************************************************//**
*   Can be used with GetParam() to retrieve the VR Service version.
* Access
*   Read only.
* Notes
*   None.
******************************************************************************/
#define QVRSERVICE_SERVICE_VERSION    "service-version"

/*
 * QVRSERVICE_CLIENT_VERSION
 */
/**
* @par Description
*   Can be used with GetParam() to retrieve the VR Service client version.
* @par Access
*   Read only.
* @par Notes
*   None.
*/
#define QVRSERVICE_CLIENT_VERSION     "client-version"

/*
 * QVRSERVICE_TRACKER_ANDROID_OFFSET_NS
 */
/**
* @par Description
*   Can be used with GetParam() to retrieve the fixed offset between the
*   tracker time domain (QTimer) and the Android time domain in ns.
* @par Access
*   Read only.
* @par Notes
*   None.
*/
#define QVRSERVICE_TRACKER_ANDROID_OFFSET_NS "tracker-android-offset-ns"

/**************************************************************************//**
* QVRSERVICE plug-in names.
******************************************************************************/
#define QVRSERVICE_PLUGIN_EYE_TRACKING     "plugin-eye-tracking"

/**************************************************************************//**
* QVRSERVICE_DEVICE_MODE
* -----------------------------------------------------------------------------
* Description
*   Can be used with GetParam() to query current device mode qvrservice is running in.
* Access
*   Read
* Notes
*   None.
******************************************************************************/
#define QVRSERVICE_DEVICE_MODE                      "device-mode"

#define QVRSERVICE_DEVICE_MODE_STANDALONE           "standalone"
#define QVRSERVICE_DEVICE_MODE_HOST_SIMPLE          "host_simple"
#define QVRSERVICE_DEVICE_MODE_HOST_SMART           "host_smart"
#define QVRSERVICE_DEVICE_MODE_HOST_AUTO            "host_auto"
#define QVRSERVICE_DEVICE_MODE_SMARTVIEWER_REMOTE   "smartviewer_remote"
#define QVRSERVICE_DEVICE_MODE_SMARTVIEWER_LOCAL    "smartviewer_local"

/**************************************************************************//**
* \enum QVRSERVICE_VRMODE_STATE
* Defines the various states returned by QVRServiceClient_GetVRMode().
******************************************************************************/
typedef enum QVRSERVICE_VRMODE_STATE {
    VRMODE_UNSUPPORTED = 0, /*!< VR Mode is unsupported on this device. */
    VRMODE_STARTING,        /*!< VR Mode is starting. The state will
                                 automatically transition to VRMODE_STARTED once
                                 startup is complete. */
    VRMODE_STARTED,         /*!< VR Mode is started/active. While in this state,
                                 only the client that started VR Mode can modify
                                 the VR Mode state. */
    VRMODE_STOPPING,        /*!< VR Mode is stopping. The state will
                                 automatically transition to VRMODE_STOPPED once
                                 complete. */
    VRMODE_STOPPED,         /*!< VR Mode is stopped/inactive. While in this
                                 state, any client can modify the VR Mode state. */
    VRMODE_HEADLESS,        /*!< VR Mode is headless. The state will automatically
                                 transition to VRMODE_STOPPED once the headset is
                                 connected. While in this state, clients will have
                                 limited functionality. Only registering for
                                 notification is allowed. */
    VRMODE_PAUSING,         /*!< VR Mode is pausing. The state will
                                 automatically transition to VRMODE_PAUSED once
                                 complete.*/
    VRMODE_PAUSED,          /*!< VR Mode is paused. While in this state,
                                 only the client that paused VR Mode can modify
                                 the VR Mode state.*/
    VRMODE_RESUMING,        /*!< VR Mode is resuming. The state will
                                 automatically transition to VRMODE_STARTED once
                                 complete.*/
} QVRSERVICE_VRMODE_STATE;

/**************************************************************************//**
* \enum QVRSERVICE_TRACKING_MODE
*   Defines the various tracking modes.
*
*   \var TRACKING_MODE_NONE
*      Tracking is disabled. Calls to GetHeadTrackingData() will fail.
*   \var TRACKING_MODE_ROTATIONAL
*      Rotational mode provides tracking information using 3 degrees of freedom,
*      i.e. "what direction am I facing?". When this mode is used, the rotation
*      quaternion in the qvrservice_head_tracking_data_t structure will be
*      filled in when GetHeadTrackingData() is called.
*   \var TRACKING_MODE_POSITIONAL
*      Positional mode provides tracking information using 6 degrees of freedom,
*      i.e. "what direction am I facing, and where am I relative to my starting
*      point?" When this mode is used, both the rotation quaternion and
*      translation vector in the qvrservice_head_tracking_data_t structure
*      will be filled in when GetHeadTrackingData() is called.
*   \var TRACKING_MODE_ROTATIONAL_MAG
*      Rotational_Mag mode provides tracking information similar to
*      TRACKING_MODE_ROTATIONAL but without any drift over time. The drift
*      sensitive apps (i.e. movie theater) would use this tracking mode.
*      However, this mode may require spacial movement of the device to have
*      the mag sensor calibrated (i.e. figure 8 movement). Also, this mode
*      is very sensitive to magnetic fields. In case of uncalibrated mag
*      data, this mode automatically defaults to TRACKING_MODE_ROTATIONAL which
*      may exhibit slow drift over time.
******************************************************************************/
typedef enum QVRSERVICE_TRACKING_MODE {
    TRACKING_MODE_NONE = 0,
    TRACKING_MODE_ROTATIONAL = 0x1,
    TRACKING_MODE_POSITIONAL = 0x2,
    TRACKING_MODE_ROTATIONAL_MAG = 0x4,
} QVRSERVICE_TRACKING_MODE;

/**************************************************************************//**
* This structure contains raw sensor values.
******************************************************************************/
typedef struct qvrservice_sensor_data_raw_t {
    uint64_t gts;           /*!< Timestamp of the gyroscope data in ns (QTimer time domain) */
    uint64_t ats;           /*!< Timestamp of the accelerometer data in in (QTimer time domain) */
    float    gx;            /*!< Gyroscope X value in m/s/s. */
    float    gy;            /*!< Gyroscope Y value in m/s/s. */
    float    gz;            /*!< Gyroscope Z value in m/s/s. */
    float    ax;            /*!< Accelerometer X value in rad/s. */
    float    ay;            /*!< Accelerometer Y value in rad/s. */
    float    az;            /*!< Accelerometer Z value in rad/s. */
    uint64_t mts;           /*!< Timestamp of the magnetometer data in ns (QTimer time domain). */
    float    mx;            /*!< Magnetometer X value in uT. */
    float    my;            /*!< Magnetometer Y value in uT. */
    float    mz;            /*!< Magnetometer Z value in uT. */
    uint8_t  reserved[4];   /*!< Reserved for future use. */
} qvrservice_sensor_data_raw_t;

/**************************************************************************//**
* \enum QVRSERVICE_DISP_INTERRUPT_ID
*   Display interrupt types.
*
*   \var DISP_INTERRUPT_VSYNC
*      This is the display VSYNC signal. It fires at the beginning of every
*      frame.
*      Interrupt config data: pointer to qvrservice_vsync_interrupt_config_t
*   \var DISP_INTERRUPT_LINEPTR
*      This interrupt can be configured to interrupt after a line of data has
*      been transmitted to the display.
*      Interrupt config data: pointer to qvrservice_lineptr_interrupt_config_t
******************************************************************************/
typedef enum QVRSERVICE_DISP_INTERRUPT_ID {
    DISP_INTERRUPT_VSYNC = 0,
    DISP_INTERRUPT_LINEPTR,
    DISP_INTERRUPT_MAX
} QVRSERVICE_DISP_INTERRUPT_ID;

/**************************************************************************//**
* \typedef disp_interrupt_callback_fn
*
* Callback for handling a display interrupt.
*
*    \param pCtx:    The context passed in to QVRServiceClient_SetDisplayInterruptConfig()
*    \param ts:      The timestamp of the hardware interrupt
******************************************************************************/
typedef void (*disp_interrupt_callback_fn)(void *pCtx, uint64_t ts);

/**************************************************************************//**
* This structure is passed to QVRServiceClient_SetDisplayInterruptConfig() and
* registers a callback to be notified whenever a VSYNC interrupt occurs.
******************************************************************************/
typedef struct qvrservice_vsync_interrupt_config_t {
    disp_interrupt_callback_fn cb;  /*!< Callback to call when interrupt occurs.
                                         Set to NULL to disable callbacks. */
    void *ctx;                      /*!< Context passed to callback */
} qvrservice_vsync_interrupt_config_t;

/**************************************************************************//**
* This structure is passed to QVRServiceClient_SetDisplayInterruptConfig() and
* serves two purposes: 1) configure lineptr interrupts, and 2) register a
* callback to be notified whenever a lineptr interrupt occurs.
*
* \note With the introduction of interrupt capture (see
* SetDisplayInterruptCapture()), enabling/disabling of callbacks is decoupled
* from configuring the lineptr interrupt. In other words, on devices that do
* not support callbacks, this structure still must be used to configure the
* lineptr interrupt.
******************************************************************************/
typedef struct qvrservice_lineptr_interrupt_config_t {
    disp_interrupt_callback_fn cb;  /*!< Callback to call when interrupt occurs.
                                         Set to NULL to disable callbacks. */
    void *ctx;                      /*!< Context passed to callback */
    uint32_t line;                  /*!< 1 to N, where N is the width of the
                                         display (in pixels), enables interrupts.
                                         Set to 0 to disable interrupts. */
} qvrservice_lineptr_interrupt_config_t;

/**************************************************************************//**
* This structure can be used to retrieve display interrupt timestamps.
******************************************************************************/
typedef struct qvrservice_ts_t {
    uint64_t ts;            /*!< timestamp in ns, relative to BOOTTIME */
    uint32_t count;         /*!< number of interrupts received since VR Mode started */
    uint32_t reserved;      /*!< reserved for future use */
} qvrservice_ts_t;

/**************************************************************************//**
* \enum QVRSERVICE_CLIENT_STATUS
*
* Client status values.
*
* \deprecated This enum is deprecated and is replaced by QVRSERVICE_CLIENT_NOTIFICATION.
*
*   \var STATUS_DISCONNECTED
*      The client was unexpectedly disconnected from server. If this occurs,
*      the QVRServiceClient object must be deleted.
*   \var STATUS_STATE_CHANGED
*      The VR Mode state has changed. arg1 will contain the new state,
*      arg2 will contain the previous state.
*   \var STATUS_SENSOR_ERROR
*      The sensor stack has detected an error. The arg parameters will be set
*      to useful values to help identify the error (TBD).
******************************************************************************/
typedef enum QVRSERVICE_CLIENT_STATUS {
    STATUS_DISCONNECTED = 0,
    STATUS_STATE_CHANGED,
    STATUS_SENSOR_ERROR,
    STATUS_MAX = 0xffffffff
} QVRSERVICE_CLIENT_STATUS;

/**************************************************************************//**
* \typedef client_status_callback_fn
*
* Callback for handling a client status event.
*
* \deprecated This callback is deprecated and is replaced by notification_callback_fn.
*
*    \param[in] pCtx:    The context passed in to SetClientStatusCallback().
*    \param[in] status:  Status value specifying the reason for the callback.
*    \param[in] arg1:    Argument 1 (depends on status).
*    \param[in] arg2:    Argument 2 (depends on status).
******************************************************************************/
typedef void (*client_status_callback_fn)(void *pCtx,
    QVRSERVICE_CLIENT_STATUS status, uint32_t arg1, uint32_t arg2);

/**************************************************************************//**
* QVRSERVICE_AUXILIARY_BRIGHTNESS_EXPECTED_PARAM
* -----------------------------------------------------------------------------
* Description
*   Can be used with GetParam() to get the expected smartviewer's brightness value.
*   Can be used with SetParam() to set the expected smartviewer's brightness value.
*   The return value of GetParam() range is [0, 100].
*   The parameter value of SetParam() range is [0, 100].
* Access
*   Read/Write
* Notes
*   None
******************************************************************************/
#define QVRSERVICE_AUXILIARY_BRIGHTNESS_EXPECTED_PARAM "auxiliary_brightness_expected"

/**************************************************************************//**
* QVRSERVICE_AUXILIARY_BRIGHTNESS_APPLIED_PARAM
* -----------------------------------------------------------------------------
* Description
*   Can be used with GetParam() to get the applied smartviewer's brightness value.
*   Can be used with SetParam() to set the applied smartviewer's brightness value.
*   The return value of GetParam() range is [0, 100].
*   The parameter value of SetParam() range is [0, 100].
* Access
*   Read/Write
* Notes
*   None
******************************************************************************/
#define QVRSERVICE_AUXILIARY_BRIGHTNESS_APPLIED_PARAM "auxiliary_brightness_applied"

/**************************************************************************//**
* QVRSERVICE_AUXILIARY_VOLUME_EXPECTED_PARAM
* -----------------------------------------------------------------------------
* Description
*   Can be used with GetParam() to get the expected smartviewer's volume value.
*   Can be used with SetParam() to set the expected smartviewer's volume value.
*   The return value of GetParam() range is [0, 100].
*   The parameter value of SetParam() range is [0, 100].
* Access
*   Read/Write
* Notes
*   None
******************************************************************************/
#define QVRSERVICE_AUXILIARY_VOLUME_EXPECTED_PARAM "auxiliary_volume_expected"

/**************************************************************************//**
* QVRSERVICE_AUXILIARY_VOLUME_APPLIED_PARAM
* -----------------------------------------------------------------------------
* Description
*   Can be used with GetParam() to get the applied smartviewer's volume value.
*   Can be used with SetParam() to set the applied smartviewer's volume value.
*   The return value of GetParam() range is [0, 100].
*   The parameter value of SetParam() range is [0, 100].
* Access
*   Read/Write
* Notes
*   None
******************************************************************************/
#define QVRSERVICE_AUXILIARY_VOLUME_APPLIED_PARAM "auxiliary_volume_applied"

/**************************************************************************//**
* QVRSERVICE_AUXILIARY_DEEP_SLEEP_TIME_OUT_PARAM
* -----------------------------------------------------------------------------
* Description
*   Can be used with GetParam() to get the smartviewer's deep sleep timeout value.
*   Can be used with SetParam() to set the smartviewer's deep sleep timeout value.
*   qvrservice won't maintain this value, the vaule will be set to default when
*   restart qvrservice. So client should restore and maintain it.
*   The unit of this parameter is second.
* Access
*   Read/Write
* Notes
*   None
******************************************************************************/
#define QVRSERVICE_AUXILIARY_DEEP_SLEEP_TIME_OUT_PARAM "auxiliary_deep_sleep_timeout"

/**************************************************************************//**
* \enum QVRSERVICE_CLIENT_NOTIFICATION
*   Types of client notifications.
*
* \deprecated NOTIFICATION_THERMAL_INFO is deprecated and is replaced by NOTIFICATION_THERMAL_INFO2
*
*   \var NOTIFICATION_DISCONNECTED
*      The client was unexpectedly disconnected from server. If this occurs,
*      the QVRServiceClient object must be deleted. notification_callback_fn
*      payload will be NULL.
*   \var NOTIFICATION_STATE_CHANGED
*      The VR Mode state has changed. notification_callback_fn payload will
*      contain qvrservice_state_notify_payload_t.
*   \var NOTIFICATION_SENSOR_ERROR
*      The sensor stack has detected an error. notification_callback_fn
*      payload will contain useful values to help identify the error(TBD).
*   \var NOTIFICATION_SUBSYSTEM_ERROR
*      The Subsystem has detected an error. notification_callback_fn
*      payload (qvrservice_subsystem_error_notify_payload_t struct)
*      will contain useful values to help identify the error.
*   \var NOTIFICATION_THERMAL_INFO
*      Thermal information from system. notification_callback_fn payload
*      will contain qvrservice_therm_notify_payload_t. Notification will stop
*      when VR mode stops.
*   \var NOTIFICATION_PROXIMITY_CHANGED
*      Proximity value from system. notification_callback_fn payload
*      will contain qvrservice_proximity_notify_payload_t.
*   \var NOTIFICATION_THERMAL_INFO2
*      Thermal information from system. notification_callback_fn payload
*      will contain qvrservice_therm_info2_payload_t. Notification will stop
*      when VR mode stops.
*   \var NOTIFICATION_DEEP_SLEEP_STATUS
*      The deep sleep status changed. notification_callback_fn payload will
*      contain qvrservice_auxiliary_notify_payload_t.
*      0 stands for exit deep sleep mode.
*      1 stands for enter deep sleep mode.
*   \var NOTIFICATION_BRIGHTNESS_CHANGE_EXPECTED
*      The brightness requester want to set.
*   \var NOTIFICATION_BRIGHTNESS_CHANGE_APPLIED
*      The brightness actually applied in system.
*   \var NOTIFICATION_VOLUME_CHANGE_EXPECTED
*      The volume requester want to set.
*   \var NOTIFICATION_VOLUME_CHANGE_APPLIED
*      The volume actually applied in system.
******************************************************************************/
typedef enum QVRSERVICE_CLIENT_NOTIFICATION {
    NOTIFICATION_DISCONNECTED = 0,
    NOTIFICATION_STATE_CHANGED,
    NOTIFICATION_SENSOR_ERROR,
    NOTIFICATION_THERMAL_INFO,
    NOTIFICATION_SUBSYSTEM_ERROR,
    NOTIFICATION_PROXIMITY_CHANGED,
    NOTIFICATION_THERMAL_INFO2,
    NOTIFICATION_DEEP_SLEEP_STATUS,
    NOTIFICATION_BRIGHTNESS_CHANGE_EXPECTED,
    NOTIFICATION_BRIGHTNESS_CHANGE_APPLIED,
    NOTIFICATION_VOLUME_CHANGE_EXPECTED,
    NOTIFICATION_VOLUME_CHANGE_APPLIED,
    NOTIFICATION_MAX
} QVRSERVICE_CLIENT_NOTIFICATION;

/**************************************************************************//**
* \typedef notification_callback_fn
*
* Callback for handling client notifications. Callback is registered using
* QVRServiceClient_RegisterForNotification().
*
*    \param[in] pCtx            The context passed to QVRServiceClient_RegisterForNotification().
*    \param[in] notification    Notification value specifying the reason for the
*                               callback.
*    \param[in] payload         Pointer to payload. Payload type depends on
*                               notification. Memory for the payload is allocated
*                               by the QVR service client and will be released when
*                               call back returns.
*    \param[in] payload_length  Length of valid data in payload.
******************************************************************************/
typedef void (*notification_callback_fn)(void *pCtx,
    QVRSERVICE_CLIENT_NOTIFICATION notification, void *payload,
    uint32_t payload_length);

/**************************************************************************//**
* Temperature levels.
******************************************************************************/
typedef enum QVRSERVICE_TEMP_LEVEL {
    TEMP_SAFE = 0,          /*!< Hardware is at a safe operating temperature. */
    TEMP_LEVEL_1,           /*!< Hardware is at a low level at which corrective actions can be taken. */
    TEMP_LEVEL_2,           /*!< Hardware is at a medium level at which corrective actions can be taken. */
    TEMP_LEVEL_3,           /*!< Hardware is at a high level at which corrective actions can be taken. */
    TEMP_CRITICAL           /*!< Hardware is at a critical operating temperature. The system may
                                 experience extreme drop in performance or shutdown. */
} QVRSERVICE_TEMP_LEVEL;

/**************************************************************************//**
* Mitigation actions.
******************************************************************************/
typedef enum QVRSERVICE_MITIGATION_ACTION {
    MIT_ACT_NONE,           /*!< No action needed. */
    MIT_ACT_INC,            /*!< Raise the FPS or Eye Buffer resolution. */
    MIT_ACT_DEC             /*!< Reduce the FPS or Eye Buffer resolution. */
} QVRSERVICE_MITIGATION_ACTION;

/**************************************************************************//**
* Hardware surface types.
******************************************************************************/
typedef enum QVRSERVICE_HW_TYPE {
    HW_TYPE_CPU = 1,        /*!< Hardware surface is CPU. */
    HW_TYPE_GPU,            /*!< Hardware surface is GPU. */
    HW_TYPE_SKIN,           /*!< Hardware surface is SKIN. */
    MAX_HW_TYPE
} QVRSERVICE_HW_TYPE;

/**************************************************************************//**
* Used to specify performance levels for HW_TYPE_CPU and HW_TYPE_GPU. The
* level for CPU and GPU can be selected as per the app requirements.
******************************************************************************/
typedef enum QVRSERVICE_PERF_LEVEL {
    PERF_LEVEL_DEFAULT = 0, /*!< Both CPU and GPU will run at default system settings
                                  if either HW_TYPE_CPU or HW_TYPE_GPU votes for this. */
    PERF_LEVEL_1,           /*!< The lowest performance level. */
    PERF_LEVEL_2,           /*!< The mid performance level. */
    PERF_LEVEL_3,           /*!< The highest performance level. */
    MAX_PERF_LEVEL
} QVRSERVICE_PERF_LEVEL;

/**************************************************************************//**
* This structure is used to pass the performance levels to
* QVRServiceClient_SetOperatingLevel().
******************************************************************************/
typedef struct qvrservice_perf_level_t {
    QVRSERVICE_HW_TYPE hw_type;       /*!< Hardware type to vote for. */
    QVRSERVICE_PERF_LEVEL perf_level; /*!< Peformance level requested. */
} qvrservice_perf_level_t;

/**************************************************************************//**
* Thread types.
******************************************************************************/
typedef enum QVRSERVICE_THREAD_TYPE {
    QVR_THREAD_TYPE_RENDER = 0, /*!< Thread renders frames to GPU. */
    QVR_THREAD_TYPE_WARP,       /*!< Thread sends time warp request to GPU. */
    QVR_THREAD_TYPE_CONTROLLER, /*!< Thread handles controller signals. */
    QVR_THREAD_TYPE_NORMAL,     /*!< Thread handles other VR app actions. */
    MAX_THREAD_TYPE
} QVRSERVICE_THREAD_TYPE;

// For backward compatibility.
#ifdef NORMAL
#undef NORMAL
#endif
#define RENDER     QVR_THREAD_TYPE_RENDER
#define WARP       QVR_THREAD_TYPE_WARP
#define CONTROLLER QVR_THREAD_TYPE_CONTROLLER
#define NORMAL     QVR_THREAD_TYPE_NORMAL

/**************************************************************************//**
* This structure is used to pass the payload to notification_callback_fn
* registered for NOTIFICATION_STATE_CHANGED.
******************************************************************************/
typedef struct qvrservice_state_notify_payload_t {
    QVRSERVICE_VRMODE_STATE new_state;      /*!< New state. */
    QVRSERVICE_VRMODE_STATE previous_state; /*!< Previous state. */
} qvrservice_state_notify_payload_t;

/**************************************************************************//**
*\deprecated This structure is deprecated and is replaced by qvrservice_therm_info2_payload_t
*
* This structure is used to pass the payload to notification_callback_fn
* registered for NOTIFICATION_THERMAL_INFO.
******************************************************************************/
typedef struct qvrservice_therm_notify_payload_t {
    QVRSERVICE_HW_TYPE           hw_type;     /*!< Hardware type for which
                                                   notification was triggered. */
    QVRSERVICE_TEMP_LEVEL        temp_level;  /*!< Temperature level of the hardware. */
    QVRSERVICE_MITIGATION_ACTION eye_buf_res; /*!< Mitigation action for eye
                                                   buffer resolution. */
    QVRSERVICE_MITIGATION_ACTION fps;         /*!< Mitigation action for FPS. */
    QVRSERVICE_MITIGATION_ACTION reserved[3]; /*!< Reserved for future use. */
} qvrservice_therm_notify_payload_t;

/**************************************************************************//**
* This structure is used to pass the payload to notification_callback_fn
* registered for NOTIFICATION_THERMAL_INFO2.
******************************************************************************/
typedef struct {
    QVRSERVICE_HW_TYPE           hw_type;      /*!< Hardware type for which
                                                    notification was triggered. */
    QVRSERVICE_TEMP_LEVEL        temp_level;   /*!< Temperature level of the hardware. */
    QVRSERVICE_MITIGATION_ACTION eye_buf_res;  /*!< Mitigation action for eye
                                                    buffer resolution. */
    QVRSERVICE_MITIGATION_ACTION fps;          /*!< Mitigation action for FPS. */
    float                        slop;         /*!< Rate of change of current temperature in celsius */
    float                        headroom;     /*!< Temperature headroom in celsius until thermal throttling occurs
                                                    This data depends on the system versions, may be nan  */
    char                         reserved[64]; /*!< Reserved for future use. */
} qvrservice_therm_info2_payload_t;

/**************************************************************************//**
* Defines the various subsystems that can generate a subsystem error.
******************************************************************************/
typedef enum QVRSERVICE_SUBSYSTEM_TYPE {
    QVRSERVICE_SUBSYSTEM_TYPE_SENSOR = 0,   /*!< Sensor subsystem */
    QVRSERVICE_SUBSYSTEM_TYPE_TRACKING      /*!< Tracking subsystem */
} QVRSERVICE_SUBSYSTEM_TYPE;

/**************************************************************************//**
* \enum QVRSERVICE_SUBSYSTEM_ERROR_STATE
*   Defines the subsystem error states.
*
*   \var QVRSERVICE_SUBSYSTEM_ERROR_STATE_RECOVERING
*      Indicates qvrservice is trying to recover from subsystem error.
*   \var QVRSERVICE_SUBSYSTEM_ERROR_STATE_RECOVERED
*      Indicates qvrservice is successfully recovered from subsystem error.
*   \var QVRSERVICE_SUBSYSTEM_ERROR_STATE_UNRECOVERABLE
*      Indicates qvrservice is unable to recover from the subsystem error,
*      device restart is required.
******************************************************************************/
typedef enum QVRSERVICE_SUBSYSTEM_ERROR_STATE {
    QVRSERVICE_SUBSYSTEM_ERROR_STATE_RECOVERING = 0,
    QVRSERVICE_SUBSYSTEM_ERROR_STATE_RECOVERED,
    QVRSERVICE_SUBSYSTEM_ERROR_STATE_UNRECOVERABLE
} QVRSERVICE_SUBSYSTEM_ERROR_STATE;

/**************************************************************************//**
* This structure is used to pass the payload to notification_callback_fn
* registered for NOTIFICATION_SUBSYSTEM_ERROR.
******************************************************************************/
typedef struct qvrservice_subsystem_error_notify_payload_t {
    QVRSERVICE_SUBSYSTEM_TYPE subsystem_type;     /*!< Subsystem type. */
    QVRSERVICE_SUBSYSTEM_ERROR_STATE error_state; /*!< Error state. */
    uint64_t param;                               /*!< if error_state is
                                                       QVRSERVICE_SUBSYSTEM_ERROR_STATE_RECOVERING,
                                                       then indicates expected
                                                       time to recovery in seconds. */
} qvrservice_subsystem_error_notify_payload_t;

/**************************************************************************//**
* This structure is used to pass the payload to notification_callback_fn
* registered for NOTIFICATION_PROXIMITY_CHANGED.
******************************************************************************/
typedef struct qvrservice_proximity_notify_payload_t {
    float scalar;                               /*!< distance in centimeters*/
} qvrservice_proximity_notify_payload_t;

/**************************************************************************//**
* This structure is used to pass the payload to notification_callback_fn
* registered for NOTIFICATION_DEEP_SLEEP_STATUS, NOTIFICATION_BRIGHTNESS_CHANGE_EXPECTED,
* NOTIFICATION_VOLUME_CHANGE_EXPECTED, NOTIFICATION_BRIGHTNESS_CHANGE_APPLIED and
* NOTIFICATION_VOLUME_CHANGE_APPLIED.
* For NOTIFICATION_DEEP_SLEEP_STATUS, value 0 means exit deep sleep mode, value
* 1 means enter deep sleep mode.
* For NOTIFICATION_BRIGHTNESS_CHANGE_EXPECTED, NOTIFICATION_VOLUME_CHANGE_EXPECTED,
* NOTIFICATION_BRIGHTNESS_CHANGE_APPLIED and NOTIFICATION_VOLUME_CHANGE_APPLIED
* value is a percentage value, range is [0, 100].
* For NOTIFICATION_BRIGHTNESS_CHANGE_EXPECTED and NOTIFICATION_VOLUME_CHANGE_EXPECTED
* Client who received this notifications need map the value according to system's
* min/max brightness or volume then apply it.
******************************************************************************/
typedef struct qvrservice_auxiliary_notify_payload_t {
    int32_t value;
} qvrservice_auxiliary_notify_payload_t;

/**************************************************************************//**
* \enum QVRSERVICE_EYE_TRACKING_MODE
*
* These values are used with the GetEyeTrackingMode() and SetEyeTrackingMode()
* APIs.
*
*   \var QVRSERVICE_EYE_TRACKING_MODE_NONE
*      Indicates that eye tracking is not supported or disabled.
*   \var QVRSERVICE_EYE_TRACKING_MODE_DUAL
*      Indicates that stereo eye tracking is supported or enabled.
******************************************************************************/
typedef enum QVRSERVICE_EYE_TRACKING_MODE {
    QVRSERVICE_EYE_TRACKING_MODE_NONE = 0,
    QVRSERVICE_EYE_TRACKING_MODE_DUAL = 1
} QVRSERVICE_EYE_TRACKING_MODE;

/**************************************************************************//**
* \enum QVRSERVICE_TRANSFORMATION_MATRIX_TYPE
*
* Types of transformation matrices.
*
* \var QVRSERVICE_LATE_LATCHING_PRE_TRANSFORMATION_MAT
*     Apply this matrix on the left side of the predicted pose in late latching.
* \var QVRSERVICE_LATE_LATCHING_POST_TRANSFORMATION_MAT
*     Apply this matrix on the right side of the predicted pose in late latching.
******************************************************************************/
typedef enum QVRSERVICE_TRANSFORMATION_MATRIX_TYPE {
    QVRSERVICE_LATE_LATCHING_PRE_TRANSFORMATION_MAT= 0,
    QVRSERVICE_LATE_LATCHING_POST_TRANSFORMATION_MAT,
    QVRSERVICE_MAX_TRANSFORMATION_MAT
} QVRSERVICE_TRANSFORMATION_MATRIX_TYPE;


typedef void* qvrservice_client_handle_t;

/**************************************************************************//**
* VR client ops table. Use QVRServiceClient_* APIs to access.
******************************************************************************/
typedef struct qvrservice_client_ops {

    qvrservice_client_handle_t (*Create)();

    void (*Destroy)(qvrservice_client_handle_t client);

    int32_t (*SetClientStatusCallback)(qvrservice_client_handle_t client,
        client_status_callback_fn cb, void *pCtx);

    QVRSERVICE_VRMODE_STATE (*GetVRMode)(qvrservice_client_handle_t client);

    int32_t (*StartVRMode)(qvrservice_client_handle_t client);

    int32_t (*StopVRMode)(qvrservice_client_handle_t client);

    int32_t (*GetTrackingMode)(qvrservice_client_handle_t client,
        QVRSERVICE_TRACKING_MODE *pCurrentMode,
        uint32_t *pSupportedModes);

    int32_t (*SetTrackingMode)(qvrservice_client_handle_t client,
         QVRSERVICE_TRACKING_MODE mode);

    int32_t (*SetDisplayInterruptConfig)(qvrservice_client_handle_t client,
        QVRSERVICE_DISP_INTERRUPT_ID id,
        void *pCfg, uint32_t cfgSize);

    int32_t (*SetThreadPriority)(qvrservice_client_handle_t client, int tid,
          int policy, int priority);

    int32_t (*GetParam)(qvrservice_client_handle_t client, const char* pName,
          uint32_t* pLen, char* pValue);

    int32_t (*SetParam)(qvrservice_client_handle_t client, const char* pName,
        const char* pValue);

    int32_t (*GetSensorRawData)(qvrservice_client_handle_t client,
        qvrservice_sensor_data_raw_t **ppData);

    int32_t (*GetHeadTrackingData)(qvrservice_client_handle_t client,
        qvrservice_head_tracking_data_t **ppData);

    int32_t (*GetRingBufferDescriptor)(qvrservice_client_handle_t client,
        QVRSERVICE_RING_BUFFER_ID id,
        qvrservice_ring_buffer_desc_t *pDesc);

    int32_t (*GetHistoricalHeadTrackingData)(
        qvrservice_client_handle_t client,
        qvrservice_head_tracking_data_t **ppData, int64_t timestampNs);

    int32_t (*SetDisplayInterruptCapture)(qvrservice_client_handle_t client,
        QVRSERVICE_DISP_INTERRUPT_ID id, uint32_t mode);

    int32_t (*GetDisplayInterruptTimestamp)(qvrservice_client_handle_t client,
        QVRSERVICE_DISP_INTERRUPT_ID id, qvrservice_ts_t** ppTs);

    int32_t (*RegisterForNotification)(qvrservice_client_handle_t client,
        QVRSERVICE_CLIENT_NOTIFICATION notification, notification_callback_fn cb,
        void *pCtx);

    int32_t (*SetThreadAttributesByType)(qvrservice_client_handle_t client,
        int tid, QVRSERVICE_THREAD_TYPE thread_type);

    int32_t (*SetOperatingLevel)(qvrservice_client_handle_t client,
        qvrservice_perf_level_t * perf_levels, uint32_t num_perf_levels,
        char *, uint32_t *);

    int32_t (*GetEyeTrackingMode)(qvrservice_client_handle_t client,
        uint32_t *pCurrentMode, uint32_t *pSupportedModes);

    int32_t (*SetEyeTrackingMode)(qvrservice_client_handle_t client,
         uint32_t mode);

    int32_t (*GetEyeTrackingData)(qvrservice_client_handle_t client,
        qvrservice_eye_tracking_data_t **ppData, int64_t timestampNs);

    int32_t (*ActivatePredictedHeadTrackingPoseElement)(qvrservice_client_handle_t client,
        int16_t* element_id, int64_t target_prediction_timestamp_ns);

    int32_t (*SetTransformationMatrix)(qvrservice_client_handle_t client,
        QVRSERVICE_TRANSFORMATION_MATRIX_TYPE type, float mat4x4[16]);

    int32_t (*GetHwTransforms)(qvrservice_client_handle_t client,
        uint32_t *pNumTransforms, qvrservice_hw_transform_t transforms[]);

    qvrplugin_data_t* (*GetPluginDataHandle)(qvrservice_client_handle_t client,
        const char* pluginName);

    void (*ReleasePluginDataHandle)(qvrservice_client_handle_t client,
        qvrplugin_data_t* handle);

    int32_t (*GetEyeTrackingDataWithFlags)(
        qvrservice_client_handle_t       client,
        qvrservice_eye_tracking_data_t **ppData,
        int64_t                          timestampNs,
        qvr_eye_tracking_data_flags_t    flags);

    int32_t (*GetPointCloud)(qvrservice_client_handle_t client,
        XrPointCloudQTI** point_cloud);

    int32_t (*ReleasePointCloud)(qvrservice_client_handle_t client,
        XrPointCloudQTI* point_cloud);

    int32_t (*GetFramePose)(qvrservice_client_handle_t client,
        XrFramePoseQTI** ppData);

    int32_t (*GetEyeTrackingCapabilities)(qvrservice_client_handle_t client,
        qvr_capabilities_flags_t *pCapabilities);

    qvrsync_ctrl_t* (*GetSyncCtrl)(qvrservice_client_handle_t client,
        QVR_SYNC_SOURCE syncSrc);

    int32_t (*ReleaseSyncCtrl)(qvrservice_client_handle_t client,
        qvrsync_ctrl_t* pSyncCtrl);

    qvrservice_class_t* (*GetClassHandle)(qvrservice_client_handle_t client,
        uint32_t classId, const char* uri);

    void (*ReleaseClassHandle)(qvrservice_client_handle_t client,
        qvrservice_class_t* handle);

    int32_t (*PauseVRMode)(qvrservice_client_handle_t client);

    int32_t (*ResumeVRMode)(qvrservice_client_handle_t client);

    //Reserved for future use
    void* reserved[64 - 26];

}qvrservice_client_ops_t;

typedef struct qvrservice_client{
    int api_version;
    qvrservice_client_ops_t* ops;
} qvrservice_client_t;

qvrservice_client_t* getQvrServiceClientInstance(void);

// client libraries
#define QVRSERVICE_CLIENT_LIB "libqvrservice_client.qti.so"     /*!< client library */
#define QVRSERVICE_CLIENT_LIB_LEGACY "libqvrservice_client.so"  /*!< legacy client library */

typedef struct qvrservice_client_helper_t {
    void* libHandle;
    qvrservice_client_t* client;
    qvrservice_client_handle_t clientHandle;
} qvrservice_client_helper_t;


/***************************************************************************//**
* Creates a VR client object.
*
* \return
*    qvrservice_client_helper_t*
* \par API level
*    1 or higher
* \par Timing requirements
*    This function needs to be called first before calling any other
*    QVRServiceClient functions. The client object received by calling
*    this function needs to be passed to the other QVRServiceClient
*    functions.
* \par Notes
*    None
**************************************************************************/
#ifdef __clang__
__attribute((no_sanitize("cfi")))
#endif
static inline qvrservice_client_helper_t* QVRServiceClient_Create()
{
    qvrservice_client_helper_t* me = (qvrservice_client_helper_t*)
                                    malloc(sizeof(qvrservice_client_helper_t));
    if(!me) return NULL;
    me->libHandle = dlopen( QVRSERVICE_CLIENT_LIB, RTLD_NOW);
    if (!me->libHandle) {
        me->libHandle = dlopen( QVRSERVICE_CLIENT_LIB_LEGACY, RTLD_NOW);
        if (!me->libHandle) {
            free(me);
            return NULL;
        }
    }

    typedef qvrservice_client_t* (*qvrservice_client_wrapper_fn)(void);
    qvrservice_client_wrapper_fn qvrServiceClient;

    qvrServiceClient = (qvrservice_client_wrapper_fn)dlsym(me->libHandle,
                                                "getQvrServiceClientInstance");
    if (!qvrServiceClient) {
        dlclose(me->libHandle);
        free(me);
        return NULL;
    }
    me->client = qvrServiceClient();
    me->clientHandle = me->client->ops->Create();
    if(!me->clientHandle){
        dlclose(me->libHandle);
        free(me);
        return NULL;
    }

    return me;
}

/***************************************************************************//**
* Destroys a VR client object.
*
*
* @param[in] me        qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \return
*    None
* \par API level
*    1 or higher
* \par Timing requirements
*    This function needs to be called when app shuts down. This
*    function will destroy the client object and therefore the same client
*    object can't be used in any other QVRServiceClient functions.
* \par Notes
*    None
**************************************************************************/
static inline void QVRServiceClient_Destroy(qvrservice_client_helper_t* me)
{
    if(!me) return;

    if (me->client->ops->Destroy){
        me->client->ops->Destroy( me->clientHandle);
    }

    if(me->libHandle ){
        dlclose(me->libHandle);
    }
    free(me);
    me = NULL;
}

/***************************************************************************//**
* Registers/deregisters a callback function with the VR client.
*
* \deprecated This API is deprecated and is replaced by
*    QVRServiceClient_RegisterForNotification()
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[in] cb         Callback function to be called to handle status events.
* @param[in] pCtx       Context to be passed to callback function.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    This function may be called at any time. The client will maintain only
*    one callback, so subsequent calls to this function will overwrite any
*    previous callbacks set. cb may be set to NULL to disable status
*    callbacks.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRServiceClient_SetClientStatusCallback(
   qvrservice_client_helper_t* me, client_status_callback_fn cb, void *pCtx)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetClientStatusCallback) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->SetClientStatusCallback(
        me->clientHandle, cb, pCtx);
}

/***************************************************************************//**
* Returned the current VR mode state.
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \return
*    Current VR mode state.
* \par API level
*    1 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    See \ref QVRSERVICE_VRMODE_STATE for more info
**************************************************************************/
static inline QVRSERVICE_VRMODE_STATE QVRServiceClient_GetVRMode(
    qvrservice_client_helper_t* me)
{
    if (!me) return VRMODE_UNSUPPORTED;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return VRMODE_UNSUPPORTED;
    if (me && me->client->ops->GetVRMode){
        return me->client->ops->GetVRMode(me->clientHandle);
    }
    return VRMODE_UNSUPPORTED;
}

/***************************************************************************//**
* Starts VR mode.
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    This function may be called at any time if the VR Mode is in the
*    VRMODE_STOPPED state. Calling this function while the VR Mode is in
*    any other state will return an error.
* \par Notes
*    The caller should not assume that VR Mode configuration (e.g. calls
*    to QVRServiceClient_SetParam(), QVRServiceClient_SetDisplayInterruptConfig(),
*    etc.) will persist through start/stop cycles. Therefore it is recommended
*    to always reconfigure VR Mode to suit the caller's use case prior to or
*    after calling QVRServiceClient_StartVRMode(). See \ref QVRSERVICE_VRMODE_STATE
*    for more information.
**************************************************************************/
static inline int32_t QVRServiceClient_StartVRMode(
    qvrservice_client_helper_t* me)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->StartVRMode) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->StartVRMode(me->clientHandle);
}

/***************************************************************************//**
* Stops VR mode.
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    See \ref QVRSERVICE_VRMODE_STATE for more information.
**************************************************************************/
static inline int32_t QVRServiceClient_StopVRMode(
    qvrservice_client_helper_t* me)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->StopVRMode) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->StopVRMode(me->clientHandle);
}

/***************************************************************************//**
* Returns the current and supported head tracking modes.
*
* @param[in] me                qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[out] pCurrentMode     If non-NULL, will be set to the currently configured
*                              tracking mode.
* @param[out] pSupportedModes  If non-NULL, will be set to a bitmask representing which
*                              tracking modes are supported by the device.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    None
***************************************************************************/
static inline int32_t QVRServiceClient_GetTrackingMode(
    qvrservice_client_helper_t* me,
    QVRSERVICE_TRACKING_MODE *pCurrentMode, uint32_t *pSupportedModes)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetTrackingMode) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->GetTrackingMode(
        me->clientHandle, pCurrentMode, pSupportedModes);
}

/***************************************************************************//**
* Sets the current head tracking mode.
*
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[in] mode       Tracking mode to set.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher.
* \par Timing requirements
*    This function must be called prior to calling QVRServiceClient_StartVRMode().
* \par Notes
*    None.
***************************************************************************/
static inline int32_t QVRServiceClient_SetTrackingMode(
    qvrservice_client_helper_t* me, QVRSERVICE_TRACKING_MODE mode)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetTrackingMode) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->SetTrackingMode(me->clientHandle, mode);
}

/***************************************************************************//**
* Configures display interrupts.
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[in] id         Display interrupt to use (see \ref QVRSERVICE_DISP_INTERRUPT_ID).
* @param[in] pCfg       Configuration information for display interrupt.
* @param[in] cfgSize    Size of the data passed in the pCfg pointer.
* \return
*    If the configuration information specified a callback but callbacks
*    are not supported on the device, then QVR_CALLBACK_NOT_SUPPORTED is
*    returned. Otherwise QVR_SUCCESS upon success or QVR_ERROR if
*    another error occurred.
* \par API level
*    1 or higher.
* \par Timing requirements
*    This function may be called at any time, but callbacks will only occur
*    when VR Mode is in the \ref VRMODE_STARTED state and the device supports
*    callbacks.
* \par Notes
*    If VR Mode is in the \ref VRMODE_STARTED state, this function will only
*    work if called by the same client that started VR Mode. If running
*    on a device that doesn't support callbacks, see
*    QVRServiceClient_SetDisplayInterruptCapture() and
*    QVRServiceClient_GetDisplayInterruptTimestamp() as an alternate method to
*    get display interrupt timestamps.
***************************************************************************/
static inline int32_t QVRServiceClient_SetDisplayInterruptConfig(
    qvrservice_client_helper_t* me, QVRSERVICE_DISP_INTERRUPT_ID id,
    void *pCfg, uint32_t cfgSize)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetDisplayInterruptConfig) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->SetDisplayInterruptConfig(
        me->clientHandle, id, pCfg, cfgSize);
}

/***************************************************************************//**
* Sets a thread priority.
*
* \deprecated This API is deprecated. Use QVRServiceClient_SetThreadPriority() instead.
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[in] tid        Thread ID of the thread whose priority will be changed.
* @param[in] policy    Scheduling policy to set. Use SCHED_FIFO or SCHED_RR for
*                       real-time performance.
* @param[in] priority   Priority value. For real-time policies, the values can be in
*                       the range of 1-99 (higher the number, higher the priority).
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
***************************************************************************/
static inline int32_t QVRServiceClient_SetThreadPriority(
    qvrservice_client_helper_t* me, int tid, int policy, int priority)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetThreadPriority) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->SetThreadPriority(
        me->clientHandle, tid, policy, priority);
}

/***************************************************************************//**
* Reads a string parameter value from the VR client.
*
* @param[in] me        qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* @param[in] pName     NUL-terminated name of the parameter length/value to retrieve.
*                      Must not be NULL.
* @param[in,out] pLen  If pValue is NULL, pLen will be filled in with the number of
*                      bytes (including the NUL terminator) required to hold the value
*                      of the parameter specified by pName. If pValue is non-NULL,
*                      pLen must point to an integer that represents the length of the
*                      buffer pointed to by pValue. pLen must not be NULL.
* @param[in]   pValue  Buffer to receive value.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The pValue buffer will be filled in up to *pLen bytes (including NUL),
*    so this may result in truncation of the value if the required length
*    is larger than the size passed in pLen.
**************************************************************************/
static inline int32_t QVRServiceClient_GetParam(
    qvrservice_client_helper_t* me, const char* pName, uint32_t* pLen,
    char* pValue)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetParam) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetParam(
        me->clientHandle, pName, pLen, pValue);
}

/***************************************************************************//**
* Writes a string parameter value to the VR client
*
* @param[in] me      qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* @param[in] pName   NUL-terminated name of parameter value to set. Must not be
*                    NULL.
* @param[in] pValue  NUL-terminated value. Must not be NULL.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher.
* \par Timing requirements
*    Some parameters may only be able to be set while VR Mode is in the
*    \ref VRMODE_STOPPED state. Additionally, some parameters may only be able
*    to be set while VR Mode is in the \ref VRMODE_STARTED state if this
*    function is called from the same client that started VR Mode.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRServiceClient_SetParam(
    qvrservice_client_helper_t* me, const char* pName, const char* pValue)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetParam) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->SetParam(
        me->clientHandle, pName, pValue);
}

/***************************************************************************//**
* Reads raw sensor data from the VR client.
*
* @param[in] me        qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[out] ppData   Address of pointer to qvrservice_sensor_data_raw_t structure
*                      that will contain the raw data values.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRServiceClient_GetSensorRawData(
    qvrservice_client_helper_t* me, qvrservice_sensor_data_raw_t **ppData)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetSensorRawData) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetSensorRawData(
        me->clientHandle, ppData);
}

/***************************************************************************//**
* Reads the latest head tracking data from the VR client.
*
* @param[in] me       qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* @param[out] ppData  Address of pointer to qvrservice_head_tracking_data_t
*                     structure that will be updated to point to the pose
*                     data within a ring buffer. See Notes for more information.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    1 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    To minimize latency, a ring buffer is used to hold many poses, and a
*    call to this function will set ppData to point to a single pose within
*    that ring buffer. The buffer will be sized appropriately to provide at
*    a minimum 80 milliseconds worth of poses. As such, the data pointed to
*    by ppData may change after that amount of time passes.
**************************************************************************/
static inline int32_t QVRServiceClient_GetHeadTrackingData(
    qvrservice_client_helper_t* me,
    qvrservice_head_tracking_data_t **ppData)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetHeadTrackingData) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetHeadTrackingData(
        me->clientHandle, ppData);
}

/***************************************************************************//**
* Allows the retrieval of a ring buffer descriptor.
*
* @param[in] me          qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* @param[in] id          ID of the ring buffer descriptor to retrieve
* @param[out] pDesc      Ring buffer descriptor
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The caller needs to read the index value from ring buffer's index offset,
*    then should access the element from the buffer's ring offset for
*    retrieving the latest updated element in the buffer.
*
*    The caller may obtain the memory address of the ring buffer by invoking
*    mmap using the pDesc returned attributes:
*       - file descriptor (pDesc->fd)
*       - size (pDesc->size)
*
*    If the caller invokes mmap, the caller must invoke munmap when the buffer
*    is no longer needed.
*
*    Regardless of whether mmap was invoked, the caller must always close the
*    returned file descriptor (pDesc->fd) when the ring buffer is no longer
*    needed.

**************************************************************************/
static inline int32_t QVRServiceClient_GetRingBufferDescriptor(
    qvrservice_client_helper_t* me, QVRSERVICE_RING_BUFFER_ID id,
    qvrservice_ring_buffer_desc_t *pDesc)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetRingBufferDescriptor) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetRingBufferDescriptor(
        me->clientHandle, id, pDesc);
}

/***************************************************************************//**
* Reads head tracking data from the VR client that occurred in the past.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* \param[out]
*    ppData      Address of pointer to qvrservice_head_tracking_data_t
*                structure that will be updated to point to the pose data within
*                a ring buffer. See Notes for more information.
* \param[in]
*    timestampNs  The time in nanoseconds for the pose requested (which must be
*                a time in the past). This value must be in the Tracker (Qtimer)
*                time domain. A value of 0 requests the most recent pose.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*  - To minimize latency, a ring buffer is used to hold many poses, and a
*    call to this function will set ppData to point to a single pose within
*    that ring buffer. The buffer will be sized appropriately to provide at
*    a minimum 80 milliseconds worth of poses. As such, the data pointed to
*    by ppData may change after that amount of time passes.
*  - Use \ref QVRSERVICE_TRACKER_ANDROID_OFFSET_NS for converting between
*    Tracker and Android time domains.
**************************************************************************/
static inline int32_t QVRServiceClient_GetHistoricalHeadTrackingData(
    qvrservice_client_helper_t* me,
    qvrservice_head_tracking_data_t **ppData, int64_t timestampNs)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetHistoricalHeadTrackingData) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetHistoricalHeadTrackingData(
        me->clientHandle, ppData, timestampNs);
}

/***************************************************************************//**
* Enables/disables display interrupt capture.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    id          Display interrupt to configure capture on (see
*                \ref QVRSERVICE_DISP_INTERRUPT_ID).
* \param[in]
*    mode        Set to 1 to enable capture, 0 to disable.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher.
* \par Timing requirements
*    None. This function may be called at any time, but timestamp capture
*    will only occur while VR Mode is in the VRMODE_STARTED state.
* \par Notes
*    QVRServiceClient_SetDisplayInterruptCapture() is meant to be used on
*    devices that do not support display interrupt client callbacks. See
*    QVRServiceClient_GetDisplayInterruptTimestamp() and
*    QVRServiceClient_SetDisplayInterruptConfig() for more information.
**************************************************************************/
static inline int32_t QVRServiceClient_SetDisplayInterruptCapture(
    qvrservice_client_helper_t* me, QVRSERVICE_DISP_INTERRUPT_ID id,
    uint32_t mode)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetDisplayInterruptCapture) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->SetDisplayInterruptCapture(
        me->clientHandle, id, mode);
}

/***************************************************************************//**
* Retrieves a display interrupt timestamp from the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    id          Display interrupt to retrieve the timestamp for (see
*                \ref QVRSERVICE_DISP_INTERRUPT_ID).
* \param[out]
*    ppTs        Pointer to qvrservice_ts_t to receive the most recent
*                timestamp for the specified display interrupt.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    2 or higher.
* \par Timing requirements
*    None. This function may be called at any time. However, timestamps
*    will only be updated if QVRServiceClient_SetDisplayInterruptCapture()
*    has been called to enable timestamp capture and VR Mode is in the
*    \ref VRMODE_STARTED state.
* \par Notes
*    QVRServiceClient_GetDisplayInterruptTimestamp() is meant to be used on
*    devices that do not support display interrupt client callbacks. See
*    QVRServiceClient_SetDisplayInterruptCapture() and
*    QVRServiceClient_SetDisplayInterruptConfig() for more information.
**************************************************************************/
static inline int32_t QVRServiceClient_GetDisplayInterruptTimestamp(
    qvrservice_client_helper_t* me, QVRSERVICE_DISP_INTERRUPT_ID id,
    qvrservice_ts_t** ppTs)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetDisplayInterruptTimestamp) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetDisplayInterruptTimestamp(
        me->clientHandle, id, ppTs);
}

/***************************************************************************//**
* Registers/deregisters a callback function with the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    notification  \ref QVRSERVICE_CLIENT_NOTIFICATION to register for.
* \param[in]
*    cb          Callback function of type notification_callback_fn.
* \param[in]
*    pCtx        Context to be passed to callback function.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    3 or higher.
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    The client will maintain only one callback, so subsequent calls to
*    this function will overwrite the previous callback. cb may be set to
*    NULL to disable notification callbacks.
***************************************************************************/
static inline int32_t QVRServiceClient_RegisterForNotification(
    qvrservice_client_helper_t* me, QVRSERVICE_CLIENT_NOTIFICATION notification,
    notification_callback_fn cb, void *pCtx)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->RegisterForNotification) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->RegisterForNotification(
        me->clientHandle, notification, cb, pCtx);
}

/***************************************************************************//**
* Set thread attributes for VR-related threads.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    tid         Thread ID of the thread whose type is set.
* \param[in]
*    thread_type  \ref QVRSERVICE_THREAD_TYPE of the thread.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    3 or higher.
* \par Timing requirements
*    None.
* \par Notes
*    Thread attributes can include scheduler priority, scheduler
*    policy and cpu affinity.
***************************************************************************/
static inline int32_t QVRServiceClient_SetThreadAttributesByType(
    qvrservice_client_helper_t* me, int tid, QVRSERVICE_THREAD_TYPE thread_type)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetThreadAttributesByType) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->SetThreadAttributesByType(
        me->clientHandle, tid, thread_type);
}

/***************************************************************************//**
* Used to set the operating levels of the system.
*
* \param[in]
*    me            qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    perf_levels   Array of qvrservice_perf_level_t.
* \param[in]
*    num_perf_levels  num of values in perf_levels array.
* \param
*    reserved1     Reserved for future use. Set to NULL.
* \param
*    reserved2     Reserved for future use. Set to NULL.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    3 or higher.
* \par Timing requirements
*     VR mode must be started before calling this function.
* \par Notes
*    All the operating levels set on the previous call will be
*    revoked in subsequent calls.
***************************************************************************/
static inline int32_t QVRServiceClient_SetOperatingLevel(
    qvrservice_client_helper_t* me, qvrservice_perf_level_t * perf_levels,
    uint32_t num_perf_levels, char * reserved1, uint32_t * reserved2)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetOperatingLevel) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->SetOperatingLevel(
        me->clientHandle, perf_levels, num_perf_levels, reserved1, reserved2);
}

/***************************************************************************//**
* Retrieves the current and supported eye tracking modes from the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[out]
*    pCurrentMode     If non-NULL, will be set to the currently configured eye
*                     tracking mode.
* \param[out]
*    pSupportedModes  If non-NULL, will be set to a bitmask representing which
*                     eye tracking modes are supported by the device.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    3 or higher.
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    See \ref QVRSERVICE_EYE_TRACKING_MODE for more information.
***************************************************************************/
static inline int32_t QVRServiceClient_GetEyeTrackingMode(
    qvrservice_client_helper_t* me,
    uint32_t *pCurrentMode, uint32_t *pSupportedModes)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetEyeTrackingMode) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetEyeTrackingMode(
        me->clientHandle, pCurrentMode, pSupportedModes);
}

/***************************************************************************//**
* Sets the current eye tracking mode of the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    mode        Eye tracking mode to set.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    3 or higher.
* \par Timing requirements
*    Typically this function must be called prior to calling
*    QVRServiceClient_StartVRMode(). However, some devices may allow calling
*    this function before or after calling StartVRMode().
* \par Notes
*    None.
***************************************************************************/
static inline int32_t QVRServiceClient_SetEyeTrackingMode(
    qvrservice_client_helper_t* me, uint32_t mode)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetEyeTrackingMode) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->SetEyeTrackingMode(me->clientHandle, mode);
}

/***************************************************************************//**
* Retrieves the latest or historical eye tracking data from the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[out]
*    ppData      Address of pointer to qvrservice_eye_tracking_data_t
*                structure that will be updated to point to the pose
*                data within a ring buffer. See Notes for more information.
* \param[in]
*    timestampNs  The time in nanoseconds for the eye pose requested
*                (which must be a time in the past). This value must
*                be in the Tracker (Qtimer) time domain. A value of
*                0 requests the most recent pose.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    3 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*  - To minimize latency, a ring buffer is used to hold many poses, and a
*    call to this function will set ppData to point to a single pose within
*    that ring buffer. The buffer will be sized appropriately to provide at
*    a minimum 0.5 seconds worth of poses. As such, the data pointed to by
*    ppData may change after that amount of time passes.
*  - Use \ref QVRSERVICE_TRACKER_ANDROID_OFFSET_NS for converting between
*    Tracker and Android time domains.
**************************************************************************/
static inline int32_t QVRServiceClient_GetEyeTrackingData(
    qvrservice_client_helper_t      *me,
    qvrservice_eye_tracking_data_t **ppData,
    int64_t                          timestampNs)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetEyeTrackingData) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetEyeTrackingData(
        me->clientHandle, ppData, timestampNs);
}

/**********************************************************************//**
* Retrieves the latest or historical eye tracking data from the VR client.
*
* \param[in]
*    me           qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* \param[out]
*    ppData       Address of pointer to qvrservice_eye_tracking_data_t
*                 structure that will be updated to point to the pose
*                 data within a ring buffer. See Notes for more information.
* \param[in]
*    timestampNs  The time in nanoseconds for the eye pose requested
*                 (which must be a time in the past). This value must
*                 be in the Tracker (Qtimer) time domain. A value of
*                 0 requests the most recent pose.
* \param[in]
*    flags        Flags. See qvr_eye_tracking_data_flags_t for values.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    5 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*  - To minimize latency, a ring buffer is used to hold many poses, and a
*    call to this function will set ppData to point to a single pose within
*    that ring buffer. The buffer will be sized appropriately to provide at
*    a minimum 0.5 seconds worth of poses. As such, the data pointed to by
*    ppData may change after that amount of time passes.
*  - Use QVRSERVICE_TRACKER_ANDROID_OFFSET_NS for converting between
*    Tracker and Android time domains.
**************************************************************************/
static inline int32_t QVRServiceClient_GetEyeTrackingDataWithFlags(
    qvrservice_client_helper_t      *me,
    qvrservice_eye_tracking_data_t **ppData,
    int64_t                          timestampNs,
    qvr_eye_tracking_data_flags_t    flags)
{
    if(!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_5) return QVR_API_NOT_SUPPORTED;
    if(!me->client->ops->GetEyeTrackingData) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetEyeTrackingDataWithFlags(
        me->clientHandle, ppData, timestampNs, flags);
}

/***************************************************************************//**
* Activates a predicted head tracking pose for forward prediction/late latching.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* \param[in,out]
*    element_id  Element id activated on predicted pose ring buffer. If -1, then
*                a new element will be activated with predicted pose and
*                element_id will be updated, otherwise, only forward prediction
*                delay will be updated for the requested element.
* \param[in]
*    target_prediction_timestamp_ns
*                targetted prediction time in QTIMER based NS resolution. This
*                will be used to compute the amount of forward prediction delay
*                that needs to be applied.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    4 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*   This API allows the caller to have a fixed buffer in shared memory
*   (\ref RING_BUFFER_PREDICTED_HEAD_POSE) that is continuously updated with
*   predictive pose for the target prediction delay. When the current time
*   reaches the target prediction timestamp, tracker stops updating the buffer.
***************************************************************************/
static inline int32_t QVRServiceClient_ActivatePredictedHeadTrackingPoseElement(
    qvrservice_client_helper_t* me, int16_t* element_id, int64_t target_prediction_timestamp_ns)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_4) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->ActivatePredictedHeadTrackingPoseElement) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->ActivatePredictedHeadTrackingPoseElement(
        me->clientHandle, element_id, target_prediction_timestamp_ns);
}

/***************************************************************************//**
* Configures a transformation matrix on the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    type        Transformation matrix type.
* \param[in]
*    mat4x4      Transformation matrix to be applied.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    4 or higher
* \par Timing requirements.
*    None. This function may be called at any time.
* \par Notes
*    For late latching, the transformation matrixes are applied as follows:
*      \ref QVRSERVICE_LATE_LATCHING_PRE_TRANSFORMATION_MAT * predicted_pose * \ref QVRSERVICE_LATE_LATCHING_POST_TRANSFORMATION_MAT
***************************************************************************/
static inline int32_t QVRServiceClient_SetTransformationMatrix(
    qvrservice_client_helper_t* me, QVRSERVICE_TRANSFORMATION_MATRIX_TYPE type, float mat4x4[16])
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_4) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->SetTransformationMatrix) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->SetTransformationMatrix(
        me->clientHandle, type, mat4x4);
}

/***************************************************************************//**
* Retrieves the list of hardware transforms from the VR client
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in,out]
*    pnTransforms  If transforms is NULL, pnTransforms will be
*                  filled in with the number of transforms that may
*                  be returned. If transforms is non-NULL, then
*                  pnTransforms must describe how many elements the
*                  transforms array can hold. pnTransforms must not be NULL.
* \param[out]
*    transforms    Pointer to array to receive hardware transforms.
* \return
*    0 upon success, -1 otherwise
* \par API level
*    4 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRServiceClient_GetHwTransforms(
    qvrservice_client_helper_t* me,
    uint32_t *pnTransforms, qvrservice_hw_transform_t transforms[])
{
    if(!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_4) return QVR_API_NOT_SUPPORTED;
    if(!me->client->ops->GetHwTransforms) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetHwTransforms(
        me->clientHandle, pnTransforms, transforms);
}

/***************************************************************************//**
* Helper function to get a specific hardware transform. Caller is to provide
* a qvrservice_hw_transform_t with the 'from' and 'to' members set to the
* desired values. If successful, the transformation matrix will be filled
* in, otherwise an error is returned: QVR_ERROR indicates out of memory,
* QVR_INVALID_PARAM means the transform was not found.
**************************************************************************/
static inline int32_t QVRServiceClient_GetHwTransform(
    qvrservice_client_helper_t* me, qvrservice_hw_transform_t *transform)
{
    qvrservice_hw_transform_t *t;
    uint32_t nt;
    int ret;
    if (NULL == transform) return QVR_INVALID_PARAM;
    ret = QVRServiceClient_GetHwTransforms(me, &nt, NULL);
    if (ret < 0) return ret;
    t = (qvrservice_hw_transform_t*) malloc(nt * sizeof(*t));
    if (NULL == t) return QVR_ERROR;
    if (0 == QVRServiceClient_GetHwTransforms(me, &nt, t)) {
        qvrservice_hw_transform_t* pt = t;
        for (uint32_t i=0; i<nt; i++, pt++) {
            if (transform->from == pt->from && transform->to == pt->to) {
                memcpy(transform->m, pt->m, sizeof(pt->m));
                free(t);
                return QVR_SUCCESS;
            }
        }
    }
    free(t);
    return QVR_INVALID_PARAM;
}

/***************************************************************************//**
* Obtains a QVRService PluginData object from the VR client.
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    pluginName  a valid QVRService plugin name defined by a constant
*                QVRSERVICE_PLUGIN_xxx.
* \return
*    Returns a qvrplugin_data_t* if successful, NULL otherwise
* \par API level
*    4 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The caller must be sure to call QVRServiceClient_ReleasePluginDataHandle()
*    when the plugin data is no longer needed. Failure to do so prior to calling
*    QVRServiceClient_Destroy() on the client may result in the client resources
*    not being completely deallocated.
*    Refer to QVRPluginData.h for a description of the PluginData functions.
**************************************************************************/
static inline qvrplugin_data_t* QVRServiceClient_GetPluginDataHandle(
    qvrservice_client_helper_t* me, const char* pluginName)
{
    if (!me) return NULL;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_4) return NULL;
    if (!me->client->ops->GetPluginDataHandle) return NULL;
    return me->client->ops->GetPluginDataHandle(me->clientHandle, pluginName);
}

/***************************************************************************//**
* Releases a PluginData object obtained by QVRServiceClient_GetPluginDataHandle().
*
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    handle      qvrplugin_data_t* returned by QVRServiceClient_GetPluginDataHandle().
* \return
*    None
* \par API level
*    4 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
**************************************************************************/
static inline void QVRServiceClient_ReleasePluginDataHandle(
    qvrservice_client_helper_t* me, qvrplugin_data_t* handle)
{
    if(!me) return;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_4) return;
    if(!me->client->ops->ReleasePluginDataHandle) return;
    me->client->ops->ReleasePluginDataHandle(me->clientHandle, handle);
}

/***************************************************************************//**
* Returns a point cloud.
*
* \param[in]
*    me           qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[out]
*    point_cloud  the point cloud.
* \return
*    0 upon success, error code otherwise.
* \par API level
*    5 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The caller must call ReleasePointCloud() when the
*    point cloud data is no longer needed.
**************************************************************************/
static inline int32_t QVRServiceClient_GetPointCloud(
    qvrservice_client_helper_t* me, XrPointCloudQTI** point_cloud)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_5) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetPointCloud) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetPointCloud(me->clientHandle, point_cloud);
}

/***************************************************************************//**
* Releases a point cloud.
*
* \param[in]
*    me           qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* \param[in]
*    point_cloud  point cloud returned by QVRServiceClient_GetPointCloud()
* \return
*    0 upon success, error code otherwise
* \par API level
*    5 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRServiceClient_ReleasePointCloud(
    qvrservice_client_helper_t* me, XrPointCloudQTI* point_cloud)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_5) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->ReleasePointCloud) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->ReleasePointCloud(me->clientHandle, point_cloud);
}

/**********************************************************************//**
* Returns the camera frame pose.
*
* \param[in]
*    me       qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* \param[out]
*    ppData   Address of pointer to XrFramePoseQTI
*             structure that will be updated to point to the pose
*             data within a ring buffer. See Notes for more information.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    5 or higher.
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*  - To minimize latency, a ring buffer is used to hold many poses, and a
*    call to this function will set ppData to point to a single pose within
*    that ring buffer.
**************************************************************************/
static inline int32_t QVRServiceClient_GetFramePose(
    qvrservice_client_helper_t* me, XrFramePoseQTI** ppData)
{
    if (!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_5) return QVR_API_NOT_SUPPORTED;
    if (!me->client->ops->GetFramePose) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetFramePose(me->clientHandle, ppData);
}

/**********************************************************************//**
* Returns eye tracking capabilities
*
* @param[in] me             qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* @param[in] pCapabilities  Pointer to qvr_capabilities_flags_t that will be
*                           updated with a bitmask to reflect the eye tracking
*                           gaze capabilities of the system. See Notes for
*                           more information.
* \return
*    Returns 0 upon success, error code otherwise
* \par API level
*    5 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    See QVR_CAPABILITY_GAZE_* for the list of available capabilities.
**************************************************************************/
static inline int32_t QVRServiceClient_GetEyeTrackingCapabilities(
    qvrservice_client_helper_t *me, qvr_capabilities_flags_t *pCapabilities)
{
    if(!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_6) return QVR_API_NOT_SUPPORTED;
    if(!me->client->ops->GetEyeTrackingCapabilities) return QVR_API_NOT_SUPPORTED;
    return me->client->ops->GetEyeTrackingCapabilities(
        me->clientHandle, pCapabilities);
}

/**********************************************************************//**
* Obtains a sync control object for the given sync source.
*
* @param[in] me        qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* @param[in] syncSrc   Sync source for which to obtain a sync control.
*                      See QVR_SYNC_SOURCE for list of valid sources.
* \return
*    Returns a qvrsync_ctrl_t* if successful, NULL otherwise
* \par API level
*    6 or higher
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline qvrsync_ctrl_t* QVRServiceClient_GetSyncCtrl(
    qvrservice_client_helper_t* me, QVR_SYNC_SOURCE syncSrc)
{
    if(!me) return NULL;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_6) return NULL;
    if(!me->client->ops->GetSyncCtrl) return NULL;

    return me->client->ops->GetSyncCtrl(
        me->clientHandle, syncSrc);
}

/**********************************************************************//**
* Releases a sync control object.
*
* @param[in] me         qvrservice_client_helper_t* returned by QVRServiceClient_Create()
* @param[in] pSyncCtrl  qvrsync_ctrl_t* returned by QVRServiceClient_GetSyncCtrl().
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    6 or higher.
* \par Timing requirements
*    This function may be called at any time.
* \par Notes
*    None.
**************************************************************************/
static inline int32_t QVRServiceClient_ReleaseSyncCtrl(
    qvrservice_client_helper_t* me, qvrsync_ctrl_t* pSyncCtrl)
{
    if(!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_6) return QVR_API_NOT_SUPPORTED;
    if(!me->client->ops->ReleaseSyncCtrl) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->ReleaseSyncCtrl(
        me->clientHandle, pSyncCtrl);
}

/***************************************************************************//**
* GetClassHandle()
* ------------------------------------------------------------------------------
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    classId     Id of a QVRService component/module
* \param[in]
*    uri         Key/Value pair of the QVRService component/module.  This parameter
*                can be NULL if the classId can uniquely identify the component,
*                e.g., no subcomponents.
* \return
*    Returns a handle to a QVRService component if successful, NULL
*    otherwise
* \par API level
*    7 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    The caller must call ReleaseClassHandle() when the component's
*    functionality is no longer needed. Failure to do so prior to calling
*    Destroy() on the client may result in the client resources not being
*    completely deallocated.
*******************************************************************************/
static inline qvrservice_class_t* QVRServiceClient_GetClassHandle(
    qvrservice_client_helper_t* me, uint32_t classId, const char* uri)
{
    if (!me) return NULL;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_7) return NULL;
    if (!me->client->ops->GetClassHandle) return NULL;
    return me->client->ops->GetClassHandle(me->clientHandle, classId, uri);
}

/***************************************************************************//**
* ReleaseClassHandle()
* ------------------------------------------------------------------------------
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \param[in]
*    handle      Handle returned by QVRServiceClient_GetClassHandle().
* \return
*    None
* \par API level
*    7 or higher
* \par Timing requirements
*    None. This function may be called at any time.
* \par Notes
*    None
*******************************************************************************/
static inline void QVRServiceClient_ReleaseClassHandle(
    qvrservice_client_helper_t* me, qvrservice_class_t* handle)
{
    if (!me) return;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_7) return;
    if (!me->client->ops->ReleaseClassHandle) return;
    return me->client->ops->ReleaseClassHandle(me->clientHandle, handle);
}

/**********************************************************************//**
* PauseVRMode()
* -------------------------------------------------------------------------
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* API level
*    8 or higher
* Timing requirements
*    This function may be called at any time if the VR Mode is in the
*    VRMODE_STARTED state. Calling this function while the VR Mode is in
*    any other state will return an error.
**************************************************************************/
static inline int32_t QVRServiceClient_PauseVRMode(
    qvrservice_client_helper_t* me)
{
    if(!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_8) return QVR_API_NOT_SUPPORTED;
    if(!me->client->ops->PauseVRMode) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->PauseVRMode(me->clientHandle);
}

/**********************************************************************//**
* ResumeVRMode()
* -------------------------------------------------------------------------
* \param[in]
*    me          qvrservice_client_helper_t* returned by QVRServiceClient_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* API level
*    8 or higher
* Timing requirements
*    This function may be called at any time if the VR Mode is in the
*    VRMODE_PAUSED state. Calling this function while the VR Mode is in
*    any other state will return an error.
**************************************************************************/
static inline int32_t QVRServiceClient_ResumeVRMode(
    qvrservice_client_helper_t* me)
{
    if(!me) return QVR_INVALID_PARAM;
    if (me->client->api_version < QVRSERVICECLIENT_API_VERSION_8) return QVR_API_NOT_SUPPORTED;
    if(!me->client->ops->ResumeVRMode) return QVR_API_NOT_SUPPORTED;

    return me->client->ops->ResumeVRMode(me->clientHandle);
}

// Debug helpers
static inline const char* QVRServiceClient_StateToName(QVRSERVICE_VRMODE_STATE state)
{
    switch(state){
    QVR_CASE_RETURN_STR(VRMODE_STARTING);
    QVR_CASE_RETURN_STR(VRMODE_STARTED);
    QVR_CASE_RETURN_STR(VRMODE_STOPPING);
    QVR_CASE_RETURN_STR(VRMODE_STOPPED);
    QVR_CASE_RETURN_STR(VRMODE_HEADLESS);
    QVR_CASE_RETURN_STR(VRMODE_PAUSING);
    QVR_CASE_RETURN_STR(VRMODE_PAUSED);
    QVR_CASE_RETURN_STR(VRMODE_RESUMING);
    default: return "unknown";
    }
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVRSERVICE_CLIENT_H */
