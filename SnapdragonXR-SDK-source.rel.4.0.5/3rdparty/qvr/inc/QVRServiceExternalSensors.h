/******************************************************************************/
/*! \file  QVRServiceExternalSensors.h  */
/*
* Copyright (c) 2016, 2019 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#ifndef QVRSERVICEEXTERNALSENSOR_H
#define QVRSERVICEEXTERNALSENSOR_H

/**
 * @addtogroup qvr_service_external_sensors
 * @{
 */

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

/**
* Defines the API versions of this interface. The api_version member of the
* qvr_external_sensors_t object retrieved from the external sensor library must
* be set to the API version that matches its functionality.
*/
typedef enum QVRSERVICEEXTERNALSENSOR_API_VERSION {
    QVRSERVICEEXTERNALSENSOR_API_VERSION_1 = 1,
} QVRSERVICEEXTERNALSENSOR_API_VERSION;

/**
* Defines the sensors available for VR service from external source
*/
typedef enum sensor_type_e
{
    ACCEL = 0,
    GYRO,
} sensor_type_e;


/**
* This structure contains a single sensor sample.
*/
typedef struct sensor_sample_t
{
    sensor_type_e type;  /**< Type of sensor sample. */
    uint64_t ts;         /**< Timestamp of the data in Android boottime time domain. */
    float sample[3];     /**< Three-element vector representing the sensor data. The data must
*                            follow the Android Specification in Android potrait orientation:
*                           - Accel: m/s/s
*                           - Gyro: rad/s */
} sensor_sample_t;


/**
* Defines the types of errors that may be encountered while using an external
* sensor.
*/
typedef enum error_e
{
    UNKNOWN_ERROR
} error_e;


/**
* Callback for handling data received from the external sensor
*   - pCtx: The context passed to start().
*   - sample: Pointer to the sensor sample.
* \return
*    Returns 0 upon success, -1 otherwise
******************************************************************************/
typedef int (*data_ready_callback_fn)
(
    void* pCtx,
    const sensor_sample_t* sample
);


/**-
* Callback for handling an error from the external sensor
*   - pCtx: The context passed to start()
*   - code: Error code
******************************************************************************/
typedef void (*handle_error_callback_fn)
(
    void* pCtx,
    error_e code
);


typedef struct qvr_external_sensor_ops {

    /**
    * This function will be called during VR service initialization.
    *
    * \return
    *    Returns 0 upon success, -1 otherwise
    */
    int (*Init)(void);

    /**
    * This function will be called during VR service deinitialization.
    *
    * \return
    *    Returns 0 upon success, -1 otherwise
    */
    int (*Deinit)(void);

    /**
    * This function will be called when an application calls StartVRMode() on
    * the VR service.
    *
    * @param[in] data_ready_cb    Function to be called by the external sensor when
    *                             new sensor data is available.
    * @param[in] handle_error_cb  Function to be called by the external sensor if
    *                             an error is detected.
    * @param[in] pCtx             Context to be passed to callback functions
    * @return
    *    Returns 0 upon success, -1 otherwise
    **************************************************************************/
    int (*Start)(data_ready_callback_fn data_ready_cb,
        handle_error_callback_fn handle_error_cb, void* pCtx);

    /**
    * This function will be called when an application calls StopVRMode() on
    * the VR service.
    *
    * @return
    *    Returns 0 upon success, -1 otherwise
    * Notes
    *    After this call, the external sensor must not call the callback
    *    functions provided in Start().
    **************************************************************************/
    int (*Stop)(void);

    /**
    * This function will be called to retrieve the configured sensor rate.
    *
    * @param[in] type    Sensor type for which the rate is being queried.
    * @param[out] rate   Output rate of the specified sensor type.
    * @return
    *    Returns 0 upon success, -1 otherwise
    **************************************************************************/
    int (*GetSensorRate)(sensor_type_e type, int* rate);

    /**
    * This function will be called to retrieve the sensor biases.
    *
    * @param[in] type   Sensor type for which the bias is being queried.
    * @param[out] rate  Output bias of the specified sensor type in the float
    *                  array of length 3.
    * @return
    *    Returns 0 upon success, -1 otherwise
    **************************************************************************/
    int (*GetSensorBias)(sensor_type_e type, float* bias);

} qvr_external_sensor_ops_t;

typedef struct qvr_external_sensors{
    int api_version;
    qvr_external_sensor_ops_t* ops;
} qvr_external_sensors_t;

qvr_external_sensors_t* getInstance(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* QVRSERVICEEXTERNALSENSOR_H */
