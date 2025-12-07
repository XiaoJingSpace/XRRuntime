/**********************************************************
 * FILE: svrCameraUndistort.cpp
 *
 * Copyright (c) 2020 QUALCOMM Technologies Inc.
 **********************************************************/
#ifdef ENABLE_CAMERA
#include <arm_neon.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "svrUtil.h"
#include "svrCameraUndistort.h"

#include "StereoRectifyWrapper.h"

static bool ConvertQVRCalibToVIOCalib(XrCameraDevicePropertiesQTI& QVRCalib,
    DeviceCalibrationQTI& VIOCalib);

svrCameraUndistort::svrCameraUndistort():
    mInitialized(false),
    mLeftDistortedPixelPositionsX(NULL),
    mLeftDistortedPixelPositionsY(NULL),
    mRightDistortedPixelPositionsX(NULL),
    mRightDistortedPixelPositionsY(NULL),
    mLeftRectifiedPixelPositionsX(NULL),
    mLeftRectifiedPixelPositionsY(NULL),
    mRightRectifiedPixelPositionsX(NULL),
    mRightRectifiedPixelPositionsY(NULL),
    mFOVFactor(0.75),
    mDisparityFactor(0.f)
{
}

svrCameraUndistort::~svrCameraUndistort()
{
    Deinit();
}

enum {
    LEFTCAM = 0,
    RIGHTCAM,
    LEFTCAM_COMBINED,
    RIGHTCAM_COMBINED
};

bool svrCameraUndistort::Init(XrCameraDevicePropertiesQTI& camDeviceProperties, float fovFactor)
{
    int stereoWidth  = 0;
    int stereoHeight = 0;

    {
        XrHardwareComponentBaseQTI* comp;
        comp = camDeviceProperties.base.components;

        for (uint32_t i=0; i<camDeviceProperties.base.componentCount; i++)
        {
            XrCameraSensorPropertiesQTI* props = (XrCameraSensorPropertiesQTI*) comp;
            stereoWidth = MAX(stereoWidth, props->frameOffset.x + props->calibrationInfo.intrinsics.size.width);
            stereoHeight = MAX(stereoHeight, props->frameOffset.y + props->calibrationInfo.intrinsics.size.height);
            comp = (XrHardwareComponentBaseQTI*) comp->next;
            if (NULL == comp)
            {
                break;
            }
        }
    }

    if (!ConvertQVRCalibToVIOCalib(camDeviceProperties, mCalData))
        return false;

    LOGI("svrCameraUndistort: stereoWidth=%d, stereoHeight= %d\n", stereoWidth, stereoHeight);
    if (mInitialized)
        return false;
    if (stereoWidth <= 0 || stereoHeight <= 0)
    {
        LOGE("svrCameraUndistort: Init failed - invalid dimensions");
        return false;
    }
    mWidth = stereoWidth;
    mHeight = stereoHeight;
    mFOVFactor = fovFactor;

    // Camera setup
    uint32_t cameraSize = mHeight * (mWidth / 2);
    mMapLeftX.resize(cameraSize);
    mMapLeftY.resize(cameraSize);
    mMapRightX.resize(cameraSize);
    mMapRightY.resize(cameraSize);

    mRectifiedMapLeftX.resize(cameraSize);
    mRectifiedMapLeftY.resize(cameraSize);
    mRectifiedMapRightX.resize(cameraSize);
    mRectifiedMapRightY.resize(cameraSize);

#ifdef __ARM_NEON
    LOGI("svrCameraUndistort: NEON is enabled");
#else
    LOGI("svrCameraUndistort: NEON is disabled");
#endif
    mInitialized = true;

    return true;
}

bool svrCameraUndistort::Deinit()
{
    mInitialized = false;

    return true;
}

bool svrCameraUndistort::CreateUndistortedImage(const uint8_t* distortedStereoImage, uint8_t* undistortedImageOut)
{
    if (!mInitialized)
        return false;
    uint32_t halfWidth = mWidth / 2;
    uint8_t* undistortedImageOutLeft = &undistortedImageOut[halfWidth];

    if (!LoadUndistortMap())
        return false;

    BcvRemap(distortedStereoImage, mHeight, mWidth, undistortedImageOut, mHeight, halfWidth, mLeftDistortedPixelPositionsX, mLeftDistortedPixelPositionsY, LEFTCAM_COMBINED);
    BcvRemap(distortedStereoImage, mHeight, mWidth, undistortedImageOutLeft, mHeight, halfWidth, mRightDistortedPixelPositionsX, mRightDistortedPixelPositionsY, RIGHTCAM_COMBINED);

    return true;
}

bool svrCameraUndistort::GetUndistortMaps(float* mapLeftX, float* mapLeftY, float* mapRightX, float* mapRightY, size_t count)
{
    if (!mInitialized)
        return false;

    if (!LoadUndistortMap())
        return false;

    if (count != mMapLeftX.size())
    {
        LOGE("svrCameraUndistort: size mismatch: %zu != %zu", count, mMapLeftX.size());
        return false;
    }

    memcpy(mapLeftX, mLeftDistortedPixelPositionsX, sizeof(float)*count);
    memcpy(mapLeftY, mLeftDistortedPixelPositionsY, sizeof(float)*count);
    memcpy(mapRightX, mRightDistortedPixelPositionsX, sizeof(float)*count);
    memcpy(mapRightY, mRightDistortedPixelPositionsY, sizeof(float)*count);

    return true;
}

bool svrCameraUndistort::GetRectifyMaps(float* mapLeftX, float* mapLeftY, float* mapRightX, float* mapRightY, size_t count, float* k)
{
    if (!mInitialized)
        return false;

    if (!LoadRectifyMap())
        return false;

    if (count != mRectifiedMapLeftX.size())
    {
        LOGE("svrCameraUndistort: size mismatch: %zu != %zu", count, mMapLeftX.size());
        return false;
    }

    memcpy(mapLeftX, mLeftRectifiedPixelPositionsX, sizeof(float)*count);
    memcpy(mapLeftY, mLeftRectifiedPixelPositionsY, sizeof(float)*count);
    memcpy(mapRightX, mRightRectifiedPixelPositionsX, sizeof(float)*count);
    memcpy(mapRightY, mRightRectifiedPixelPositionsY, sizeof(float)*count);
    *k = mDisparityFactor;

    return true;
}

bool svrCameraUndistort::LoadUndistortMap()
{
    if (NULL == mLeftDistortedPixelPositionsX)
    {
        bool result = false;
        uint32_t cameraSize = mHeight * (mWidth / 2);
        result = CreateUndistortMap(&mCalData, 0,  mFOVFactor,
            mMapLeftX.data(), cameraSize, mMapLeftY.data(), cameraSize,
            mMapRightX.data(), cameraSize, mMapRightY.data(), cameraSize);
        if (!result)
        {
            LOGE("svrCameraUndistort: CreateUndistortMap failed");
            return false;
        }

        mLeftDistortedPixelPositionsX =  &mMapLeftX[0];
        mLeftDistortedPixelPositionsY =  &mMapLeftY[0];
        mRightDistortedPixelPositionsX = &mMapRightX[0];
        mRightDistortedPixelPositionsY = &mMapRightY[0];
    }

    return true;
}

#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))

bool svrCameraUndistort::LoadRectifyMap()
{
    if (NULL == mLeftRectifiedPixelPositionsX)
    {
        bool result = false;
        uint32_t cameraSize = mHeight * (mWidth / 2);
        CameraCalibrationQTI rectifiedCam;
        result = CreateRectifyMap(&mCalData, 0, 
            mRectifiedMapLeftX.data(), cameraSize, mRectifiedMapLeftY.data(), cameraSize,
            mRectifiedMapRightX.data(), cameraSize, mRectifiedMapRightY.data(), cameraSize,
            &mDisparityFactor, &rectifiedCam);
        if (!result)
        {
            LOGE("svrCameraUndistort: CreateRectifyMap failed");
            return false;
        }

        mLeftRectifiedPixelPositionsX =  &mRectifiedMapLeftX[0];
        mLeftRectifiedPixelPositionsY =  &mRectifiedMapLeftY[0];
        mRightRectifiedPixelPositionsX = &mRectifiedMapRightX[0];
        mRightRectifiedPixelPositionsY = &mRectifiedMapRightY[0];
    }

    return true;
}

bool svrCameraUndistort::GetVSTMaps(float* mapLeftX, float* mapLeftY, float* mapRightX, float* mapRightY, size_t count, 
                                    CameraCalibrationQTI* leftDisplay, CameraCalibrationQTI* rightDisplay, float planeDistance)
{
    if (!mInitialized)
        return false;

    //resize to target map size
    mMapLeftX.resize(count);
    mMapLeftY.resize(count);
    mMapRightX.resize(count);
    mMapRightY.resize(count);


    if (!LoadVSTMap(count, leftDisplay, rightDisplay, planeDistance))
        return false;
    
    memcpy(mapLeftX, &mMapLeftX[0], sizeof(float) * count);
    memcpy(mapLeftY, &mMapLeftY[0], sizeof(float) * count);
    memcpy(mapRightX, &mMapRightX[0], sizeof(float) * count);
    memcpy(mapRightY, &mMapRightY[0], sizeof(float) * count);

    return true;
}
bool svrCameraUndistort::LoadVSTMap(uint32_t mapSize, CameraCalibrationQTI* leftDisplay, CameraCalibrationQTI* rightDisplay, float planeDistance)
{
    bool result = false;
    uint32_t includeoutsidepixels=1;
    result = CreateFixedPlaneVSTMaps(&mCalData, leftDisplay, rightDisplay, planeDistance, includeoutsidepixels, mMapLeftX.data(), mapSize, mMapLeftY.data(), mapSize,
                                   mMapRightX.data(), mapSize, mMapRightY.data(), mapSize);
    if (!result)
    {
        LOGE("svrCameraUndistort: CreateFixedPlaneVSTMap failed");
        return false;
    }
    return true;
}

void svrCameraUndistort::BcvRemap(const uint8_t* srcY, int rowsSrcY, int colsSrcY, uint8_t* dstY, int rowsDstY, int colsDstY, const float* xMap, const float* yMap, const int cam)
{
    OperationPipeline pipe;
    pipe.addOperation<RowOpRemapY>(srcY, rowsSrcY, colsSrcY, dstY, rowsDstY, colsDstY, xMap, yMap, cam);
    pipe.run();
}

RowOpRemapY::RowOpRemapY(const uint8_t* srcY, int rowsSrcY, int colsSrcY, uint8_t* dstY, int rowsDstY, int colsDstY, const float* xMap, const float* yMap, const int cam) : RowOperationBase(rowsDstY, 8, 0),
    _srcY(srcY),
    _rowsSrcY(rowsSrcY),
    _colsSrcY(colsSrcY),
    _dstY(dstY),
    _rowsDstY(rowsDstY),
    _colsDstY(colsDstY),
    _xMap(xMap),
    _yMap(yMap),
    _cam(cam)
{
    _srcCorner.resize(0);

    for (int i = 0; i < 4; i++)
    {
        _srcCorner.push_back(new uint8_t[8 * _colsDstY]);
    }

    _outputRowBuf = new uint8_t[8 * _colsDstY];

    switch(_cam) {
    case LEFTCAM:
        _srcimgoffset = 0;
    break;

    case RIGHTCAM:
        _srcimgoffset = _colsDstY;
    break;

    case LEFTCAM_COMBINED:
        _srcimgoffset = 0;
        _outputimgoffset = _colsDstY;
    break;

    case RIGHTCAM_COMBINED:
        _srcimgoffset = _colsDstY;
        _outputimgoffset = _colsDstY;
    break;

    default:
    break;
    }
}

RowOpRemapY::~RowOpRemapY()
{
    for (uint8_t* srcC : _srcCorner)
    {
        delete [] srcC;
    }
    delete [] _outputRowBuf;
}

void RowOpRemapY::perform(unsigned int y)
{
    if (!_srcY || !_dstY || !_xMap || !_yMap || _rowsSrcY <= 0 || _colsSrcY <= 0 || _rowsDstY <= 0 || _colsDstY <= 0)
    {
        LOGE("svrCameraUndistort: ERROR: Bad inputs for RowOpRemapY!");
        return;
    }
    const float* xMapPtr = &(_xMap[y * _colsDstY]);
    const float* yMapPtr = &(_yMap[y * _colsDstY]);

    int tid = y & 7;
    uint8_t* srcCorner0 = &(_srcCorner[0][tid * _colsDstY]);
    uint8_t* srcCorner1 = &(_srcCorner[1][tid * _colsDstY]);
    uint8_t* srcCorner2 = &(_srcCorner[2][tid * _colsDstY]);
    uint8_t* srcCorner3 = &(_srcCorner[3][tid * _colsDstY]);

    for (int x = 0; x < _colsDstY; x++)
    {
        int xMapVal = (int)xMapPtr[x];
        int yMapVal = (int)yMapPtr[x];

        if (xMapVal < 0 || yMapVal < 0 || xMapVal > _colsSrcY - 2 || yMapVal > _rowsSrcY - 2)
        {
            srcCorner0[x] = srcCorner1[x] = srcCorner2[x] = srcCorner3[x] = 0;
        }
        else
        {
            srcCorner0[x] = _srcY[(yMapVal * _colsSrcY) + xMapVal + _srcimgoffset];
            srcCorner1[x] = _srcY[(yMapVal * _colsSrcY) + xMapVal + 1 + _srcimgoffset];
            srcCorner2[x] = _srcY[((yMapVal + 1) * _colsSrcY) + xMapVal + 1 + _srcimgoffset];
            srcCorner3[x] = _srcY[((yMapVal + 1) * _colsSrcY) + xMapVal + _srcimgoffset];
        }
    }

    uint8_t* outputBuf = &(_outputRowBuf[tid * _colsDstY]);
    RemapPerRow(srcCorner0, srcCorner1, srcCorner2, srcCorner3, _colsDstY, xMapPtr, yMapPtr, outputBuf);

    uint8_t* dstRow = &(_dstY[y * (_colsDstY + _outputimgoffset)]);
    for (int x = 0; x < _colsDstY; x++)
    {
        dstRow[x] = outputBuf[x];
    }
}

static inline
void RemapPerRow(const uint8_t* src1Ptr, const uint8_t* src2Ptr, const uint8_t* src3Ptr, const uint8_t* src4Ptr,
                 int width, const float* xMapPtr, const float* yMapPtr, uint8_t* dstPtr)
{
   int x = 0;

#ifdef __ARM_NEON
   int16x8_t CONST_MAX_S16 = vdupq_n_s16(32767);

   for (; x + 8 <= width; x += 8)
   {
        uint32x4_t xMap0Q16 = vcvtq_n_u32_f32(vld1q_f32(&xMapPtr[x]), 16);
        uint32x4_t xMap1Q16 = vcvtq_n_u32_f32(vld1q_f32(&xMapPtr[x + 4]), 16);

        uint16x8_t xMap0U8 = vreinterpretq_u16_u32(xMap0Q16);
        uint16x8_t xMap1U8 = vreinterpretq_u16_u32(xMap1Q16);
        int16x8_t fx = vreinterpretq_s16_u16(vshrq_n_u16(vuzpq_u16(xMap0U8, xMap1U8).val[0], 1));
        int16x8_t fx1 = vsubq_s16(CONST_MAX_S16, fx);

        uint32x4_t yMap0Q16 = vcvtq_n_u32_f32(vld1q_f32(&yMapPtr[x]), 16);
        uint32x4_t yMap1Q16 = vcvtq_n_u32_f32(vld1q_f32(&yMapPtr[x + 4]), 16);

        uint16x8_t yMap0U8 = vreinterpretq_u16_u32(yMap0Q16);
        uint16x8_t yMap1U8 = vreinterpretq_u16_u32(yMap1Q16);
        int16x8_t fy = vreinterpretq_s16_u16(vshrq_n_u16(vuzpq_u16(yMap0U8, yMap1U8).val[0], 1));
        int16x8_t fy1 = vsubq_s16(CONST_MAX_S16, fy);

        int16x8_t w1 = vqrdmulhq_s16(fx1, fy1);
        int16x8_t w2 = vqrdmulhq_s16(fx, fy1);
        int16x8_t w3 = vqrdmulhq_s16(fx, fy);
        int16x8_t w4 = vqrdmulhq_s16(fx1, fy);

        int16x8_t p1 = vreinterpretq_s16_u16(vshll_n_u8(vld1_u8(&src1Ptr[x]), 7));
        int16x8_t p2 = vreinterpretq_s16_u16(vshll_n_u8(vld1_u8(&src2Ptr[x]), 7));
        int16x8_t p3 = vreinterpretq_s16_u16(vshll_n_u8(vld1_u8(&src3Ptr[x]), 7));
        int16x8_t p4 = vreinterpretq_s16_u16(vshll_n_u8(vld1_u8(&src4Ptr[x]), 7));

        int16x8_t prod = vqrdmulhq_s16(w1, p1);
        prod = vqaddq_s16(prod, vqrdmulhq_s16(w2, p2));
        prod = vqaddq_s16(prod, vqrdmulhq_s16(w3, p3));
        prod = vqaddq_s16(prod, vqrdmulhq_s16(w4, p4));

        vst1_u8(&dstPtr[x], vqrshrun_n_s16(prod, 7));
   }
#else
   for (; x < width; x++)
   {
        uint32_t tmp = floor(xMapPtr[x] * 65536);
        tmp = tmp & 0xFFFF;
        int16_t fx = tmp >> 1;

        tmp = floor(yMapPtr[x] * 65536);
        tmp = tmp & 0xFFFF;
        int16_t fy = tmp >> 1;

        int16_t fx1 = 32767 - fx;
        int16_t fy1 = 32767 - fy;

        int16_t w1 = saturate_cast<int16_t>((((int32_t)fx1*fy1) + (1 << 14)) >> 15);
        int16_t w2 = saturate_cast<int16_t>((((int32_t)fx*fy1) + (1 << 14)) >> 15);
        int16_t w3 = saturate_cast<int16_t>((((int32_t)fx*fy) + (1 << 14)) >> 15);
        int16_t w4 = saturate_cast<int16_t>((((int32_t)fx1*fy) + (1 << 14)) >> 15);

        int16_t p1 = src1Ptr[x] << 7;
        int16_t p2 = src2Ptr[x] << 7;
        int16_t p3 = src3Ptr[x] << 7;
        int16_t p4 = src4Ptr[x] << 7;

        int16_t w1p1 = saturate_cast<int16_t>((((int32_t)w1 * p1) + (1 << 14)) >> 15);
        int16_t w2p2 = saturate_cast<int16_t>((((int32_t)w2 * p2) + (1 << 14)) >> 15);
        int16_t w3p3 = saturate_cast<int16_t>((((int32_t)w3 * p3) + (1 << 14)) >> 15);
        int16_t w4p4 = saturate_cast<int16_t>((((int32_t)w4 * p4) + (1 << 14)) >> 15);

        dstPtr[x] = saturate_cast<uint8_t>(((int32_t)w1p1 + w2p2 + w3p3 + w4p4 + (1 << 6)) >> 7);
   }
#endif
}

// converts from distortion models defined in calibration.h to those defined in QXR.h
static inline DistortionModel ConvertDistortionModel(XrDistortionModelQTI model)
{

    LOGE("svrCameraUndistort: XRDistortionModelQTI : %d\n", model);
    switch (model) {
    case XR_DISTORTION_MODEL_QTI_LINEAR: return FISHEYE_4_PARAMETERS;
    case XR_DISTORTION_MODEL_QTI_RADIAL_2_PARAMS: return RADIAL_2_PARAMETERS;
    case XR_DISTORTION_MODEL_QTI_RADIAL_3_PARAMS: return RADIAL_3_PARAMETERS;
    case XR_DISTORTION_MODEL_QTI_RADIAL_6_PARAMS: return RADIAL_6_PARAMETERS;
    case XR_DISTORTION_MODEL_QTI_FISHEYE_1_PARAM: return FISHEYE_1_PARAMETER;
    case XR_DISTORTION_MODEL_QTI_FISHEYE_4_PARAMS: return FISHEYE_4_PARAMETERS;
    default: return _32BIT_PLACEHOLDER_DistortionModel;
    }
}

static bool ConvertQVRCalibToVIOCalib(XrCameraDevicePropertiesQTI& QVRCalib,
    DeviceCalibrationQTI& VIOCalib)
{
    if (QVRCalib.base.type != XR_TYPE_QTI_CAM_DEVICE_PROPS)
    {
        LOGE("svrCameraUndistort: Error: camera device props wrong struct type: %d\n", QVRCalib.base.type);
        return false;
    }
    if (QVRCalib.base.deviceType != XR_HW_DEVICE_TYPE_QTI_CAMERA)
    {
        LOGE("svrCameraUndistort: Error: camera device props wrong device type: %d\n", QVRCalib.base.deviceType);
        return false;
    }

    // we only support up to 2 cameras currently
    if (QVRCalib.base.componentCount > 2)
    {
        LOGE("svrCameraUndistort: Error: camera device props contains more than 2 cameras (%d)", QVRCalib.base.componentCount);
        return false;
    }

    XrHardwareComponentBaseQTI* comp = QVRCalib.base.components;

    for (uint32_t i=0; i<QVRCalib.base.componentCount; i++)
    {
        XrCameraSensorPropertiesQTI* props = (XrCameraSensorPropertiesQTI*) comp;

        if (comp->type != XR_TYPE_QTI_CAM_SENSOR_PROPS)
        {
            LOGE("svrCameraUndistort: Error: Camera component is not a sensor structure type: %d\n", comp->type);
            return false;
        }

        if (comp->componentType != XR_HW_COMP_TYPE_QTI_CAMERA_SENSOR)
        {
            LOGE("svrCameraUndistort: Error: Camera component type is not of type sensor: %d\n", comp->componentType);
            return false;
        }
        // must copy intrinsics one member at a time since the radial distortion
        // arrays are different sizes.
        memcpy(&VIOCalib.cameras[i].calibration.intrinsics.size,
            &props->calibrationInfo.intrinsics.size,
            sizeof(VIOCalib.cameras[i].calibration.intrinsics.size));
        memcpy(&VIOCalib.cameras[i].calibration.intrinsics.principal_point,
            &props->calibrationInfo.intrinsics.principalPoint,
            sizeof(VIOCalib.cameras[i].calibration.intrinsics.principal_point));
        memcpy(&VIOCalib.cameras[i].calibration.intrinsics.focal_length,
            &props->calibrationInfo.intrinsics.focalLength,
            sizeof(VIOCalib.cameras[i].calibration.intrinsics.focal_length));
        memcpy(&VIOCalib.cameras[i].calibration.intrinsics.radial_distortion,
            &props->calibrationInfo.intrinsics.radialDistortion,
            sizeof(VIOCalib.cameras[i].calibration.intrinsics.radial_distortion));
        memcpy(&VIOCalib.cameras[i].calibration.intrinsics.tangential_distortion,
            &props->calibrationInfo.intrinsics.tangentialDistortion,
            sizeof(VIOCalib.cameras[i].calibration.intrinsics.tangential_distortion));
        VIOCalib.cameras[i].calibration.intrinsics.model =
            ConvertDistortionModel(props->calibrationInfo.intrinsics.distortionModel);

        memcpy(&VIOCalib.cameras[i].calibration.extrinsic,
            &props->base.extrinsic,
            sizeof(VIOCalib.cameras[i].calibration.extrinsic));

        VIOCalib.cameras[i].calibration.line_time = props->calibrationInfo.lineTime;
        VIOCalib.cameras[i].calibration.timestamp_row = props->calibrationInfo.timestampRow;

        memcpy(&VIOCalib.cameras[i].device.offset,
            &props->frameOffset,
            sizeof(VIOCalib.cameras[i].device.offset));

        comp = (XrHardwareComponentBaseQTI*) comp->next;
        if (NULL == comp)
        {
            break;
        }
    }

    VIOCalib.num_cameras = QVRCalib.base.componentCount;

    return true;
}

#endif
