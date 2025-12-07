/**********************************************************
 * FILE: svrCameraUndistort.h
 *
 * Copyright (c) 2020 QUALCOMM Technologies Inc.
 **********************************************************/
#ifndef __SVR_CAMERA_UNDISTORT_H__
#define __SVR_CAMERA_UNDISTORT_H__

#ifdef ENABLE_CAMERA

#include <vector>
#include "QXR.h"
#include "CameraModel/calibration.h"

#include "RowPipeline.h"
class svrCameraUndistort {

public:
    svrCameraUndistort();
    virtual ~svrCameraUndistort();

    bool Init(XrCameraDevicePropertiesQTI& camDeviceProperties, float fovFactor);
    bool Deinit();

    bool CreateUndistortedImage(const uint8_t* imageData, uint8_t* undistortedImageOut);

    bool GetUndistortMaps(float* mapLeftX, float* mapLeftY, float* mapRightX, float* mapRightY, size_t count);
    bool GetRectifyMaps(float* mapLeftX, float* mapLeftY, float* mapRightX, float* mapRightY, size_t count, float* k);
    bool GetVSTMaps(float* mapLeftX, float* mapLeftY, float* mapRightX, float* mapRightY, size_t count, CameraCalibrationQTI* leftDisplay, CameraCalibrationQTI* rightDisplay, float planeDistance);

protected:
    void BcvRemap(const uint8_t* srcY, int rowsSrcY, int colsSrcY, uint8_t* dstY, int rowsDstY, int colsDstY, const float* xMap, const float* yMap, const int cam);

private:
    bool LoadUndistortMap();
    bool LoadRectifyMap();
    bool LoadVSTMap(uint32_t mapsize, CameraCalibrationQTI* leftDisplay, CameraCalibrationQTI* rightDisplay, float planeDistance);
protected:
    bool mInitialized;
    uint32_t mWidth = 0, mHeight = 0;
    std::vector<float> mMapLeftX, mMapLeftY, mMapRightX, mMapRightY;
    float *mLeftDistortedPixelPositionsX, *mLeftDistortedPixelPositionsY;
    float *mRightDistortedPixelPositionsX, *mRightDistortedPixelPositionsY;

    std::vector<float> mRectifiedMapLeftX, mRectifiedMapLeftY, mRectifiedMapRightX, mRectifiedMapRightY;
    float *mLeftRectifiedPixelPositionsX, *mLeftRectifiedPixelPositionsY;
    float *mRightRectifiedPixelPositionsX, *mRightRectifiedPixelPositionsY;

    float mFOVFactor;
    float mDisparityFactor;

    DeviceCalibrationQTI mCalData;
};

class RowOpRemapY : public RowOperationBase {
public:
    RowOpRemapY(const uint8_t* srcY, int rowsSrcY, int colsSrcY, uint8_t* dstY, int rowsDstY, int colsDstY, const float* xMap, const float* yMap, const int cam);
    ~RowOpRemapY();

protected:
    void perform(unsigned int y);

private:
    const uint8_t *_srcY;
    int _rowsSrcY = 0, _colsSrcY = 0;  // These dims define _srcY
    uint8_t *_dstY;
    int _rowsDstY = 0, _colsDstY = 0;  // These dims define _dstY, _xMap and _yMap
    const float *_xMap, *_yMap;
    int _cam = 0;
    int _srcimgoffset = 0;
    int _outputimgoffset = 0;
    std::vector<uint8_t*> _srcCorner;
    uint8_t* _outputRowBuf;
};

static inline
void RemapPerRow(const uint8_t* src1Ptr, const uint8_t* src2Ptr, const uint8_t* src3Ptr, const uint8_t* src4Ptr, int width, const float* xMapPtr, const float* yMapPtr, uint8_t* dstPtr);

#endif // ENABLE_CAMERA
#endif //__SVR_CAMERA_UNDISTORT_H__
