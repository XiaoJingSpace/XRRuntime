/**********************************************************
 * FILE: svrCameraManager.h
 *
 * Copyright (c) 2020-2021 QUALCOMM Technologies Inc.
 **********************************************************/
#ifndef __SVR_CAMERA_MANAGER_H__
#define __SVR_CAMERA_MANAGER_H__

#ifdef ENABLE_CAMERA

#define NUM_CAMERA_FRAMES 16
#define MAX_IMAGES_PER_FRAME 2

#include <string>
#include <map>
#include <vector>
#include "svrRenderExt.h"
#include "svrGeometry.h"
#include "svrApiTimeWarp.h"
#include "glm/glm.hpp"
#include "CameraModel/calibration.h"
using namespace Svr;

/**********************************************************
 * SvrCameraInfo
 **********************************************************/
class SvrCameraInfo
{
public:
    int m_id;
    float m_width, m_height;
    float m_centerX, m_centerY;
    float m_focalX, m_focalY;
    float m_radialDisortion[8];
    glm::vec3 m_position;
    glm::vec3 m_rotation;
};


/**********************************************************
 * SvrCameraManager
 **********************************************************/
class SvrCameraManager
{
public:
    SvrCameraManager();
    ~SvrCameraManager();

    SxrResult Init();
    SxrResult Update();
    void Shutdown();
    void Pause();
    bool IsPaused();
    void Resume();
    bool IsPFDSupported();
    /*
     * Used in 2pass
     */
    SxrResult Render();
    int32_t GetCorrectedTextureID(sxrEyeMask eyeMask);
    int32_t GetUncorrectedTextureID(sxrEyeMask eyeMask);

    /*
     * Used in 1 pass
     */
	SxrResult MakeOnePassMesh();
    void EndOnePass();
    void GetCameraWarpCoords(float* pWarpIn, float* pWarpOut, int width, int height, int whichEye);
    sxrHeadPoseState GetPoseState();
    bool IsSinglePass();
    void ReleaseFrame(int64_t t1, int64_t t2);
    bool IsStereoPair();
    bool IsOK();
    bool IsFloat16();
    QVRCAMERA_FRAME_FORMAT GetCameraFrameFormat();
	bool IsMonoFloat16();

private:
    SxrResult InitCameraMesh();
    int AddFrame(qvrcamera_device_helper_t * pCameraDevice,
                 qvrcamera_frame_t camera,
                 unsigned int * pTextureID);

    void RenderCamera(sxrWhichEye whichEye);

	SxrResult RenderCameraUVs();
	SxrResult RenderDisplayMesh(SvrGeometry &displayMesh, sxrWhichEye whichEye);

    void TextureLookup(float* pTexture,
                       int pixelSize,
                       int width,
                       int height,
                       float uCoord,
                       float vCoord,
                       int component,
                       float *pValue);

    void GenerateCameraMeshUsingQXR(SvrCameraInfo * pCameraInfo,
                                    int cameraWidth,
                                    int cameraHeight,
                                    float * pMapU,
                                    float * pMapV,
                                    int numCameraTextures,
                                    float offsetU,
                                    SvrGeometry** ppOutGeometry);

    void GenerateDefaultMesh(float offsetU,
                             SvrCameraInfo * pCameraInfo,
                             SvrGeometry** ppOutGeometry);

    void MakeDefaultCameraInfo(qvrcamera_device_helper_t* pCamDevice, SvrCameraInfo *pCamera);
    int GetNumTexturesPerFrame();
    void GetProperties();
    void DumpRawMap(int imageWidth,
                    int imageHeight,
                    float *mapLeftX,
                    float *mapLeftY,
                    float *mapRightX,
                    float *mapRightY);
    void GetProperty(const char *name, char *value, size_t len, const char *default_value);

    void SubmitFrame();

    int CreateCameraDevice();
    int GetCameraCalibInfo(qvrcamera_device_helper_t* pCamDevice, SvrCameraInfo *pCamera);
    void InitializeShaderFromBuffer(Svr::SvrShader &whichShader,
                                    char *pVertexSource,
                                    char *pFragmentSource,
                                    const char *vertexName,
                                    const char *fragmentName);
    void ConvertQVRto6DOF(XrPosedQTI *pPoseIn, XrPosedQTI *pPoseOut);
    void Convert6DOFtoQVR(XrPosedQTI *pPoseIn, XrPosedQTI *pPoseOut);
    void GetSensorOrientationFromDevice(std::string &s);
    void SetSensorOrientation();

    void getEyeViewMatrices( sxrHeadPoseState& poseState, bool bUseHeadModel,
								glm::vec3 eyeTranslationLeft, glm::vec3 eyeTranslationRight, glm::fquat eyeRotationLeft, glm::fquat eyeRotationRight, float headHeight, float headDepth,
								glm::mat4& outLeftEyeMatrix, glm::mat4& outRightEyeMatrix);
    std::vector<std::string> StrSplit(const std::string& s, char sep);
    std::map<std::string,std::string> GetKeyValuePairs(const std::string& s, char sep);
    int SetDisplayProperties(sxrDeviceInfo* di);
    void unpackCameraUVs(int whichEye, int imageWidth, int imageHeight);
    qvrcamera_client_helper_t *m_cameraClient;
    qvrcamera_device_helper_t *m_cameraDevice;
   /*
    * Options to turn things on/off
    */
    bool useSyncFramework;
    bool doUndistort; // correct fisheye distortion
    bool doCameraTimestamp;
    bool useHardwareBuffers; // using shared HW buffer from QVR Service to avoid GL texture copy during two-pass.
    bool useRawFormat;       // using a raw format
    bool doTwoPassRender;  // see comment below
    bool do10bit;
    bool do16bit;
    bool doShowStatus;
    bool doColorCamera;
    bool doRectify;
    bool useVSTMaps; //use cal. maps generated based on XR device properties 
    bool doPartialFrame;
    float vfov;
    float shiftX, shiftY, shiftZ;
    bool doR16float; // 16 bit float, single channel camera texture format
    std::string resSel; // resolution selection

	// temporary color camera calibration overrides.
	float leftColorTilt, leftColorPan, leftColorRoll;
	float rightColorTilt, rightColorPan, rightColorRoll;

    //fill percentage to request for partial frame
    //these should be int but getproperties only does float ....
    float partialFrameFillPercentage;//property
    int m_minPartialFrameFillPercentage;//determine min/max from some other config prop. setting for now
    int m_maxPartialFrameFillPercentage;
    int m_defaultPartialFrameFillPercentage;
    bool m_pfdSupport=false;

    GLuint m_cameraTexture[MAX_IMAGES_PER_FRAME];
    int m_numCameraTextures; // 1 if array holds a stereo image. 2 if array is 2 single images.

    SvrGeometry * m_cameraModel[kNumEyes];

    SvrCameraInfo m_cameraInfo[2];
    SvrCameraInfo m_displayInfo[2];
    float *m_cameraUV[2];
    qvrsync_ctrl_t *m_syncCtrl;
    char* m_cameraName;

    int m_frameNumber;
    int m_frameIndex;
    bool m_gotFPS;
    bool m_10BitFailed;
    bool m_okToRender;
    bool m_showStatusLog;
    bool m_leftRenderSaved, m_rightRenderSaved;
    unsigned char * m_pCameraBuffer;
    SvrShader m_cameraShader;

    sxrHeadPoseState m_poseState;
    glm::mat4 m_cameraModelMatrix[kNumEyes];
    glm::mat4 m_projectionMatrix[kNumEyes];
    glm::mat4 m_viewMatrix[kNumEyes];
    bool m_cameraStarted;
    bool m_paused;
    unsigned int m_previousCameraTime;
    float m_targetEyeWidth, m_targetEyeHeight;
    int m_cameraImageWidth, m_cameraImageHeight;

    unsigned int m_rawTexture[2];
    unsigned int m_viewTexture[2];

    bool m_initDone;
    int m_numTexturesPerFrame;
    bool m_isStereoPair;
    bool m_disableGetFrameImageIsSet;
    bool gLogPartialFrame = false;
    PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC getNativeClientBuffer;
    SvrRenderTarget m_eyeTarget[2];
    EGLDisplay m_eglDisplay;

    QVRCAMERA_FRAME_FORMAT m_frameFormat;
    int m_axis_ids[3];
    int m_axis_id_signs[3];
    
    unsigned int m_frameBufferID;
    unsigned int m_frameBufferTextureID;
	bool m_doneCameraUV;
    CameraCalibrationQTI m_leftDisplay;
    CameraCalibrationQTI m_rightDisplay;
    float m_distanceToRenderPlane;
    float *m_pMapU[2];
    float *m_pMapV[2];
    XrCameraPartialFrameRequestInfoInputQTI m_pfi;
    XrCameraFrameRequestInfoInputQTI m_gfi;
    XrCameraPartialFrameRequestInfoOutputQTI m_pfo;
    XrCameraFrameRequestInfoOutputQTI m_gfo;
    XrCameraHwBufferOutputQTI m_hwBufs[MAX_IMAGES_PER_FRAME];
};

#endif
#endif

