#ifndef _CALIBRATION_H
#define _CALIBRATION_H
/// The Hexagon builds, or 'make trackerLib-qidlTargetcalibration', will translate the IDL file to the header file, and place the calibration.h file in the build directory. Copy this file back to the hexagon source tree (here) and the CameraModel source tree.
#include <AEEStdDef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int64_t XrDurationQTI_t;
/// double version of OpenXR XrVector2f
typedef struct XrVector2dQTI_t XrVector2dQTI_t;
struct XrVector2dQTI_t {
   double x;
   double y;
};
/// double version of OpenXR XrVector3f
typedef struct XrVector3dQTI_t XrVector3dQTI_t;
struct XrVector3dQTI_t {
   double x;
   double y;
   double z;
};
/// double version of OpenXR XrQuaternionf
typedef struct XrQuaterniondQTI_t XrQuaterniondQTI_t;
struct XrQuaterniondQTI_t {
   double x;
   double y;
   double z;
   double w;
};
/// double version of OpenXR XrPosed
typedef struct XrPosedQTI_t XrPosedQTI_t;
struct XrPosedQTI_t {
   XrQuaterniondQTI_t orientation;
   XrVector3dQTI_t position;
};
/// OpenXR type
typedef struct XrOffset2DiQTI_t XrOffset2DiQTI_t;
struct XrOffset2DiQTI_t {
   int32_t x;
   int32_t y;
};
/// OpenXR type
typedef struct XrExtent2DiQTI_t XrExtent2DiQTI_t;
struct XrExtent2DiQTI_t {
   int32_t width;
   int32_t height;
};
typedef struct CaptureDetails CaptureDetails;
struct CaptureDetails {
   XrExtent2DiQTI_t nativeSensorSize;
   int32_t cropL;
   int32_t cropR;
   int32_t cropT;
   int32_t cropB;
   int32_t binningH;
   int32_t binningV;
};
enum DistortionModel {
   LINEAR,
   RADIAL_2_PARAMETERS,
   RADIAL_3_PARAMETERS,
   RADIAL_6_PARAMETERS,
   FISHEYE_1_PARAMETER,
   FISHEYE_4_PARAMETERS,
   _32BIT_PLACEHOLDER_DistortionModel = 0x7fffffff
};
typedef enum DistortionModel DistortionModel;
/// describes the intrinsic calibration parameters of a camera
typedef struct IntrinsicCameraCalibrationQTI IntrinsicCameraCalibrationQTI;
struct IntrinsicCameraCalibrationQTI {
   XrExtent2DiQTI_t size;
   XrVector2dQTI_t principal_point;
   XrVector2dQTI_t focal_length;
   double radial_distortion[6];
   double tangential_distortion[2];
   double distortion_limit;
   double undistortion_limit;
   DistortionModel model;
};
/// a full camera calibration structure including
/// intrinsic, extrinsic and other additional parameters.
typedef struct CameraCalibrationQTI CameraCalibrationQTI;
struct CameraCalibrationQTI {
   char description[64];
   IntrinsicCameraCalibrationQTI intrinsics;
   XrPosedQTI_t extrinsic;
   XrDurationQTI_t line_time;
   int32_t timestamp_row;
   CaptureDetails capture_details;
};
/// mapping from hardware camera device to camera calibration parameters
typedef struct CameraDeviceMappingQTI CameraDeviceMappingQTI;
struct CameraDeviceMappingQTI {
   XrOffset2DiQTI_t offset;
};
/// full camera description including HW device information
typedef struct CameraDataQTI CameraDataQTI;
struct CameraDataQTI {
   CameraCalibrationQTI calibration;
   CameraDeviceMappingQTI device;
   XrDurationQTI_t imu_camera_timestamp_offset;
};
/// linear calibration model for different IMU sensors
typedef struct IMULinearCalibrationQTI IMULinearCalibrationQTI;
struct IMULinearCalibrationQTI {
   XrVector3dQTI_t scale;
   XrVector3dQTI_t bias;
   XrVector3dQTI_t nonorthogonality;
};
/// IMU calibration parameters
typedef struct IntrinsicIMUCalibrationQTI IntrinsicIMUCalibrationQTI;
struct IntrinsicIMUCalibrationQTI {
   IMULinearCalibrationQTI accelerometer;
   IMULinearCalibrationQTI gyroscope;
   IMULinearCalibrationQTI magnetometer;
   XrQuaterniondQTI_t ombg;
};
/// a full IMU calibration structure including
/// intrinsic and extrinsic parameters
typedef struct IMUCalibrationQTI IMUCalibrationQTI;
struct IMUCalibrationQTI {
   char description[64];
   IntrinsicIMUCalibrationQTI intrinsics;
   XrPosedQTI_t extrinsic;
};
/// a full device calibration
typedef struct DeviceCalibrationQTI DeviceCalibrationQTI;
struct DeviceCalibrationQTI {
   char description[64];
   char serial_num[64];
   int32_t num_imus;
   IMUCalibrationQTI imus[2];
   int32_t num_cameras;
   CameraDataQTI cameras[6];
};
#ifdef __cplusplus
}
#endif
#endif //_CALIBRATION_H
