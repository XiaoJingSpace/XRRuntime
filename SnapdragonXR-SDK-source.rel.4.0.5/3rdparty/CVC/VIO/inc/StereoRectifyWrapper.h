/*==============================================================================================================================
*  Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
*  All Rights Reserved.
*  Confidential and Proprietary - Qualcomm Technologies, Inc.
==============================================================================================================================*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Common header file between CVC and target build environment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __STEREO_RECTIFY_WRAPPER_H__
#define __STEREO_RECTIFY_WRAPPER_H__
#include <stdint.h>
#include <CameraModel/calibration.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateRectifyMap
//
// Description
//   This function creates a stereo image undistortion and rectification map
//   based on the calibration file provided by pathCalibration.
//
// Parameters
//   deviceCalibration     [in]  The device's camera calibration for the two tracking cameras provided by
//								 tracker_6dof_get_device_calibration.  Only two cameras are supported.
//   includeOutsidePixels  [in]  If non-zero, the maps will also include pixels outside of the rectified frame bounds
//   invalidValue          [in]  The value which will indicate invalid pixels in the output maps.
//   mapLeftX, mapLeftY,
//   mapRightX, mapRightY: [out] arrays of pixel maps for stereo cameras
//   k:                    [out] Disparity-to-depth factor
//   rectifiedCam:         [out] The left virtual camera model (size 1)
//
// Return
//   True on success, false otherwise
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CreateRectifyMap(const DeviceCalibrationQTI* deviceCalibration, uint32_t includeOutsidePixels, float* mapLeftX, uint32_t mapLeftX_size, float* mapLeftY, uint32_t mapLeftY_size, float* mapRightX, uint32_t mapRightX_size, float* mapRightY, uint32_t mapRightY_size, float* k, CameraCalibrationQTI* rectifiedCam);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateUndistortMap
//
// Description
//   This function creates a stereo image undistortion map based on the
//   calibration file provided by pathCalibration.
//
// Parameters
//   calibrationBody:      [in]  The contents of the camera calibration XML document
//   includeOutsidePixels  [in]  If non-zero, the maps will also include pixels outside of the undistorted frame bounds
//   invalidValue          [in]  The value which will indicate invalid pixels in the output maps.
//   fovFactor:            [in]  Adjustment for the field of view of the
//                               undistorted image.  fovFactor should be > 0,
//                               <= 1.0.  Lower values will preserve more
//                               image data.
//   mapLeftX, mapLeftY,
//   mapRightX, mapRightY: [out] arrays of pixel maps for stereo cameras
//
// Return
//   True on success, false otherwise
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CreateUndistortMap(const DeviceCalibrationQTI* deviceCalibration, uint32_t includeOutsidePixels, float fovFactor, float* mapLeftX, uint32_t mapLeftX_size, float* mapLeftY, uint32_t mapLeftY_size, float* mapRightX, uint32_t mapRightX_size, float* mapRightY, uint32_t mapRightY_size);

bool CreateFixedPlaneVSTMaps(const DeviceCalibrationQTI* deviceCalibration, const CameraCalibrationQTI* leftDisplay, const CameraCalibrationQTI* rightDisplay,  float planeDistance, uint32_t includeOutsidePixels, 
                  float* mapLeftX, uint32_t mapLeftX_size, float* mapLeftY, uint32_t mapLeftY_size, float* mapRightX, uint32_t mapRightX_size, float* mapRightY, 
                  uint32_t mapRightY_size);
#endif
