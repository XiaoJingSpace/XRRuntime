/******************************************************************************/
/*! \file  QVRPluginData.h */
/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/
#ifndef QVRPLUGIN_DATA_H
#define QVRPLUGIN_DATA_H


/**
 * @addtogroup qvr_plugin_data
 * @{
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include "QVRTypes.h"


/**************************************************************************//**
* \page page-plugindata VR Service PluginData API
* APIs to communicate with QVRPluginData. Typical call flow is as follows:
*   -# Create QVRServiceClient object
*   -# Call QVRServiceClient_GetPluginDataHandle() to get a
*      QVRPluginData handle
*   -# Call QVRPluginData_GetPluginDataInfo() to get information on the
*      plugin vendor/version and its data version, which can be used to
*      determine data format, protocol, etc., of the plugin data.
*   -# Call QVRPluginData_GetData()/QVRPluginData_SetData()
*   -# Call QVRServiceClient_ReleasePluginDataHandle() to release the
*      QVRPluginData handle.
******************************************************************************/

/**************************************************************************//**
* QVRPLUGINDATA_API_VERSION
*
* Defines the API versions of this interface. The api_version member of the
* qvrplugin_data_t structure must be set to accurately reflect the version
* of the API that the implementation supports.
******************************************************************************/
typedef enum QVRPLUGINDATA_API_VERSION {
    QVRPLUGINDATA_API_VERSION_1 = 1,     /*!< API version 1 */
    QVRPLUGINDATA_API_VERSION_2 = 2,     /*!< API version 2 */
} QVRPLUGINDATA_API_VERSION;

typedef void* qvrplugin_data_handle_t;


typedef struct qvrplugin_data_ops {

    int32_t (*GetPluginDataInfo)(qvrplugin_data_handle_t handle,
        qvr_plugin_info_t* pInfo);

    int32_t (*GetData)(qvrplugin_data_handle_t handle,
        const char* pControl, uint32_t controlLen,
        char* pPayload, uint32_t* pPayloadLen);

    int32_t (*SetData)(qvrplugin_data_handle_t handle,
        const char* pControl, uint32_t controlLen,
        const char* pPayload, uint32_t payloadLen);

    int32_t (*GetMaxFdCount)(qvrplugin_data_handle_t handle, uint32_t *pCount);

    int32_t (*GetFd)(qvrplugin_data_handle_t handle,
        const char* pName, QVR_PLUGIN_DATA_FD_MODE mode, int32_t *pFd);

    int32_t (*ReleaseFd)(qvrplugin_data_handle_t handle, int32_t fd);

    //Reserved for future use
    void* reserved[64 - 3];

} qvrplugin_data_ops_t;

typedef struct qvrplugin_data {
    int api_version;
    qvrplugin_data_handle_t handle;
    qvrplugin_data_ops_t* ops;
} qvrplugin_data_t;

/**********************************************************************//**
* Returns information about a plugin.
*
* \param[in]
*    me               qvrplugin_data_t* returned by
*                      QVRServiceClient_GetPluginData().
* \param[out]
*    pluginInfo       Pointer to a qvr_plugin_info_t that will be filled
*                      in with information about the plugin data. Must not
*                      be NULL.
* \return
*    - QVR_SUCCESS upon success,
*    - QVR_API_NOT_SUPPORTED if not supported
*    - QVR_INVALID_PARAM if payload parameters are not following the non-NULL requirement
*    - QVR_ERROR for other error
* API level
*    1 or higher
* Timing requirements
*    None
* Notes
*    None
**************************************************************************/
static inline int32_t QVRPluginData_GetPluginDataInfo(qvrplugin_data_t* me,
    qvr_plugin_info_t* pluginInfo)
{
    if(!me) return QVR_INVALID_PARAM;
    if(!me->ops->GetPluginDataInfo) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetPluginDataInfo(me->handle, pluginInfo);
}

/**********************************************************************//**
* This function can be used to query the plugin for some data, e.g. reading
* a user profile.
*
* \param[in]
*    me                  qvrplugin_data_t* returned by
*                         QVRServiceClient_GetPluginData().
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
*    - QVR_SUCCESS upon success,
*    - QVR_API_NOT_SUPPORTED if not supported
*    - QVR_INVALID_PARAM if payload parameters are not following the non-NULL requirement
*    - QVR_BUSY if the call cannot be completed at this time, e.g. due to concurrency issues.
*    - QVR_ERROR for other error
* \par API level
*    1 or higher
* \par Timing requirements
*    None
* \par Notes
*    The pPayload array will be filled in up to *pPayloadLen bytes
*    so this may result in truncation of the payload if the required length
*    is larger than the size passed in pPayloadLen.
**************************************************************************/
static inline int32_t QVRPluginData_GetData(qvrplugin_data_t* me,
    const char* pControl, uint32_t controlLen,
    char* pPayload, uint32_t* pPayloadLen)
{
    if(!me) return QVR_INVALID_PARAM;
    if(!me->ops->GetData) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetData(
        me->handle, pControl, controlLen, pPayload, pPayloadLen);
}

/**********************************************************************//**
* This function can be used to write data to the plugin, e.g. setting a
* user profile.
*
* \param[in]
*    me              qvrplugin_data_t* returned by
*                     QVRServiceClient_GetPluginData().
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
*    - QVR_SUCCESS upon success,
*    - QVR_API_NOT_SUPPORTED if not supported
*    - QVR_INVALID_PARAM if payload parameters are
*                        not following the non-NULL requirement
*    - QVR_BUSY if the call cannot be completed at this time, e.g.
*               due to concurrency issues.
*    - QVR_ERROR for other error
* \par API level
*    1 or higher
* \par Timing requirements
*    None
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRPluginData_SetData(qvrplugin_data_t* me,
    const char* pControl, uint32_t controlLen,
    const char* pPayload, uint32_t payloadLen)
{
    if(!me) return QVR_INVALID_PARAM;
    if(!me->ops->SetData) return QVR_API_NOT_SUPPORTED;
    return me->ops->SetData(
        me->handle, pControl, controlLen, pPayload, payloadLen);
}

/**********************************************************************//**
* GetMaxFdCount()
* -------------------------------------------------------------------------
* This function is used to get the maximum number of concurrent file
* descriptors allowed to be acquired from the plugin.
*
* \param[in]
*    me        qvrplugin_data_handle_t returned by
*               QVRServiceClient_GetPluginData().
* \param[out]
*    pCount    Returned count. Must not be NULL.
*
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if payload parameters are
*                              not following the non-NULL requirement
*            QVR_ERROR for other error
* \par API level
*    2 or higher
* \par Timing requirements
*    None
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRPluginData_GetMaxFdCount(qvrplugin_data_t *me,
    uint32_t *pCount)
{
    if(!me) return QVR_INVALID_PARAM;
    if (!me->ops->GetMaxFdCount || me->api_version < QVRPLUGINDATA_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetMaxFdCount(me->handle, pCount);
}

/**********************************************************************//**
* GetFd()
* -------------------------------------------------------------------------
* This function is used to get a file descriptor from the plugin for
* transferring data.
*
* \param[in]
*    me         qvrplugin_data_handle_t returned by
*                QVRServiceClient_GetPluginData().
* \param[in]
*    pName      String identifier for the fd to retrieve. Must not
*                be NULL.
* \param[in]
*    mode       Mode the fd should be opened in. See
*                QVR_PLUGIN_DATA_FD_MODE for more info.
* \param[out]
*    pFd        Returned fd. Must not be NULL.
*
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if payload parameters are
*                              not following the non-NULL requirement
*            QVR_BUSY if the call cannot be completed at this time, e.g.
*                     the implementation is unable to open any more fds on the
*                     plugin's behalf due to exceeding the maximum limit
*                     specified by QVRServiceClient_GetMaxFdCount().
*            QVR_ERROR for other error
* \par API level
*    2 or higher
* \par Timing requirements
*    None
* \par Notes
*    When using fds acquired from a plugin, callers must be sure to check
*    for error conditions which may indicate that the fd is no longer valid.
*    This can occur when accessing plugins over a remote connection.
**************************************************************************/
static inline int32_t QVRPluginData_GetFd(qvrplugin_data_t *me,
    const char* pName, QVR_PLUGIN_DATA_FD_MODE mode, int32_t *pFd)
{
    if(!me) return QVR_INVALID_PARAM;
    if (!me->ops->GetFd || me->api_version < QVRPLUGINDATA_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    return me->ops->GetFd(me->handle, pName, mode, pFd);
}

/**********************************************************************//**
* ReleaseFd()
* -------------------------------------------------------------------------
* This function is used to release a file descriptor obtained using GetFd().
*
* \param[in]
*    me         qvrplugin_data_handle_t returned by
*                QVRServiceClient_GetPluginData().
* \param[in]
*    fd         fd returned by QVRPluginData_GetFd().
*
* \return
*    Returns QVR_SUCCESS upon success,
*            QVR_API_NOT_SUPPORTED if not supported
*            QVR_INVALID_PARAM if the fd provided is unknown
*            QVR_BUSY if the call cannot be completed at this time, e.g.
*                     due to concurrency issues.
*            QVR_ERROR for other error
* \par API level
*    2 or higher
* \par Timing requirements
*    None
* \par Notes
*    None
**************************************************************************/
static inline int32_t QVRPluginData_ReleaseFd(qvrplugin_data_t *me, int32_t fd)
{
    if(!me) return QVR_INVALID_PARAM;
    if (!me->ops->ReleaseFd || me->api_version < QVRPLUGINDATA_API_VERSION_2) return QVR_API_NOT_SUPPORTED;
    return me->ops->ReleaseFd(me->handle, fd);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVRPLUGIN_DATA_H */
