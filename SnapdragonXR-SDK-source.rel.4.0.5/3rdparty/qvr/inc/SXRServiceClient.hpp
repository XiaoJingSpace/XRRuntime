/******************************************************************************
* File: SXRServiceClient.hpp
*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#pragma once

#include <stdint.h>
#include <vector>

typedef enum {
    SXR_SERVICE_NONE        = 0,
    SXR_SERVICE_RVR_SERVER  = 1,
    SXR_SERVICE_VDEC        = 2,
    SXR_SERVICE_USB         = 3,
    SXR_SERVICE_RVR_CLIENT  = 4,
    SXR_SERVICE_VENC        = 5,
    SXR_SERVICE_MAX,
}SXRServiceType_t;

enum class SXRATWMode
{
    DOF3 = 0,
    DOF4HarmonicMean = 2,
    APAPR = 3,
};

class ISXRServiceClient
{
public:
    ISXRServiceClient() = default;
    virtual ~ISXRServiceClient() = default;

    virtual SXRServiceType_t GetType() = 0;
    virtual int32_t GetInterface(void **ppService) = 0;

    virtual void Release() = 0;
};

typedef int32_t (*FnSXRCreateService)(SXRServiceType_t , void *, ISXRServiceClient **);
extern "C" int32_t SXRCreateService(SXRServiceType_t type, void *info, ISXRServiceClient **ppService);

//***************************Video Decoder************************************
struct SXRDimensions
{
    int32_t width;
    int32_t height;
};

struct SXRBufferInfo
{
    size_t   index;
    uint8_t* data;
    size_t   size;
    int64_t  timestampUs;
    uint32_t flags;
};

typedef void (*SXRFnCallback)(void *, int32_t);

struct SXRCodecInfo
{
    int32_t       id;
    int32_t       textureID;
    SXRDimensions dimensions;
    int32_t       ipDequeueTimeoutMs;
    int32_t       opDequeueTimeoutMs;
    void*         pObject;
    SXRFnCallback fnCallback;
    int32_t       ipBufferCount;
    int32_t       opBufferCount;
    int32_t       codec;
    uint32_t      fps;
    int32_t       perf_mask;
    float         perf_factor0;
    float         perf_factor1;
};

class ISXRVideoDecoderServiceClient
{
public:
    ISXRVideoDecoderServiceClient() = default;
    virtual ~ISXRVideoDecoderServiceClient() = default;

    virtual int32_t DequeueInputBuffer(SXRBufferInfo& bufferInfo) = 0;
    virtual int32_t EnqueueInputBuffer(const SXRBufferInfo& bufferInfo) = 0;
    virtual int32_t DequeueOutputBuffer(SXRBufferInfo& bufferInfo) = 0;

    virtual uint32_t UpdateTexImage() = 0;
    virtual int64_t GetTimestampUs() = 0;

    virtual SXRDimensions GetDimensions() = 0;
};

//*****************************************************************************

//*******************************USB Service***********************************

struct SXRUSBMapData_t
{
    void*    data;
    uint32_t size;
};

class ISXRUSBServiceClient
{
public:
    ISXRUSBServiceClient() = default;
    virtual ~ISXRUSBServiceClient() = default;

    virtual int32_t Open(const char* node, int32_t bufferSize, int32_t numBuffers) = 0;
    virtual int32_t Lock(SXRUSBMapData_t &map, int32_t timeoutMs) = 0;
    virtual int32_t Unlock() = 0;
    virtual int32_t Write(void* data, uint32_t size, int32_t timeoutMs) = 0;
};

//*****************************************************************************

//*******************************RVR Service***********************************

struct SXRRVRConfig
{
    uint32_t width;
    uint32_t height;
    uint32_t avg_bitrate;
    uint32_t peak_bitrate;
    int32_t  gop_length;
    uint32_t slices_per_frame;
    uint32_t mtu;
    float    fps;
    char     codec[8];
    uint8_t  use_pose_socket;
    uint8_t  remote_mode;
    uint8_t  perf_mask;
    uint8_t  ycocg;
    uint8_t  display_flags;
    uint8_t  sync_with_remote;
    float    perf_factor0;
    float    perf_factor1;
    float    cam_near;
    float    cam_far;
    uint8_t  audio_enabled;
    uint8_t  warpmesh_type;
    float ipd;
    float fov;
    bool calculateDepthDFS;
    int32_t downScaleFactor;
    int32_t localSizeX;
    int32_t localSizeY;
    int32_t kernelSize;
    int32_t maxDisparity;
    SXRATWMode defaultAtwMode;
    float positionScaleZ;
    float farClipPlane;
    bool  enableHeartbeat;
    bool  waitForHMD;
    bool  payloadInRTP;
    uint32_t streamingWidth;
    uint32_t streamingHeight;
    float    foveaAngle;
    uint8_t  flags;
};

struct SXRGLInfo
{
    int32_t w;
    int32_t h;
    int32_t colorSpace;
    void*   nativeWindow;
};

enum SXREyeMask
{
    kSXREyeMaskRight = 0x00000002,
    kSXREyeMaskLeft = 0x00000001,
    kSXREyeMaskBoth = 0x00000003
};

struct SXRVector3
{
    float x,y,z;
};

struct SXRQuaternion
{
    float x, y, z, w;
};

struct SXRHeadPose
{
    SXRQuaternion rotation;
    SXRVector3    position;
};

struct SXRHeadPoseState
{
   SXRHeadPose pose;                //!< Head pose
   uint64_t    poseTimeStampNs;     //!< Time stamp in which the head pose was generated (nanoseconds)
};

struct SXRRenderLayer
{
    int32_t    imageHandle;        //!< Handle to the texture/image to be rendered
    SXREyeMask eyeMask;            //!< Determines which eye[s] receive this render layer
    uint32_t   layerFlags;         //!< Flags applied to this render layer
};

struct SXRFrameParams
{
    int32_t          frameIndex;         //!< Frame Index
    SXRHeadPoseState headPoseState;      //!< Head pose state used to generate the frame
    SXRRenderLayer   renderLayers[2];    //!< Description of each render layer
};

class ISXRRVRServiceClient
{
public:
    ISXRRVRServiceClient() = default;
    virtual ~ISXRRVRServiceClient() = default;
    virtual int32_t Init(SXRRVRConfig &param) = 0;
    virtual int32_t Deinit() = 0;
    virtual int32_t SetNativeWindow(SXRGLInfo *info) = 0;
    virtual int32_t Submit(const SXRFrameParams& pFrameParams) = 0;
    virtual int32_t GetHeadPose(SXRHeadPoseState& headPose) = 0;
};
//*****************************************************************************

//*******************************RVRClient Service*****************************

struct SXRAudioInfo
{
    int32_t   index;
    uint8_t* data;
    size_t   size;
    int64_t  timestampUs;
    uint32_t seqnum;
};

struct SXRSampleInfo
{
    int32_t size;
    int64_t timestampUs;
    bool isCodecSpecificData;
    bool isSEIData;
    bool eosReached;
    bool isIncompleteFrame;
    bool isKeyFrame;
    float predictedTimeMs;
    int32_t frameNumber;
    SXRVector3    position;
    SXRQuaternion rotation;
    float depth;
    SXRVector3 planeParams;
    std::vector<std::pair<int32_t, int32_t>> mbList;
    std::vector<std::pair<int32_t, int32_t>> intraRefreshMBList;
};

struct SXRVideoInfo 
{
    uint32_t texId;
    SXRSampleInfo sInfo;
};

class ISXRRVRClient
{
public:
    ISXRRVRClient() = default;
    virtual ~ISXRRVRClient() = default;
    virtual int32_t Init(SXRCodecInfo &param) = 0;
    virtual int32_t Deinit() = 0;
    virtual int32_t SendHeadPose(void* data, uint32_t size) = 0;
    virtual void    GetVideoFrame(int id, SXRVideoInfo& info) = 0;
    virtual int32_t GetAudioFrame(SXRAudioInfo& info) = 0;
    virtual int32_t ReleaseAudioFrame(SXRAudioInfo& info) = 0;
};
//*****************************************************************************

