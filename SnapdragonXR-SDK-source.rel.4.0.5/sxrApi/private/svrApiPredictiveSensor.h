//=============================================================================
// FILE: svrApiPredictiveSensor.h
//                  Copyright (c) 2016 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================
#ifndef _SVR_API_PREDICTIVE_SENSOR_H_
#define _SVR_API_PREDICTIVE_SENSOR_H_

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "private/svrApiCore.h"

SxrResult GetTrackingFromPredictiveSensor(float fw_prediction_delay, uint64_t *pSampleTimeStamp, sxrHeadPoseState& poseState);
SxrResult GetTrackingFromHistoricSensor(uint64_t timestampNs, sxrHeadPoseState& poseState);

#endif //_SVR_API_PREDICTIVE_SENSOR_H_
