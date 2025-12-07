/******************************************************************************/
/*! \file  QVRServicePlugin.h  */
/*
* Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/
#ifndef QVRSERVICE_PLUGIN_H
#define QVRSERVICE_PLUGIN_H

/**
 * @addtogroup qvr_service_plugin
 * @{
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include "QVRTypes.h"

/**************************************************************************//**
* \enum QVRSERVICE_PLUGIN_API_VERSION
* Defines the API versions of this interface. The api_version member of the
* qvr_plugin_t object retrieved from the plugin library must
* be set to the API version that matches its functionality.
******************************************************************************/
typedef enum QVRSERVICE_PLUGIN_API_VERSION {
    QVRSERVICE_PLUGIN_API_VERSION_INVALID = 0,
    QVRSERVICE_PLUGIN_API_VERSION_1 = 1,
    QVRSERVICE_PLUGIN_API_VERSION_2 = 2,
    QVRSERVICE_PLUGIN_API_VERSION_3 = 3,
    QVRSERVICE_PLUGIN_API_VERSION_4 = 4,
} QVRSERVICE_PLUGIN_API_VERSION;

/**************************************************************************//**
* \enum QVRSERVICE_PLUGIN_ID
* Defines the ID of the plugin.
******************************************************************************/
typedef enum QVRSERVICE_PLUGIN_ID {
    QVRSERVICE_PLUGIN_ID_NONE,
    QVRSERVICE_PLUGIN_ID_EYE_TRACKING = 1,
} QVRSERVICE_PLUGIN_ID;

/**
 *   Can be used with SetParam() to set the configuration path for the device.
 *   The path contains read only files pertinent to plug-in configuration.
 */
#define QVRSERVICE_PLUGIN_CONFIG_PATH "qvr-plugin-config-path"

/**
*   Can be used with SetParam() to set the data path for the device.
*   The path can be used to write any data by the plug-in.
*/
#define QVRSERVICE_PLUGIN_DATA_PATH "qvr-plugin-data-path"

/**
*   Can be used with SetParam() to set the calibration path for the device.
*   The path contains read only files pertinent to device-specific calibration
*   (i.e. eye camera calibration).
*/
#define QVRSERVICE_PLUGIN_CALIBRATION_PATH "qvr-plugin-calibration-path"

/**
*   Can be used with GetParam() to get vendor string from the plug-in.
*/
#define QVRSERVICE_PLUGIN_VENDOR_STRING "qvr-plugin-vendor-string"

/**
*   Can be used with GetParam() to get the plug-in version.
*/
#define QVRSERVICE_PLUGIN_VERSION "qvr-plugin-version"

/**************************************************************************//**
* \enum QVRSERVICE_PLUGIN_ERROR_STATE
*   \var QVRSERVICE_PLUGIN_ERROR_STATE_RECOVERING
*      Indicates the plugin is trying to recover from an error.
*   \var QVRSERVICE_PLUGIN_ERROR_STATE_RECOVERED
*      Indicates the plugin hass successfully recovered from an error.
*   \var QVRSERVICE_PLUGIN_ERROR_STATE_UNRECOVERABLE
*      Indicates the plugin is unable to recover from the error.
******************************************************************************/
typedef enum QVRSERVICE_PLUGIN_ERROR_STATE {
    QVRSERVICE_PLUGIN_ERROR_STATE_RECOVERING = 0,
    QVRSERVICE_PLUGIN_ERROR_STATE_RECOVERED,
    QVRSERVICE_PLUGIN_ERROR_STATE_UNRECOVERABLE
} QVRSERVICE_PLUGIN_ERROR_STATE;

typedef struct qvr_plugin_callbacks_ops {

    void (*NotifyError)(void *pCtx, QVRSERVICE_PLUGIN_ERROR_STATE error_state,
        uint64_t dwParam);

    //Reserved for future use
    void* reserved[64 - 0];

} qvr_plugin_callbacks_ops_t;

typedef struct qvr_plugin_callbacks {
    QVRSERVICE_PLUGIN_API_VERSION api_version;
    void *ctx;
    qvr_plugin_callbacks_ops_t* ops;
} qvr_plugin_callbacks_t;


typedef struct qvr_plugin_ops {

    int32_t (*Init)(const char* key);

    int32_t (*Deinit)(void);

    int32_t (*Start)(void);

    int32_t (*Stop)(void);

    int32_t (*GetParam)(const char* pName, uint32_t* pLen, char* pValue);

    int32_t (*SetParam)(const char* pName, const char* pValue);

    int32_t (*Create)(qvr_plugin_param_t pParams[], int32_t nParams,
        qvrservice_hw_transform_t pTransforms[], int32_t nTransforms,
        qvr_plugin_callbacks_t *callbacks);

    void (*Destroy)();

    int32_t (*SetTransform)(qvrservice_hw_transform_t* pTransform);

    int32_t (*GetData)(char* pControl, uint32_t controlLen,
        char* pPayload, uint32_t* pPayloadLen);

    int32_t (*SetData)(const char* pControl, uint32_t controlLen,
        const char* pPayload, uint32_t payloadLen);

    int32_t (*GetFd)(const char* pName, QVR_PLUGIN_DATA_FD_MODE mode,
        int32_t *pFd);

    int32_t (*ReleaseFd)(int32_t fd);

    int32_t (*GetCapabilities)(qvr_capabilities_flags_t *pCapabilities);

    int32_t (*Pause)(void);

    int32_t (*Resume)(void);

    //Reserved for future use
    void* reserved[64 - 8];

} qvr_plugin_ops_t;

typedef struct qvr_plugin{
    QVRSERVICE_PLUGIN_API_VERSION api_version;
    QVRSERVICE_PLUGIN_ID plugin_id;
    qvr_plugin_ops_t*    ops;
    qvr_plugin_info_t    info;
} qvr_plugin_t;

qvr_plugin_t* getQVRPluginInstance(void);

/**
* The following are QVRServicePluginCallbacks helpers
* used by the plug-in to notify the VR service that an error has occurred.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \param[in]
*    error_state QVRSERVICE_PLUGIN_ERROR_STATE.
* \param[in]
*    param     Error-specific parameter (see Notes).
* \return
*    None
* \par API level
*    2 or higher.
* \par Notes
*   -# If error_state is QVRSERVICE_PLUGIN_ERROR_STATE_RECOVERING, then
*      dwParam indicates the expected time to recovery in seconds.
*   -# If error_state is QVRSERVICE_PLUGIN_ERROR_STATE_UNRECOVERABLE, then
*      the plugin may be destroyed and re-created, but not until after
*      the call to NotifyError() has returned.
******************************************************************************/
static __inline void QVRServicePluginCallbacks_NotifyError(
    qvr_plugin_callbacks_t *me, QVRSERVICE_PLUGIN_ERROR_STATE error_state,
    uint64_t param)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_2) return;
    if (NULL == me->ops || NULL == me->ops->NotifyError) return;
    me->ops->NotifyError(me->ctx, error_state, param);
}

/**
* The following are QVRServicePlugin helpers
* that initialize the plugin for normal operation.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \param[in]
*    key        NULL terminated string for privilege checking. This
*                 key may be used to create privileged clients.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    1 or higher.
* \par Notes
*    Memory pointed to by the pKey parameter is only valid for the duration
*    of the call to Init(), so the plugin should make a copy of pKey if it
*    needs to retain it.
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: no contraints on the plugin
*      - Power: the plugin must remain in a low power state
**************************************************************************/
static __inline int32_t QVRServicePlugin_Init(qvr_plugin_t *me,
    const char* key)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Init) return QVR_API_NOT_SUPPORTED;
    return me->ops->Init(key);
}

/**********************************************************************//**
* Deinitializes the plugin. The plugin should relinquish any resources
* obtained in Init().
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Notes
*    None
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: the plugin must minimize its memory usage
*      - Power: the plugin must remain in a low power state
**************************************************************************/
static __inline int32_t QVRServicePlugin_Deinit(qvr_plugin_t *me)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Deinit) return QVR_API_NOT_SUPPORTED;
    return me->ops->Deinit();
}

/**********************************************************************//**
* Starts the plug-in normal operation.
* This function will be called when an application calls StartVRMode() on
* the VR service.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Notes
*    None
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: no contraints on the plugin
*      - Power: no contraints on the plugin
**************************************************************************/
static __inline int32_t QVRServicePlugin_Start(qvr_plugin_t *me)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Start) return QVR_API_NOT_SUPPORTED;
    return me->ops->Start();
}

/**********************************************************************//**
* This function will be called when an application calls StopVRMode() on
* the VR service.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Notes
*    None
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: no contraints on the plugin
*      - Power: the plugin must remain in a low power state
**************************************************************************/
static __inline int32_t QVRServicePlugin_Stop(qvr_plugin_t *me)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Stop) return QVR_API_NOT_SUPPORTED;
    return me->ops->Stop();
}

/**********************************************************************//**
* Gets a specific parameter value from the plugin.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \param[in]
*    pName       NULL-terminated name of the parameter length/value to
*                 retrieve. Must not be NULL.
* \param[in,out]
*    pLen        If pValue is NULL, pLen will be filled in with the
*                 number of bytes (including the NUL terminator) required
*                 to hold the value of the parameter specified by pName.
*                 If pValue is non-NULL, pLen must point to an integer
*                 that represents the length of the buffer pointed to by
*                 pValue. pLen must not be NULL.
* \param[in]
*    pValue      Buffer to receive value.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Notes
*    The pValue buffer will be filled in up to *pLen bytes (including NUL),
*    so this may result in truncation of the value if the required length
*    is larger than the size passed in pLen.
**************************************************************************/
static __inline int32_t QVRServicePlugin_GetParam(qvr_plugin_t *me,
    const char* pName, uint32_t* pLen, char* pValue)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->GetParam) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetParam(pName, pLen, pValue);
}

/**********************************************************************//**
* Sets a specific parameter value on the plugin.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \param[in]
*    pName       NULL-terminated name of parameter value to set. Must not
*                 be NULL.
* \param[in]
*    pValue      NULL-terminated value. Must not be NULL.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    1 or higher
* \par Notes
*    Memory pointed to by the pTransform parameter is only valid for the
*    duration of the call to SetTransform(), so the plugin should make a
*    copy of pTransform if it needs to retain it.
**************************************************************************/
static __inline int32_t QVRServicePlugin_SetParam(qvr_plugin_t *me,
    const char* pName, const char* pValue)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_1) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->SetParam) return QVR_API_NOT_SUPPORTED;
    return me->ops->SetParam(pName, pValue);
}

/**********************************************************************//**
* Creates the plugin.
*
* \param[in]
*    me           The context passed to QVRServicePlugin_Create().
* \param[in]
*    params       An array of qvr_plugin_param_t elements.
* \param[in]
*    nParams      Number of elements in the params array.
* @param[in]
*    transforms   Transform array.
* \param[in]
*    nTransforms  Number of elements in the transforms array.
* \param[in]
*    callbacks    Pointer to callbacks object to provide notifications.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Notes
*    1. This function may be called at boot time, so the plugin can use this
*       function to do very basic initialization, but resource usage must be
*       kept to an absolute minimum until Init() is called.
*    2. Memory pointed to by the array pointers is only valid for the
*       duration of the call to Create(), so the plugin should make copies
*       if it needs to retain the contents.
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: the plugin must minimize its memory usage
*      - Power: the plugin must remain in a low power state
**************************************************************************/
static __inline int32_t QVRServicePlugin_Create(qvr_plugin_t *me,
    qvr_plugin_param_t* params, int32_t nParams,
    qvrservice_hw_transform_t transforms[], int32_t nTransforms,
    qvr_plugin_callbacks_t *callbacks)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Create) return QVR_API_NOT_SUPPORTED;
    return me->ops->Create(params, nParams, transforms, nTransforms, callbacks);
}

/**********************************************************************//**
* Destroys the plugin. The plugin should relinquish any resources it
* obtained in Create().
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \return
*    None
* \par API level
*    2 or higher
* \par Notes
*    None
**************************************************************************/
static __inline void QVRServicePlugin_Destroy(qvr_plugin_t *me)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_2) return;
    if (NULL == me->ops || NULL == me->ops->Destroy) return;
    me->ops->Destroy();
}

/**********************************************************************//**
* Sets or updates a hardware transform.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \param[in]
*    transform    The hardware transform to set or update.
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    2 or higher
* \par Notes
*    Memory pointed to by the pTransform parameter is only valid for the
*    duration of the call to SetTransform(), so the plugin should make a
*    copy of pTransform if it needs to retain it.
**************************************************************************/
static __inline int32_t QVRServicePlugin_SetTransform(qvr_plugin_t *me,
    qvrservice_hw_transform_t* transform)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->SetTransform) return QVR_API_NOT_SUPPORTED;
    return me->ops->SetTransform(transform);
}

/**********************************************************************//**
* This function will be called when a QVRService client needs to query the
* plugin for some data, e.g. user profile data.
*
* \param[in]
*    me                  The context passed to QVRServicePlugin_Create().
* \param[in]
*    pControl            Byte array (of length controlLen)
*                         allowing the client to convey control information
*                         associated with the data payload. This is an
*                         optional array and thus pControl may be NULL.
* \param[in]
*    controlLen          controlLen is an integer that represents the
*                         length of the byte array pointed to by pControl.
*                         controlLen may be zero when the control array is
*                         not necessary.
* \param[out]
*    pPayload            Byte array (of length payloadLen) for the data to
*                         be received. May be NULL on the first call when
*                         querying the length.
* \param[in,out]
*    pPayloadLen         If pPayload is NULL, pPayloadLen will be filled
*                         in with the number of bytes required to hold the
*                         value of the parameter specified by pPayload. If
*                         pPayload is non-NULL, pPayloadLen must point to an
*                         integer that represents the length of the buffer
*                         pointed to by pPayload. pPayloadLen must not be
*                         NULL.
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if payload parameters are
*                              not following the non-NULL requirement
*            QVR_ERROR for other error
* \par API level
*    2 or higher
* \par Notes
*    This API is invoked by QVRService on behalf of a remote QVRService
*    client that uses the QVRPluginData_GetData() API.
*    The pPayload array will be filled in up to *pPayloadLen bytes
*    so this may result in truncation of the payload if the required length
*    is larger than the size passed in pPayloadLen.
**************************************************************************/
static __inline int32_t QVRServicePlugin_GetData(qvr_plugin_t *me,
    char* pControl, uint32_t controlLen,
    char* pPayload, uint32_t* pPayloadLen)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->GetData) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetData(pControl, controlLen, pPayload, pPayloadLen);
}

/**********************************************************************//**
* This function will be called when a QVRService client needs to configure
* the plugin with some data, e.g. user profile data.
*
* \param[in]
*    me              The context passed to QVRServicePlugin_Create().
* \param[in]
*    pControl        Byte array (of length controlLen) allowing the client
*                     to convey control information associated with the data
*                     payload. This is an optional array and thus pControl
*                     may be NULL
* \param[in]
*    controlLen      controlLen is an integer that represents the length of
*                     the byte array pointed to by pControl. controlLen is
*                     ignored if pControl is NULL.
* \param[in]
*    pPayload        Byte array (of length payloadLen) representing the
*                     data to be configured. Must not be NULL.
* \param[in]
*    payloadLen      payloadLen is an integer that represents the length of
*                     the byte array pointed to by pPayload. Must not be
*                     zero.
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if payload parameters are
*                              not following the non-NULL requirement
*            QVR_ERROR for other error
* \par API level
*    2 or higher
* \par Notes
*    This API is invoked by QVRService on behalf of a remote QVRService
*    client which would use the QVRPluginData_SetData API.
**************************************************************************/
static __inline int32_t QVRServicePlugin_SetData(qvr_plugin_t *me,
    const char* pControl, uint32_t controlLen,
    const char* pPayload, uint32_t payloadLen)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->SetData) return QVR_API_NOT_SUPPORTED;
    return me->ops->SetData(pControl, controlLen, pPayload, payloadLen);
}

/**********************************************************************//**
* This function is used to get a file descriptor from the plugin for
* transferring data.
*
* \param[in]
*    me              The context passed to QVRServicePlugin_Create().
* \param[in]
*    pName           String identifier for the fd to retrieve. Will not
*                     be NULL.
* \param[in]
*    mode            Mode the fd should be opened in. See
*                     QVR_PLUGIN_DATA_FD_MODE for more info.
* \param[out]
*    pFd             Returned fd. Will not be NULL.
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if the fd name is unknown
*            QVR_BUSY if the call cannot be completed at this time, e.g.
*                     due to concurrency issues.
*            QVR_ERROR for other error
* \par API level
*    3 or higher
* \par Timing requirements
*    None
* \par Notes
*    If a request for a file descriptor is made with the mode
*    QVR_PLUGIN_DATA_FD_MODE_READ, then the fd returned by the plugin must
*    support the poll() operation.
**************************************************************************/
static __inline int32_t QVRServicePlugin_GetFd(qvr_plugin_t *me,
    const char* pName, QVR_PLUGIN_DATA_FD_MODE mode, int32_t *pFd)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->GetFd) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetFd(pName, mode, pFd);
}

/**********************************************************************//**
* This function is used to release a file descriptor obtained using GetFd().
*
* \param[in]
*    me              The context passed to QVRServicePlugin_Create().
* \param[in]
*    fd              fd returned by QVRServicePlugin_GetFd().
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if the fd provided is unknown
*            QVR_BUSY if the call cannot be completed at this time, e.g.
*                     due to concurrency issues.
*            QVR_ERROR for other error
* \par API level
*    3 or higher
* \par Timing requirements
*    None
* \par Notes
*    None
**************************************************************************/
static __inline int32_t QVRServicePlugin_ReleaseFd(qvr_plugin_t *me, int32_t fd)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->ReleaseFd) return QVR_API_NOT_SUPPORTED;
    return me->ops->ReleaseFd(fd);
}

/**********************************************************************//**
* Get capabilities
*
* \param[in]
*    me                  The context passed to QVRServicePlugin_Create().
* \param[out]
*    pCapabilities       Pointer to qvr_capabilities_flags_t that will be
*                         updated with a bitmask to reflect the plugin's
*                         capabilities. See Notes for more info.
* \return
*    Returns 0 upon success, error code otherwise
* \par API level
*    3 or higher
* \par Timing requirements
*    This function will be called once immediately after the call to
*    Create().
* \par Notes
*    See QVR_CAPABILITY_* for the list of available capabilities.
**************************************************************************/
static __inline int32_t QVRServicePlugin_GetCapabilities(qvr_plugin_t *me,
    qvr_capabilities_flags_t *pCapabilities)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_3) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->GetCapabilities) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetCapabilities(pCapabilities);
}

/**********************************************************************//**
* This function will be called when an application calls PauseVRMode() on
* the VR service.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    4 or higher
* \par Notes
*    None
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: no contraints on the plugin
*      - Power: the plugin must remain in a low power state
**************************************************************************/
static __inline int32_t QVRServicePlugin_Pause(qvr_plugin_t *me)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_4) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Pause) return QVR_API_NOT_SUPPORTED;
    return me->ops->Pause();
}

/**********************************************************************//**
* Resume the plug-in normal operation.
* This function will be called when an application calls ResumeVRMode() on
* the VR service.
*
* \param[in]
*    me          The context passed to QVRServicePlugin_Create().
* \return
*    Returns 0 upon success, -1 otherwise
* \par API level
*    4 or higher
* \par Notes
*    None
* \par Resource requirements
*    After this function is called, the plugin must conform to the following
*    contraints:
*      - Memory: no contraints on the plugin
*      - Power: no contraints on the plugin
**************************************************************************/
static __inline int32_t QVRServicePlugin_Resume(qvr_plugin_t *me)
{
    if (me->api_version < QVRSERVICE_PLUGIN_API_VERSION_4) return QVR_API_NOT_SUPPORTED;
    if (NULL == me->ops || NULL == me->ops->Resume) return QVR_API_NOT_SUPPORTED;
    return me->ops->Resume();
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVRSERVICE_PLUGIN_H */
