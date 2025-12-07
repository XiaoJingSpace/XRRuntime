/**********************************************************
 * FILE: svrCameraManager.cpp
 *
 * Copyright (c) 2020-2021 QUALCOMM Technologies Inc.
 **********************************************************/
#ifdef ENABLE_CAMERA
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/system_properties.h>
#include <math.h>
#include "svrCameraManager.h"
#include "svrGeometry.h"
#include "svrShader.h"
#include "svrUtil.h"
#include "svrConfig.h"
#include "svrConfig.h"
#include "private/svrApiTimeWarp.h"
#include "svrCustomMesh.h"

#include "QVRCameraClient.h"
#define QVR_CAMDEVICE_FRAME_SYNC_PARTIAL "partial"
#define QVR_CAMCLIENT_ATTACH_STRING_FRAME_SYNC "qvrcamclient_frame_sync"
#include "QVRServiceClient.h"
#include "svrCameraUndistort.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/ext.hpp"
#include "glm/gtx/string_cast.hpp"
#include "private/svrApiCore.h"

EXTERN_VAR(float, gSensorHeadOffsetX);       //Adjustment for device physical distance from head (meters)
EXTERN_VAR(float, gSensorHeadOffsetY);       //Adjustment for device physical distance from head (meters)
EXTERN_VAR(float, gSensorHeadOffsetZ);     //Adjustment for device physical distance from head (meters)
EXTERN_VAR(int, gWarpMeshRows);
EXTERN_VAR(int, gWarpMeshCols);

//SvrCustomMesh gSvrCustomMesh;

#define GL_CLAMP_TO_BORDER 0x812D
#define TEXTURE_BORDER_COLOR                0x1004
#define MESH_ROWS_COLS 51
#define CAMERA_MESH_BORDER_SIZE 0.1
#define INVALID_UV_VALUE 1000.0
#ifndef GL_R16_EXT
#define GL_R16_EXT 0x822A
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif

#ifndef QVRSERVICE_SENSORS_ORIENTATION
#define QVRSERVICE_SENSORS_ORIENTATION    "sensors-orientation"
#endif

using namespace Svr;
using namespace glm;

extern char camTextureFragment2D_simple[];             // used in one-pass or two-pass w/no AHBs
extern char camTextureFragment2D_mono_16_integer[];    // two-pass w/AHBs for mono 16-bit integer
extern char camTextureFragment2D_mono_16_float[];      // two-pass w/AHBs for mono 16-bit float
extern char camTextureFragmentEXT_mono[];              // two-pass w/AHBs for mono 8-bit and 10-bit
extern char camTextureFragmentEXT_color[];             // two-pass w/AHBs for RGB
extern char camTextureFragmentEXT_color_swap[];        // two-pass w/AHBs for RGB with PFD support

extern char camTextureVertex[];                        // always used
extern char camTextureFragmentUV[];                    // used in one-pass

extern char displayMeshVertex[];                        // for 1-pass creation
extern char displayMeshFragment[];                      // for 1-pass creation

static QVRCAMERA_FRAME_FORMAT CamFrameFormatFromString(const char* str);
static const char* CamFrameFormatToString(QVRCAMERA_FRAME_FORMAT format);

static __inline bool CamFrameFormatIsRaw(QVRCAMERA_FRAME_FORMAT format)
{
    switch (format) {
    case QVRCAMERA_FRAME_FORMAT_RAW10_MONO:
    case QVRCAMERA_FRAME_FORMAT_RAW16_MONO:
        return true;
    default:
        return false;
    }
}
static uint64_t now_us(void) {
    struct timespec res;
    clock_gettime(CLOCK_BOOTTIME, &res);
    return ((uint64_t)1000*1000*res.tv_sec + (uint64_t)(res.tv_nsec/1e3));
}
/**********************************************************
 * Constuctor
 **********************************************************/
SvrCameraManager::SvrCameraManager()
{
    m_initDone = false;

    // Init() does initialization, but there are here as an extra precaution.
    m_cameraClient = NULL;
    m_cameraDevice = NULL;
    m_cameraStarted = false;
}

/**********************************************************
 * Destuctor
 **********************************************************/
SvrCameraManager::~SvrCameraManager()
{
}
void SvrCameraManager::unpackCameraUVs(int whichEye, int imageWidth, int imageHeight)
{
    m_pMapU[whichEye]= (float *)malloc(imageWidth * imageHeight * sizeof(float));
    m_pMapV[whichEye]= (float *)malloc(imageWidth * imageHeight * sizeof(float));
    for(int i = 0;i<imageWidth*imageHeight;i++)
    {
        m_pMapU[whichEye][i] = m_cameraUV[whichEye][i*4+0];
        m_pMapV[whichEye][i] = m_cameraUV[whichEye][i*4+1];

    }
}
/**********************************************************
 * Pause
 **********************************************************/
void SvrCameraManager::Pause()
{
    /*
     * Initialize if needed. Ideally we wouldn't get here until initialization,
     * but you never know.
     */
    if (m_initDone == false)
    {
        SxrResult result = Init();
        if (result != SXR_ERROR_NONE)
        {
            return;
        }
    }

    m_paused = true;
    glClearColor (0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    LOGI("svrCameraManager: Pausing camera: '%s'", m_cameraName);

    /*
     * If we're not using the tracking camera, attempt to stop the camera
     */
    if (strcmp(m_cameraName, QVRSERVICE_CAMERA_NAME_TRACKING) != 0)
    {
        if (m_cameraStarted == true)
        {
            if (m_cameraDevice != NULL)
            {
                (void) QVRCameraDevice_Stop(m_cameraDevice);
            }
            m_cameraStarted = false;
        }
    }
}

/**********************************************************
 * IsPaused
 **********************************************************/ 
bool SvrCameraManager::IsPaused()
{
    return m_paused;
}

/**********************************************************
 * Resume
 **********************************************************/
void SvrCameraManager::Resume()
{
    /*
     * Initialize if needed. Ideally we wouldn't get here until initialization,
     * but you never know.
     */
    if (m_initDone == false)
    {
        SxrResult result = Init();
        if (result != SXR_ERROR_NONE)
        {
            return;
        }
    }

    LOGE("svrCameraManager: Resuming camera: '%s'", m_cameraName);
    m_paused = false;

    /*
     * If we're not using the tracking camera, attempt to start the camera
     */
    if (strcmp(m_cameraName, QVRSERVICE_CAMERA_NAME_TRACKING) != 0)
    {
        if (QVRCameraDevice_Start(m_cameraDevice) < 0)
        {
            LOGE("svrCameraManager: Failed to start '%s' camera", m_cameraName);
        }
        else
        {
            m_cameraStarted = true;
        }
    }
}

/**********************************************************
 * RenderCameraUVs -  Used in One Pass camera mesh creation.
 * Camera images go through 2 lenses. We render the unwarped
 * camera image as texture coordinates.
 **********************************************************/
SxrResult SvrCameraManager::RenderCameraUVs()
{
    /*
     * Initialize if needed
     */
    if (m_initDone == false)
    {
        SxrResult result = Init();
        if (result != SXR_ERROR_NONE)
        {
            return result;
        }
    }

    /*
     * Make floating point frame buffer, render 1st pass camera UVs
     */

    glGenTextures(1, &m_frameBufferTextureID);
    glBindTexture( GL_TEXTURE_2D, m_frameBufferTextureID);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_targetEyeWidth, m_targetEyeHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    /*
     * Make frame buffer
     */
    glGenFramebuffers(1, &m_frameBufferID);
    glBindFramebuffer( GL_FRAMEBUFFER, m_frameBufferID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_frameBufferTextureID, 0);
    int errorCode = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(errorCode != GL_FRAMEBUFFER_COMPLETE)
    {
        LOGI("ERROR: Frame buffer not OK");
        return SXR_ERROR_UNKNOWN; // error
    }

    /*
     * Viewport
     */
    glClearColor (0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, m_targetEyeWidth, m_targetEyeHeight);

    /*
     * Build Shaders
     */
    Svr::SvrShader cameraShader;
    InitializeShaderFromBuffer(cameraShader, camTextureVertex, camTextureFragmentUV, "Vs", "Fs");

    /*
     * Matrix
     */
    glm::mat4 viewMatrix = mat4(1.f);
    glm::vec3 eyePos = vec3(0, 0, 0);

    LOGI("svrCameraManager: Inited cameraMeshUV buffers\n");

    /*
     * Allocate UV buffers
     */
    for (int i=0; i < 2; i++)
    {
        /*
         * Set shader parameters
         */
        cameraShader.Bind();
        cameraShader.SetUniformMat4("projectionMatrix", m_projectionMatrix[i]);
        cameraShader.SetUniformMat4("viewMatrix", m_viewMatrix[i]);
        cameraShader.SetUniformVec3("eyePos", eyePos);
        cameraShader.SetUniformMat4("modelMatrix", m_cameraModelMatrix[i]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_cameraUV[i]  = (float*)malloc(m_targetEyeWidth * m_targetEyeHeight * 4 * sizeof(float));

        /*
         * Check that camera model exists
         */
        if (m_cameraModel[i] == NULL)
        {
            InitCameraMesh();
        }

        /*
         * Render Camera UVs
         */
        if (m_cameraModel[i] != NULL)
        {
            m_cameraModel[i]->Submit();
        }
        else
        {
            LOGE("svrCameraManager: Error - No camera model to render\n");
        }
        glReadPixels(0, 0, m_targetEyeWidth, m_targetEyeHeight, GL_RGBA, GL_FLOAT, m_cameraUV[i]);
    }

    cameraShader.Destroy();
    for(int i = 0; i< 2;i++)
        unpackCameraUVs(i, m_targetEyeWidth, m_targetEyeHeight);
    return SXR_ERROR_NONE;
}
/**********************************************************
 * Camera look up for one pass mesh 
 **********************************************************/
void SvrCameraManager::GetCameraWarpCoords(float* pWarpIn, float* pWarpOut, int width, int height, int whichEye)
{
    float uu = (pWarpIn[0] + 1.0) / 2.0;
    float vv = (pWarpIn[1] + 1.0) / 2.0;
    float u = 0, v = 0;

    if (uu > 1.0 || uu < 0.0 || vv > 1.0 || vv < 0.0)
    {
        u = INVALID_UV_VALUE;
        v = INVALID_UV_VALUE;
    }
    else
    {
        TextureLookup(m_pMapU[whichEye],
                      1,
                      width,
                      height,
                      uu,
                      vv,
                      0,
                      &u);

        TextureLookup(m_pMapV[whichEye],
                      1,
                      width,
                      height,
                      uu,
                      vv,
                      0,
                      &v);
    }
    if (u > CAMERA_MESH_BORDER_SIZE && v < (2.0 - CAMERA_MESH_BORDER_SIZE))
    {
        LOGI("Valid");
    }
    else
    {
        u = INVALID_UV_VALUE;
        v = INVALID_UV_VALUE;
    }
    pWarpOut[0] = 2.0f * u - 1.0f;
    pWarpOut[1] = -1.0 * ((2.0f * v) - 1.0f);
}

/**********************************************************
 * TextureLookup
 **********************************************************/
void SvrCameraManager::TextureLookup(float* pTexture,
                                     int pixelSize,
                                     int width,
                                     int height,
                                     float uCoord,
                                     float vCoord,
                                     int component,
                                     float *pValue)
{
    if (pTexture == NULL || width == 0 || height == 0)
    {
        LOGE("svrCameraManager: ERROR - Trying to build 1 pass camera mesh without initializing first.");
        return;
    }

    if (uCoord > 1.f)
    {
        uCoord = 1.f;
    }

    if (vCoord > 1.f)
    {
        vCoord = 1.f;
    }


    /*
     * 0..1 U coordinate maps to 0..(width-1) for array access convention
     * 0..1 V coordinate maps to 0..(height-1) for array access convention
     */
    float fU = uCoord * (float)(width - 1);
    float fV = vCoord * (float)(height - 1);

    int u = ((int)fU);
    int v = ((int)fV);


    /*
     * Bottom Left
     */
    int leftOffset = (v * width * pixelSize) + (u * pixelSize) + component;
    float leftVal = *(pTexture + leftOffset);

    /*
     * Right
     */
    int uNext = u + 1;
    if (uNext >= (width - 1))
    {
        uNext = width - 1;
    }
    int rightOffset = (v * width * pixelSize) + (uNext * pixelSize) + component;
    float rightVal = *(pTexture + rightOffset);

    /*
     * Top Left
     */
    int vNext = v + 1;
    if (vNext >= (height - 1))
    {
        vNext = height - 1;
    }
    int topLeftOffset = (vNext * width * pixelSize) + (u * pixelSize) + component;
    float topLeftVal = *(pTexture + topLeftOffset);

    /*
     * Top Right
     */
    int topRightOffset = (vNext * width * pixelSize) + (uNext * pixelSize) + component;
    float topRightVal = *(pTexture + topRightOffset);


    float dU = fU - (float)u;
    float dV = fV - (float)v;

    float val = ((1.f - dU) * (1.f - dV) * leftVal) +
        (dU * (1.f - dV) * rightVal) +
        ((1.f - dU) * dV * topLeftVal) +
        (dU * dV * topRightVal);


    *pValue = val;
}


/**********************************************************
 * GenerateCameraMeshUsingQXR
 **********************************************************/
void SvrCameraManager::GenerateCameraMeshUsingQXR(SvrCameraInfo * pCameraInfo,
                                                  int cameraWidth,
                                                  int cameraHeight,
                                                  float * pMapU,
                                                  float * pMapV,
                                                  int numCameraTextures,
                                                  float offsetU,
                                                  SvrGeometry** ppOutGeometry)
{
    float imageWidth = pCameraInfo->m_width;
    float imageHeight = pCameraInfo->m_height;
    float focalX = pCameraInfo->m_focalX;
    float focalY = pCameraInfo->m_focalY;
    float centerX = imageWidth/2.f;
    float centerY = imageHeight/2.f;

    SvrGeometry * pOutGeometry = new SvrGeometry;
    *ppOutGeometry = pOutGeometry;

    centerY = imageHeight - centerY;  // image coords origin is upper left. We want lower left.

    int maxNum = MESH_ROWS_COLS -1;
    int numVerts = (maxNum + 1)*(maxNum + 1);

    SvrProgramAttribute attribs[3];
    int vertexSize = 8 * sizeof(float); // 3 pos, 3 normal, 2 uv
    float * pVertexBufferData = new float[vertexSize * numVerts];
    float * pData = pVertexBufferData;

    attribs[0].index = kPosition;
    attribs[0].size = 3;
    attribs[0].type = GL_FLOAT;
    attribs[0].normalized = false;
    attribs[0].stride = vertexSize;
    attribs[0].offset = 0;

    attribs[1].index = kNormal;
    attribs[1].size = 3;
    attribs[1].type = GL_FLOAT;
    attribs[1].normalized = false;
    attribs[1].stride = vertexSize;
    attribs[1].offset = 3 * sizeof(float);

    attribs[2].index = kTexcoord0;
    attribs[2].size = 2;
    attribs[2].type = GL_FLOAT;
    attribs[2].normalized = false;
    attribs[2].stride = vertexSize;
    attribs[2].offset = 6 * sizeof(float);

    int whichEye;
    offsetU > 0.0 ? whichEye = 1 : whichEye = 0;
    /*
     * POSITION
     */
    for (int y = 0; y <= maxNum; y++) // maxNum is number of divisions in 3D mesh. e.g. 20
    {
        /*
         * X
         */
        for (int x = 0; x <= maxNum; x++)
        {
            float fx = ((float)x / (float)maxNum) * imageWidth;
            float fy = ((float)y / (float)maxNum) * imageHeight;

            fx = (fx - centerX) / focalX;
            if (useVSTMaps)
                fy = -1.0 * (fy - centerY) / focalY;
            else
                fy = (fy - centerY) / focalY;

            /*
             *  WARP USING UVs
             */
            /*
             * Position
             */
            *pData++ = fx;
            *pData++ = fy;
            *pData++ = -1.0f;

            /*
             * Normals
             */
            *pData++ = 1.0f;
            *pData++ = 0.f;
            *pData++ = 0.f;

            float uu = ((float)x / (float)maxNum);
            float vv = ((float)y / (float)maxNum);
            float u = 0, v = 0;

            TextureLookup(pMapU,
                          1,
                          imageWidth,
                          imageHeight,
                          uu,
                          vv,
                          0,
                          &u);

            TextureLookup(pMapV,
                          1,
                          imageWidth,
                          imageHeight,
                          uu,
                          vv,
                          0,
                          &v);

            /*
             * Normalize to 0..1
             */
            u = u / (float)cameraWidth;
            v = v / (float)cameraHeight;
            
            if (numCameraTextures == 1)
            {
                /*
                 * Squeeze & shift because image is left/right side by side
                 */
                u = (u / 2.f) + offsetU;
            }

            /*
             * flip
             */
            if (!useVSTMaps)
                v = 1.f - v;

            /*
             * Assign to data array & increment
             */

            *pData++ = u;
            *pData++ = v;
        }
    }

    /*
     * Faces
     */
    int inner = 0;
    int outer = 0;
    int a, b, c, d;
    int numPerRow = maxNum;

    /*
     * Do indices
     */
    bool doQuads = false;
    int numIndices;
    if (doQuads == true)
    {
        numIndices = maxNum * maxNum * 4;
    }
    else
    {
        numIndices = maxNum * maxNum * 6;
    }
    int * pIndexBuffer = new int[numIndices];
    int * pIndex = pIndexBuffer;

    for (int y = 0; y < maxNum; y++)
    {
        for (int x = 0; x < maxNum; x++)
        {
            a = inner;
            b = inner + 1;
            c = b + (numPerRow + 1);
            d = a + (numPerRow + 1);
            if (doQuads == true)
            {
                *pIndex++ = a;
                *pIndex++ = b;
                *pIndex++ = c;
                *pIndex++ = d;
            }
            else
            {
                *pIndex++ = a;
                *pIndex++ = b;
                *pIndex++ = c;

                *pIndex++ = a;
                *pIndex++ = c;
                *pIndex++ = d;
            }
            inner++;
        }
        inner++;
    }

    pOutGeometry->Initialize(&attribs[0],
                             3,
                             (unsigned int*)pIndexBuffer,
                             numIndices,
                             (const void*)pVertexBufferData,
                             vertexSize * numVerts,
                             numVerts);
}

/**********************************************************
 * Init
 **********************************************************/
SxrResult SvrCameraManager::Init()
{
   /*
    * Options to turn things on/off
    */
    doUndistort = true; // correct fisheye distortion
    useVSTMaps = true;
    doCameraTimestamp = true;
    useHardwareBuffers = true; // using shared HW buffer from QVR Service to avoid GL texture copy during two-pass.
    useRawFormat = true;
    doTwoPassRender = true;  // see comment below
    doColorCamera = false;
    doRectify = false; 
    doR16float = true;
    doPartialFrame = true;//set this to true so that we know there was can over-ride using setprop. Logic below handles other cases.
    partialFrameFillPercentage = 55.; // property
    m_minPartialFrameFillPercentage = 10;//determine min/max from some other config prop. setting for now
    m_maxPartialFrameFillPercentage = 100;
    m_defaultPartialFrameFillPercentage = 55;

    resSel = "";
    m_distanceToRenderPlane = 1.0;
    
    if(useVSTMaps==true)
        doRectify = false; //mutually exclusive

    vfov = 90.f;   // A value of 81.f seems good for trinity, based solely on human observation.
    shiftX = 0.0f;
    shiftY = 0.0f; // A value of 0.02f seems good for trinity, so that the low mounted cameras should be raised to eye level
    shiftZ = 0.f;

    leftColorTilt = leftColorPan = leftColorRoll = 0.f;
    rightColorTilt = rightColorPan = rightColorRoll = 0.f;

    memset(m_cameraTexture, 0, sizeof(m_cameraTexture));
    m_numCameraTextures = 0;
    m_cameraModel[0] = NULL;
    m_cameraModel[1] = NULL;

    m_cameraClient = NULL;
    m_cameraDevice = NULL;
    m_cameraName = NULL;

    m_cameraUV[0] = NULL;
    m_cameraUV[1] = NULL;
    m_syncCtrl = NULL;

    m_frameNumber = -1;
    m_frameIndex = 0;

    m_gotFPS = false;
    m_okToRender = true;
    m_previousCameraTime = 0;
    m_showStatusLog = true;
    m_cameraStarted = false;
    m_paused = false;

    getNativeClientBuffer = (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)NULL;
    m_initDone = false;

    m_numTexturesPerFrame = 0;
    m_isStereoPair = false;
    m_disableGetFrameImageIsSet = false;
    m_cameraImageWidth = 0;
    m_cameraImageHeight = 0;

    m_doneCameraUV = false;

    m_frameFormat = QVRCAMERA_FRAME_FORMAT_UNKNOWN;

    /*
     * Set by "adb setprop" shell command. Just because the prop is requested, there is still
     * sanity checking to be done and properties may re-set accordingly.
     */
    GetProperties();
    if (doUndistort == false)
    {
        useVSTMaps = false;
    }

    
    /*
     * Create the camera client so we can attach to the camera
     */
    m_cameraClient = QVRCameraClient_Create();
    if (m_cameraClient == NULL)
    {
        LOGE("svrCameraManager: May not have camera permissions set. Failed to create camera client.\n");
        free(m_cameraName);
        return SXR_ERROR_UNKNOWN;
    }

    /*
     * Create the camera device
     */
    (void) CreateCameraDevice();
    if (m_cameraDevice == NULL)
    {
        LOGE("svrCameraManager: Error - Could not Attach to camera '%s'\n", m_cameraName);
        QVRCameraClient_Destroy(m_cameraClient);
        m_cameraClient = NULL;
        free(m_cameraName);
        return SXR_ERROR_UNKNOWN;
    }

    /*
     * Load camera calibration info. IF we are indeed doing lens distortion correction.
     */
    if (doUndistort == true)
    {
        int success = GetCameraCalibInfo(m_cameraDevice, m_cameraInfo);
        if (success == -1)
        {
            LOGE("svrCameraManager: Camera Calibration not loaded.\n");
            MakeDefaultCameraInfo(m_cameraDevice, m_cameraInfo);
            doUndistort = false;
            useVSTMaps = false;

            if (doColorCamera == true)
            {
                // fake some values, via adb shell setprop
                m_cameraInfo[0].m_rotation[0] = glm::radians(leftColorTilt);
                m_cameraInfo[0].m_rotation[1] = glm::radians(leftColorPan);
                m_cameraInfo[0].m_rotation[2] = glm::radians(leftColorRoll);

                m_cameraInfo[1].m_rotation[0] = glm::radians(rightColorTilt);
                m_cameraInfo[1].m_rotation[1] = glm::radians(rightColorPan);
                m_cameraInfo[1].m_rotation[2] = glm::radians(rightColorRoll);

                vfov = 90.f;
                shiftY = 0.f;
            }
        }
    }
    else
    {
        LOGE("svrCameraManager: doUndistort if false. Create default camera calib info.\n");
        MakeDefaultCameraInfo(m_cameraDevice, m_cameraInfo);
    }

    /*
     * Check if build supports PF. doPartialFrame is true when we enter unless we do not want it by setprop.
     * This code section may modify the doPartialFrame setprop. 
     * This code section needs to be called BEFORE CameraDevice( ), which depends on a valid doPartialFrame value.
     */
    uint8_t is_partial_frame_enabled=false;
    int ret = QVRCameraDevice_GetParamNum(m_cameraDevice, QVR_CAMDEVICE_UINT8_ENABLE_PARTIAL_FRAME_READ,
            QVRCAMERA_PARAM_NUM_TYPE_UINT8,1,(char*)&is_partial_frame_enabled);
    if (ret < 0) {
        m_pfdSupport = false;
        doPartialFrame = false;//PF will not work, override setprop
        LOGI("svrCameraManager: PFD is notsupported. doPartialFrame set to false");
    }
    else
    {
        if (!is_partial_frame_enabled)
        {
            LOGI("svrCameraManager: PFD supported but camera is not configured for PFD");
            doPartialFrame = false;//PF will not work, override setprop 
        }
        else
        {
            LOGI("svrCameraManager: Enabling PFD");
            if(doPartialFrame)//we did not set PFD to false using setprop. Otherwise we do not want to override it.
                doPartialFrame = true;
            else //remove log
                LOGI("svrCameraManager: PFD is set to false");
        }
        m_pfdSupport = true;
    }
    

    //Partial Frame only works with OnePass. 
    if(doPartialFrame)
    {
        doTwoPassRender=false;

        LOGI("svrCameraManager: PFD on, so doTwoPassRender is now set to false");
        
        if( partialFrameFillPercentage < m_minPartialFrameFillPercentage || 
            partialFrameFillPercentage > m_maxPartialFrameFillPercentage)
        {
            partialFrameFillPercentage = m_defaultPartialFrameFillPercentage;
        }
    }    

    /*
     * 1 if m_cameraTexture[] holds a single stereo image.
     * 2 if array is 2 single images, one for each eye
     */
    m_numCameraTextures = GetNumTexturesPerFrame();
    if (m_numCameraTextures == 1)
    {
        m_isStereoPair = true;
    }

    /*
     * Get current eglDisplay
     */
    m_eglDisplay = eglGetCurrentDisplay();

    /*
     * Get internal eye target size for render target setups.
     * Use camera image resolution as guide for eye target size, no point
     * in rendering larger than is necessary.
     */
    sxrDeviceInfo di = sxrGetDeviceInfo();
    float deviceTargetEyeWidth = (float)di.targetEyeWidthPixels;
    float deviceTargetEyeHeight = (float)di.targetEyeHeightPixels;
    float deviceTargetAspect = deviceTargetEyeWidth / deviceTargetEyeHeight;

    m_targetEyeWidth = deviceTargetEyeWidth;
    m_targetEyeHeight = deviceTargetEyeHeight;


    float headposX = gSensorHeadOffsetX;
    int status = SetDisplayProperties(&di);
    if(status !=0)
    {

        LOGE("svrCameraManager:Unable to set display properties");
    }


    /*
     * Set up internal eye buffers used for 2 pass rendering (undistoring camera image as step 1).
     */
    for (int i = 0; i < 2; i++)
    {
        m_eyeTarget[i].Initialize(m_targetEyeWidth, m_targetEyeHeight, 1, GL_RGBA8, true);
    }

    /*
     * Set up shaders
     */
    if (doTwoPassRender == true)
    {
        if (useHardwareBuffers == true)
        {
            switch (m_frameFormat)
            {
                case QVRCAMERA_FRAME_FORMAT_RAW16_MONO:
                    if (doR16float == true)
                    {
                        // 16 bit raw float texture - GL_LINEAR texture filtering
                        InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragment2D_mono_16_float , "Vs", "Fs");
                    }
                    else
                    {
                        // 16 bit raw int texture - GL_NEAREST texture filtering
                        InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragment2D_mono_16_integer , "Vs", "Fs");
                    }
                    break;
                case QVRCAMERA_FRAME_FORMAT_RAW10_MONO:
                case QVRCAMERA_FRAME_FORMAT_Y8:
                    // 10 bit raw or 8 bit pixel texture - GL_LINEAR texture filtering
                    InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragmentEXT_mono , "Vs", "Fs");
                    break;
                case QVRCAMERA_FRAME_FORMAT_YUV420:
                    // RGB pixel texture - GL_LINEAR texture filtering
                    if(m_pfdSupport)
                    {
                        InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragmentEXT_color_swap, "Vs", "Fs");
                    }
                    else
                    {
                        InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragmentEXT_color, "Vs", "Fs");
                    }
                    break;
                default:
                    LOGE("svrCameraManager: oops! We don't have support for this format: %d", m_frameFormat);
                    QVRCameraDevice_DetachCamera(m_cameraDevice);
                    m_cameraDevice = NULL;
                    QVRCameraClient_Destroy(m_cameraClient);
                    m_cameraClient = NULL;
                    free(m_cameraName);
                    return SXR_ERROR_UNKNOWN;
            }
        }
        else
        {
            InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragment2D_simple, "Vs", "Fs");
        }

    }

    /*
     * For each eye, calculate camera model and projection matrices. Use for rendering
     * camera images, whether color (2pass) or UV (1pass)
     */
    float aspect = (float)di.targetEyeWidthPixels / (float) di.targetEyeHeightPixels;
    for (int i = 0; i < 2; i++)
    {
        if(!useVSTMaps)
        {
            if (doRectify == true)
            {
                float rotX = m_cameraInfo[i].m_rotation[0];
                float rotY = m_cameraInfo[i].m_rotation[1];
                float rotZ = m_cameraInfo[i].m_rotation[2];

                // Translate plane out, then rotate around a 1.0 unit radius, origin is pivot.
                glm::mat4 rotateMatrix = glm::eulerAngleXYZ(rotX, rotY, rotZ);
                glm::mat4 result = glm::translate(rotateMatrix, glm::vec3(0.0, 0.0f, -1.));

                // Allow user to shift camera around
                if (i == 1)
                {
                    shiftX *= -1.f; // mirror the X (horizontal) shift between eyes.
                }
                glm::mat4 shiftMatrix = glm::translate(glm::mat4(1.f), glm::vec3(shiftX, -1.f * shiftY, shiftZ));
                glm::mat4 final = shiftMatrix * result;

                m_cameraModelMatrix[i] = final;
                float verticalFOVradians = glm::radians(vfov);
                m_projectionMatrix[i] = glm::perspective(verticalFOVradians, aspect, .1f, 100.f);
            }
            else
            {
                m_cameraModelMatrix[i] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, -1.));

                m_projectionMatrix[i] = glm::ortho(-aspect, aspect,
                                                   -1.f, 1.f,
                                                   .1f, 100.f);
            }
        }
        else
        {
            //useVSTMaps
            glm::fquat poseRot; // Identity
            glm::vec3 posePos;  // Identity

            m_poseState.pose.rotation.x = poseRot.x;
            m_poseState.pose.rotation.y = poseRot.y;
            m_poseState.pose.rotation.z = poseRot.z;
            m_poseState.pose.rotation.w = poseRot.w;

            m_poseState.pose.position.x = posePos.x;
            m_poseState.pose.position.y = posePos.y;
            m_poseState.pose.position.z = posePos.z;
            // add in changes for view matrix view customization parameters
            glm::vec3 LeftEyePos(di.leftEyeFrustum.position.x, di.leftEyeFrustum.position.y, di.leftEyeFrustum.position.z);
            glm::fquat LeftEyeRot(di.leftEyeFrustum.rotation.w, di.leftEyeFrustum.rotation.x, di.leftEyeFrustum.rotation.y, di.leftEyeFrustum.rotation.z); // glm quat is (w)(xyz)
            glm::vec3 RightEyePos(di.rightEyeFrustum.position.x, di.rightEyeFrustum.position.y, di.rightEyeFrustum.position.z);
            glm::fquat RightEyeRot(di.rightEyeFrustum.rotation.w, di.rightEyeFrustum.rotation.x, di.rightEyeFrustum.rotation.y, di.rightEyeFrustum.rotation.z); // glm quat is (w)(xyz)

            SvrGetEyeViewMatrices(m_poseState,
                                  true, // use a head model?
                                  LeftEyePos, RightEyePos,
                                  LeftEyeRot, RightEyeRot,
                                  DEFAULT_HEAD_HEIGHT, DEFAULT_HEAD_DEPTH,
                                  m_viewMatrix[kLeftEye],
                                  m_viewMatrix[kRightEye]);


            //update projection matrix
            sxrViewFrustum *pLeftFrust = &di.leftEyeFrustum;

            m_projectionMatrix[kLeftEye] = glm::frustum(pLeftFrust->left, pLeftFrust->right, pLeftFrust->bottom,
                                                        pLeftFrust->top, pLeftFrust->near, pLeftFrust->far);


            sxrViewFrustum *pRightFrust = &di.rightEyeFrustum;
            m_projectionMatrix[kRightEye] = glm::frustum(pRightFrust->left, pRightFrust->right,
                                                         pRightFrust->bottom, pRightFrust->top,
                                                         pRightFrust->near, pRightFrust->far);


            m_cameraModelMatrix[kLeftEye] = glm::translate(glm::mat4(1.0f), glm::vec3(m_viewMatrix[kLeftEye][3][0], m_viewMatrix[kLeftEye][3][1],
                                                                                      0.0f));

            m_cameraModelMatrix[kRightEye] = glm::translate(glm::mat4(1.0f), glm::vec3(m_viewMatrix[kRightEye][3][0], m_viewMatrix[kRightEye][3][1],
                                                                                       0.0f));


        }
    } 
    

    /*
     * If we're not using the tracking camera, attempt to start the camera
     */
    if (strcmp(m_cameraName, QVRSERVICE_CAMERA_NAME_TRACKING) != 0)
    {
        if (QVRCameraDevice_Start(m_cameraDevice) < 0)
        {
            LOGE("svrCameraManager: Failed to start '%s' camera", m_cameraName);
        }
        else
        {
            m_cameraStarted = true;
        }
    }

    m_viewTexture[0] = 0;
    m_viewTexture[1] = 0;
    m_rawTexture[0] = 0;
    m_rawTexture[1] = 0;

    m_initDone = true;
    return SXR_ERROR_NONE;
}

/**********************************************************
 * InitCameraMesh - Used in 2 pass, by Render()
 **********************************************************/
SxrResult SvrCameraManager::InitCameraMesh()
{
    /*
     * Create render mesh for doing camera lens undistort
     */
    while (doUndistort == true)
    {
        if (useVSTMaps)
        {
            /*
             * Prep structures to get Undistort/Rectify maps from QVR
             */
            float *mapX[2]; // one per eye
            float *mapY[2];
            int mapSize = m_targetEyeWidth * m_targetEyeHeight * sizeof(float);
            mapX[0] = (float *)malloc(mapSize);
            mapY[0] = (float *)malloc(mapSize);
            mapX[1] = (float *)malloc(mapSize);
            mapY[1] = (float *)malloc(mapSize);

            LOGI("svrCameraManager: alloced cal. map buffers for vst of size = %d\n", mapSize);
            if (mapX[0] == NULL || mapY[0] == NULL || mapX[1] == NULL || mapY[1] == NULL)
            {
                LOGE("svrCameraManager: Error: failed to alloc cal. map buffers for vst.\n");
                doUndistort = false;
                break;
            }

            XrCameraDevicePropertiesQTI cameraProperties;
            int result = QVRCameraDevice_GetProperties(m_cameraDevice, &cameraProperties);
            if (result < 0)
            {
                LOGE("svrCameraManager: Error: failed to get camera properties: %d\n", result);
                doUndistort = false;
                break;
            }

            /*
             * Get camera lens correction map
             */
            svrCameraUndistort *UndistortDriver = new svrCameraUndistort();
            if (!UndistortDriver->Init(cameraProperties, 1.0f))
            {
                LOGE("svrCameraManager: Error: failed to create undistort library\n");
                delete UndistortDriver;
                doUndistort = false;
                break;
            }

            int numPixels = m_targetEyeWidth * m_targetEyeHeight;
            bool success = UndistortDriver->GetVSTMaps(mapX[0], mapY[0], mapX[1], mapY[1],
                                                       numPixels, &m_leftDisplay, &m_rightDisplay, m_distanceToRenderPlane);

            if (!success)
            {

                LOGE("svrCameraManager: Error: failed to create vst maps\n");
                delete UndistortDriver;
                doUndistort = false;
                break;
            }
            /*
             * Generate left / right eye meshes for rendering
             */
           
            for (int i = 0; i < 2; i++)
            {
                /*
                 * Generate undistort mesh from fisheye calibration data
                 */
                float offsetU = i * 0.5f; // there is 1 camera image with left/right images next to each other.

                SvrGeometry *pCameraGeometry;
                GenerateCameraMeshUsingQXR(&m_displayInfo[i],
                                           m_cameraImageWidth,
                                           m_cameraImageHeight,
                                           mapX[i],
                                           mapY[i],
                                           m_numCameraTextures,
                                           offsetU,
                                           &pCameraGeometry);
                
                
                m_cameraModel[i] = pCameraGeometry;
            }
            /*
             * Free stuff
             */
            delete UndistortDriver;
            free(mapX[0]);
            free(mapY[0]);
            free(mapX[1]);
            free(mapY[1]);

            break;
        }
        else
        {
            //generate mesh using VST maps
            float *mapX[2]; // one per eye
            float *mapY[2];
            int mapSize = m_cameraImageWidth * m_cameraImageHeight * sizeof(float);
            mapX[0] = (float *)malloc(mapSize);
            mapY[0] = (float *)malloc(mapSize);
            mapX[1] = (float *)malloc(mapSize);
            mapY[1] = (float *)malloc(mapSize);

            if (mapX[0] == NULL || mapY[0] == NULL || mapX[1] == NULL || mapY[1] == NULL)
            {
                LOGE("svrCameraManager: Error: failed to get map buffers for undistortion.\n");
                doUndistort = false;
                useVSTMaps = false;
                break;
            }

            XrCameraDevicePropertiesQTI cameraProperties;
            int result = QVRCameraDevice_GetProperties(m_cameraDevice, &cameraProperties);
            if (result < 0)
            {
                LOGE("svrCameraManager: Error: failed to get camera properties: %d\n", result);
                doUndistort = false;
                useVSTMaps = false;
                break;
            }

            /*
             * Get camera lens correction map
             */
            svrCameraUndistort *UndistortDriver = new svrCameraUndistort();
            if (!UndistortDriver->Init(cameraProperties, 1.0f))
            {
                LOGE("svrCameraManager: Error: failed to create undistort library\n");
                delete UndistortDriver;
                doUndistort = false;
                useVSTMaps = false;
                break;
            }

            /*
             * Rectify applies camera extriniscs
             */
            int numPixels = m_cameraImageWidth * m_cameraImageHeight;
            /*
             * Do not apply camera extrinsics, correct lens only
             */
            (void)UndistortDriver->GetUndistortMaps(mapX[0],
                                                    mapY[0],
                                                    mapX[1],
                                                    mapY[1],
                                                    numPixels);

            /*
             * Generate left / right eye meshes for rendering
             */
            for (int i = 0; i < 2; i++)
            {
                /*
                 * Generate undistort mesh from fisheye calibration data
                 */
                float offsetU = i * 0.5f; // there is 1 camera image with left/right images next to each other.

                SvrGeometry *pCameraGeometry;
                GenerateCameraMeshUsingQXR(&m_cameraInfo[i],
                                           m_cameraImageWidth,
                                           m_cameraImageHeight,
                                           mapX[i],
                                           mapY[i],
                                           m_numCameraTextures,
                                           offsetU,
                                           &pCameraGeometry);

                m_cameraModel[i] = pCameraGeometry;
            }

            /*
             * Free stuff
             */
            delete UndistortDriver;
            free(mapX[0]);
            free(mapY[0]);
            free(mapX[1]);
            free(mapY[1]);

            break;
        }
    }//while ends

    if (doUndistort == false)
    {
        /*
         * Render without camera image correction.
         */
        LOGE("svrCameraManager: No lens correction.\n");

        useVSTMaps = false;

        /*
         * For each eye
         */
        for (int i = 0; i < 2; i++)
        {
            float offsetU = i * 0.5f; // there is 1 camera image with left/right images next to each other.

            SvrGeometry *pCameraGeometry;

            GenerateDefaultMesh(offsetU, &m_cameraInfo[i], &pCameraGeometry);
            m_cameraModel[i] = pCameraGeometry;
        }
    }

    return SXR_ERROR_NONE;
}

/**********************************************************
 * Update
 **********************************************************/
SxrResult SvrCameraManager::Update()
{
    SxrResult result;

    /*
     * Initialize if needed
     */
    if (m_initDone == false)
    {
        result = Init();
        if (result != SXR_ERROR_NONE)
        {
            return result;
        }
    }

    m_okToRender = true; // assume ok

    /*
     * Nothing to do if paused
     */
    if (m_paused == true)
    {
        result = SXR_ERROR_NONE;
        return result; 
    }

    qvrcamera_frame_t cameraFrame;

    int status = 0;
    QVRCAMERA_CAMERA_STATUS camState = QVRCAMERA_CAMERA_CLIENT_DISCONNECTED;
    (void) QVRCameraDevice_GetCameraState(m_cameraDevice, &camState);
    if (camState != QVRCAMERA_CAMERA_STARTED)
    {
        LOGE("svrCameraManager: Camera not started yet.\n");
        m_okToRender = false;
        m_frameNumber = -1;
        return SXR_ERROR_UNKNOWN;
    }

    if(!doPartialFrame)
    {
        if (m_frameNumber < 0)
        {
            status = QVRCameraDevice_GetCurrentFrameNumber(m_cameraDevice, &m_frameNumber);
            if (status < 0)
            {
                LOGE("svrCameraManager: GetCurrentFrameNumber() failed: %d\n", status);
                m_okToRender = false;
                return SXR_ERROR_UNKNOWN;
            }
        }
    }
    else
    {
        //if PF, set for every frame
        m_frameNumber = -1;
        m_pfi.fillPercentage = (uint32_t) partialFrameFillPercentage; // we request the minimum fill (> 10, <=100)
        m_gfi.frameNum = 0;
        m_gfi.dropMode = XR_CAMERA_DROP_MODE_QTI_NEWER_IF_AVAILABLE;
        m_gfo.hwBufferInfo.hwBufferCapacityIn = ARRAY_SIZE(m_hwBufs);
        m_gfo.hwBufferInfo.hwBuffers = m_hwBufs;
        cameraFrame.start_of_exposure_ts = m_gfo.frameInfo.startOfExposureNs;
        cameraFrame.exposure=m_gfo.frameInfo.exposureNs;
    	cameraFrame.secondary_width=0;
    	cameraFrame.secondary_height=0;
    	cameraFrame.gain=m_gfo.frameInfo.gain;
    	cameraFrame.format=QVRCAMERA_FRAME_FORMAT_YUV420;
    }
    if (m_syncCtrl != NULL)
    {
        if(doPartialFrame)
        {
            m_gfi.blockMode = XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING_SYNC;
            status = QVRCameraDevice_GetFrameEx(m_cameraDevice, &m_gfi, &m_gfo);
             m_frameNumber = m_gfo.frameNum;
        }
        else
        {
            status = QVRCameraDevice_GetFrame(m_cameraDevice, &m_frameNumber, QVRCAMERA_MODE_NON_BLOCKING_SYNC,
                                              QVRCAMERA_MODE_NEWER_IF_AVAILABLE, &cameraFrame);
        }
    }
    else
    {
        if (doPartialFrame)
        {
            m_gfi.blockMode = XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING;
            status = QVRCameraDevice_GetFrameEx(m_cameraDevice, &m_gfi, &m_gfo);
             m_frameNumber = m_gfo.frameNum;
        }
        status = QVRCameraDevice_GetFrame(m_cameraDevice, &m_frameNumber, QVRCAMERA_MODE_NON_BLOCKING,
                                          QVRCAMERA_MODE_NEWER_IF_AVAILABLE, &cameraFrame);
    }

    if (status != QVR_CAM_SUCCESS)
    {
        m_frameNumber = -1;
        m_okToRender = false;
        return SXR_ERROR_UNKNOWN;
    }

    int cameraWidth = cameraFrame.width;
    int cameraHeight = cameraFrame.height;
    int64_t now = (int64_t)now_us(); 
    
    /*
     *log image age
     */
    if (gLogPartialFrame) {
        if (doPartialFrame)
        {
            int64_t age = now - (int64_t)m_gfo.frameInfo.startOfExposureNs / 1000LL - m_gfo.frameInfo.exposureNs / 1000LL;
            LOGI("SvrCamerManager:PartialFrame fn=%d, fill=%d (requested = %d), age=%6.1f, exp=%6.1f\n",
                 m_frameNumber,
                 m_pfo.fillPercentage,
                 m_pfi.fillPercentage,
                 (float)(age) / 1000.0f,
                 (float)((int64_t)cameraFrame.exposure / 1000000LL));
        }
        else
        {
            int64_t age = now - (int64_t)cameraFrame.start_of_exposure_ts / 1000LL - cameraFrame.exposure / 1000LL;
            LOGI("SvrCamerManager:LegacyFrame fn=%d, age=%6.1f, exp=%6.1f\n",
                 m_frameNumber,
                 (float)(age) / 1000.0f,
                 (float)((int64_t)cameraFrame.exposure / 1000000LL));
        }
    }
    /*
     * numTextures should be the same as m_numCameraTextures if all goes well.
     */
    int numTextures = 1;
    if (useHardwareBuffers == true)
    {
        numTextures = AddFrame(m_cameraDevice, cameraFrame, m_cameraTexture);


        if (numTextures == 0)
        {
            /*
             * There may not be proper support for the raw format in the graphics driver.
             * Try re-creating the camera device in non-raw mode and try again.
             */
            if (useRawFormat == true && CamFrameFormatIsRaw(m_frameFormat))
            {
                LOGI("svrCameraManager: re-creating camera device in non-raw mode");

                ReleaseFrame(0, 0); // args not used
                m_frameNumber = -1;

                useRawFormat = false;
                CreateCameraDevice();
                if (m_cameraDevice == NULL)
                {
                    LOGE("svrCameraManager: error re-creating camera device");
                }
                /*
                 * We don't have a frame anymore, so return here
                 */
                m_okToRender = false;
                return SXR_ERROR_UNKNOWN;
            }

            /*
             * There may not be support for the hardware buffer format in the graphics driver.
             * Fall back to 2 pass mode with hardware buffers disabled.
             */
            LOGE("svrCameraManager:  Shared Camera Image failed. Using non-shared 2 pass render mode.");
            useHardwareBuffers = false;
            doTwoPassRender = true;
            glGenTextures(1, m_cameraTexture);
            m_numCameraTextures = 1;
            InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragment2D_simple, "Vs", "Fs");
        }
        else if (m_disableGetFrameImageIsSet == false)
        {
            LOGE("svrCameraManager: disableGetFrame\n");

            /*
             * We can use hardware buffers, so disable GetFrame() images for a performance boost
             */
            if (QVRCameraDevice_SetParam(m_cameraDevice, QVR_CAMDEVICE_STRING_GET_FRAME_IMG_DISABLE, "true") < 0)
            {
                LOGE("svrCameraManager: disabling GetFrame() images failed! Oh well, we tried...");
            }
            else
            {
                LOGI("svrCameraManager: GetFrame() images successfully disabled %d", m_paused);
            }
            m_disableGetFrameImageIsSet = true;
        }
    }

    /*
     * Print out which of the four methods we're using.
     */
    if (m_showStatusLog)
    {
        if (useHardwareBuffers == true && doTwoPassRender == false)
        {
            LOGE("svrCameraManager: Using Single Pass, Shared Buffer mode\n");
            m_showStatusLog = false;
        }
        else if (useHardwareBuffers == true && doTwoPassRender == true)
        {
            LOGE("svrCameraManager: Using Two Pass, Shared Buffer mode\n");
            m_showStatusLog = false;
        }
        else if (useHardwareBuffers == false && doTwoPassRender == true)
        {
            LOGE("svrCameraManager: Using Two Pass, Non-Shared Buffer mode\n");
            m_showStatusLog = false;
        }
        else
        {
            // should never get here InitializeModel() tries to prevent this.
            LOGE("svrCameraManager: ERROR, invalid app settings\n");
            m_showStatusLog = false;
        }
    }

    /*
     * We are not using a shared buffer OR our sharing attempt failed above.
     */
    if (useHardwareBuffers == false)
    {
        LOGE("svrCameraManager: useHardwareBuffers is false\n");
        /*
         * COPY the pixels via glTexImage2D, back to the GPU. Renders fine.
         */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_cameraTexture[0]);

        unsigned char * pPixels = (unsigned char*)cameraFrame.buffer;
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     cameraWidth,
                     cameraHeight,
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     pPixels);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    m_frameIndex++;

    unsigned int delta = 0;

    /*
     * Calculate the camera FPS so we can display its stat.
     */
    if (m_gotFPS == false)
    {
        if (m_previousCameraTime == 0)
        {
            m_previousCameraTime = cameraFrame.start_of_exposure_ts;
        }
        else
        {
            delta = (cameraFrame.start_of_exposure_ts - m_previousCameraTime);
            m_previousCameraTime = cameraFrame.start_of_exposure_ts;

            if (m_frameIndex > 150 && delta != 0)
            {
                int millisec = delta / 1000000;
                LOGE("svrCameraManager: CAMERA MS: %d  FRAME (%d)", millisec, m_frameIndex);
                m_gotFPS = true;
            }
        }
    }

    /*
     * Get the pose of the camera at the time of the camera frame exposure start
     */
    if (doCameraTimestamp == true)
    {
        uint64_t cameraFrameTime = cameraFrame.start_of_exposure_ts + (uint64_t)(cameraFrame.exposure/2); // Nanoseconds from kernel boot time (?)
        m_poseState = sxrGetHistoricHeadPose(cameraFrameTime);
        if(m_poseState.poseStatus !=  (kTrackingRotation | kTrackingPosition))
        {
            // Pose quality was NOT good enough.
            LOGE("svrCameraManager: Historic Data pose quality was too low! status: %d", m_poseState.poseStatus);
        }
    }
    else
    {
        float predDispTime = sxrGetPredictedDisplayTime();
        m_poseState = sxrGetPredictedHeadPose(predDispTime);
    }
   
    return SXR_ERROR_NONE;
}


/**********************************************************
 * Render - Used in 2 pass. This class renders the camera
 * image in a 1st pass to remove the lens distortion. Later
 * on, in svrApiTimeWarp, a second render pass is made to handle
 * display lens correction.
 **********************************************************/
SxrResult SvrCameraManager::Render()
{
    SxrResult result;

    /*
     * Initialize if needed
     */
    if (m_initDone == false)
    {
        result = Init();
        if (result != SXR_ERROR_NONE)
        {
            return result;
        }
    }

    if (m_okToRender == false)
    {
        return SXR_ERROR_UNKNOWN;
    }

    /*
     * Render images.
     */
    RenderCamera(kLeftEye);
    RenderCamera(kRightEye);

    /*
     * glViewport was modified in RenderCamera(), restore it to full screen.
     */
    GL(glViewport(0, 0, gAppContext->modeContext->warpRenderSurfaceWidth, gAppContext->modeContext->warpRenderSurfaceHeight));

    return SXR_ERROR_NONE;
}


/**********************************************************
 * ReleaseFrame
 **********************************************************/
void SvrCameraManager::ReleaseFrame(int64_t trelease, int64_t trendertop) // args not used?
{
    /*
     * Release camera frames, we're done rendering.
     */
    if (m_frameNumber != -1)
    {
        QVRCameraDevice_ReleaseFrame(m_cameraDevice, m_frameNumber);
    }

    for (int i=0; i < 2; i++)
    {
        if (m_rawTexture[i] != 0)
        {
            GL(glDeleteTextures(1, (const GLuint*) &m_rawTexture[i]));
            m_rawTexture[i] = 0;
        }

        if (m_viewTexture[i] != 0)
        {
            GL(glDeleteTextures(1, (const GLuint*) &m_viewTexture[i]));
            m_viewTexture[i] = 0;
        }
    }
}

/**********************************************************
 * RenderCamera - Used in 2 pass rendering: We do not
 * combine camera lens undisort with display lens undistort.
 **********************************************************/
void SvrCameraManager::RenderCamera(sxrWhichEye whichEye)
{
    /*
     * We render the camera to the eye target first.
     */
    m_eyeTarget[whichEye].Bind();

    glViewport(0, 0, m_targetEyeWidth, m_targetEyeHeight);
    glClearColor(0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    if (m_paused == true)
    {
        m_eyeTarget[whichEye].Unbind();
        return;
    }
    
    //useVSTCalMaps setup only supported.
    //Will need to add prev implementation if need to support undistort etc.

    m_cameraShader.Bind();
    m_cameraShader.SetUniformMat4("projectionMatrix", m_projectionMatrix[(int)whichEye]);
    m_cameraShader.SetUniformMat4("viewMatrix", m_viewMatrix[(int)whichEye]);

    /*
     * Set the OpenGL texture type. If using shared hardware buffers,
     * the texture sampler type is "external", memory not allocated by OpenGL
     */
    int textureIndex = (int)whichEye;
    unsigned int samplerType = GL_TEXTURE_2D;
    if (m_frameFormat != QVRCAMERA_FRAME_FORMAT_RAW16_MONO)
    {
        if (useHardwareBuffers == true)
        {
            samplerType = GL_TEXTURE_EXTERNAL_OES;
        }
    }

    if  (m_numCameraTextures == 1)
    {
        /*
         * Using a single texture.
         */
        textureIndex = 0;
    }

    m_cameraShader.SetUniformSampler("srcTex", m_cameraTexture[textureIndex], samplerType, 0);
    m_cameraShader.SetUniformMat4("modelMatrix", m_cameraModelMatrix[whichEye]);

    if (m_cameraModel[whichEye] == NULL)
    {
        LOGE("svrCameraManager: InitCameraMesh()\n");
        InitCameraMesh();
    }

    if (m_cameraModel[whichEye] != NULL)
    {
        m_cameraModel[whichEye]->Submit();
    }
    else
    {
        LOGE("svrCameraManager: Error - No camera model to render\n");
    }

    m_cameraShader.Unbind();

    m_eyeTarget[whichEye].Unbind();
}


/**********************************************************
 * IsStereoPair
 **********************************************************/
bool SvrCameraManager::IsStereoPair()
{
    return m_isStereoPair;
}

/**********************************************************
 * GetNumImagesPerFrame
 **********************************************************/
int SvrCameraManager::GetNumTexturesPerFrame()
{
    char driver[32];
    uint32_t len = sizeof(driver);
    if (QVRCameraDevice_GetParam(m_cameraDevice,
                                 QVR_CAMDEVICE_STRING_CAMERA_DRIVER,
                                 &len,
                                 driver) < 0)
    {
        LOGE("Failed to query camera device for driver name");
        return 0;
    }

    /*
     * cameras that use the 'native_pair' type can return two separate image buffers
     */
    if (strcmp(driver, QVR_CAMDEVICE_CAMERA_DRIVER_NATIVE_PAIR) == 0)
    {
        return 2;
    }

    return 1;
}


/**********************************************************
 * MakeDefaultCameraInfo - Loading camera lens calibration
 * failed, fake info to help default mesh creation.
 **********************************************************/
void SvrCameraManager::MakeDefaultCameraInfo(qvrcamera_device_helper_t* pCamDevice, SvrCameraInfo *pCamera)
{
    LOGE("svrCameraManager: Fabricating camera calib info instead.");

    uint16_t width, height;
    QVRCameraDevice_GetParamNum(pCamDevice,
                                QVR_CAMDEVICE_UINT16_CAMERA_FRAME_WIDTH,
                                QVRCAMERA_PARAM_NUM_TYPE_UINT16,
                                sizeof(uint16_t),(char*)&width);

    QVRCameraDevice_GetParamNum(pCamDevice,
                                QVR_CAMDEVICE_UINT16_CAMERA_FRAME_HEIGHT,
                                QVRCAMERA_PARAM_NUM_TYPE_UINT16,
                                sizeof(uint16_t),
                                (char*)&height);

    float imageWidth = (float)width / 2.f;
    float imageHeight = (float)height;

    float focal = (269.f / 400.f) * height;
    for (int i=0; i < 2; i++)
    {
        m_cameraInfo[i].m_id = i;
        m_cameraInfo[i].m_width = imageWidth;
        m_cameraInfo[i].m_height = imageHeight;
        m_cameraInfo[i].m_centerX = imageWidth / 2.f;
        m_cameraInfo[i].m_centerY = imageHeight / 2.f;
        m_cameraInfo[i].m_focalX = focal;
        m_cameraInfo[i].m_focalY = focal;
    }
}

/**********************************************************
 * GenerateDefaultMesh - Does no warping
 **********************************************************/
void SvrCameraManager::GenerateDefaultMesh(float offsetU,
                                           SvrCameraInfo * pCameraInfo,
                                           SvrGeometry** ppOutGeometry)
{
    float imageWidth = pCameraInfo->m_width;
    float imageHeight = pCameraInfo->m_height;
    float centerX = pCameraInfo->m_centerX;
    float centerY = pCameraInfo->m_centerY;
    float focalX = pCameraInfo->m_focalX;
    float focalY = pCameraInfo->m_focalY;
    float distortion[1];

    SvrGeometry * pOutGeometry = new SvrGeometry;
    *ppOutGeometry = pOutGeometry;

    distortion[0] = pCameraInfo->m_radialDisortion[0];
    centerY = imageHeight - centerY;  // image coords origin is upper left. We want lower left.

    int maxNum = 50;
    int numVerts = (maxNum + 1)*(maxNum + 1);

    SvrProgramAttribute attribs[3];
    int vertexSize = 8 * sizeof(float); // 3 pos, 3 normal, 2 uv
    float * pVertexBufferData = new float[vertexSize * numVerts];
    float * pData = pVertexBufferData;

    attribs[0].index = kPosition;
    attribs[0].size = 3;
    attribs[0].type = GL_FLOAT;
    attribs[0].normalized = false;
    attribs[0].stride = vertexSize;
    attribs[0].offset = 0;

    attribs[1].index = kNormal;
    attribs[1].size = 3;
    attribs[1].type = GL_FLOAT;
    attribs[1].normalized = false;
    attribs[1].stride = vertexSize;
    attribs[1].offset = 3 * sizeof(float);

    attribs[2].index = kTexcoord0;
    attribs[2].size = 2;
    attribs[2].type = GL_FLOAT;
    attribs[2].normalized = false;
    attribs[2].stride = vertexSize;
    attribs[2].offset = 6 * sizeof(float);

    /*
     * POSITION
     */
    for (int y = 0; y <= maxNum; y++) // maxNum is number of divisions in 3D mesh. e.g. 20
    {
        /*
         * X
         */
        for (int x = 0; x <= maxNum; x++)
        {
            float fx = ((float)x / (float)maxNum) * imageWidth;
            float fy = ((float)y / (float)maxNum) * imageHeight;

            fx = (fx - centerX) / focalX;
            fy = (fy - centerY) / focalY;

            fx = (((float)x / (float)maxNum) * 2.f) - 1.f;
            fy = (((float)y / (float)maxNum) * 2.f) - 1.f;

            /*
             *  WARP USING UVs
             */
            /*
             * Do tracking camera unwarp using Texture Coordinates
             */
            /*
             * Position
             */
            *pData++ = fx;
            *pData++ = fy;
            *pData++ = 0.f;

            /*
             * Normals
             */
            *pData++ = 1.0f;
            *pData++ = 0.f;
            *pData++ = 0.f;

            float u = ((float)x / (float)maxNum);
            float v = ((float)y / (float)maxNum);

            if (m_numCameraTextures == 1)
            {
                /*
                 * Squeeze & shift because image is left/right side by side
                 */
                u = (u / 2.f) + offsetU;
            }

            /*
             * flip
             */
            v = 1.f - v;

            *pData++ = u;
            *pData++ = v;
        }
    }

    /*
     * Faces
     */
    int inner = 0;
    int outer = 0;
    int a, b, c, d;
    int numPerRow = maxNum;

    /*
     * Do indices
     */
    bool doQuads = false;
    int numIndices;
    if (doQuads == true)
    {
        numIndices = maxNum * maxNum * 4;
    }
    else
    {
        numIndices = maxNum * maxNum * 6;
    }
    int * pIndexBuffer = new int[numIndices];
    int * pIndex = pIndexBuffer;

    for (int y = 0; y < maxNum; y++)
    {
        for (int x = 0; x < maxNum; x++)
        {
            a = inner;
            b = inner + 1;
            c = b + (numPerRow + 1);
            d = a + (numPerRow + 1);
            if (doQuads == true)
            {
                *pIndex++ = a;
                *pIndex++ = b;
                *pIndex++ = c;
                *pIndex++ = d;
            }
            else
            {
                *pIndex++ = a;
                *pIndex++ = b;
                *pIndex++ = c;

                *pIndex++ = a;
                *pIndex++ = c;
                *pIndex++ = d;
            }
            inner++;
        }
        inner++;
    }

    pOutGeometry->Initialize(&attribs[0],
                             3,
                             (unsigned int*)pIndexBuffer,
                             numIndices,
                             (const void*)pVertexBufferData,
                             vertexSize * numVerts,
                             numVerts);
}

/**********************************************************
 * GetProperties helpers
 **********************************************************/
#define INIT_PROPERTY_BOOL(p)                                           \
    {                                                                   \
        char propValue[PROP_VALUE_MAX];                                 \
        GetProperty("persist.sxr." #p, propValue,                       \
                    sizeof(propValue), p ? "true" : "false");           \
        p = (propValue[0] == 't');                                      \
        LOGE("svrCameraManager: Property '%s' is %s", #p, p ? "true" : "false"); \
    }                                                                   \

#define INIT_PROPERTY_NUM(p)                                            \
    {                                                                   \
        char propValue[PROP_VALUE_MAX];                                 \
        std::string val = std::to_string(p);                            \
        GetProperty("persist.sxr." #p, propValue, sizeof(propValue), val.c_str()); \
        p = (float)atof(propValue);                                     \
        LOGE("svrCameraManager: Property '%s' is %f ", # p, (float)p);  \
    }


/**********************************************************
 * GetProperties
 **********************************************************/
void SvrCameraManager::GetProperties()
{
    INIT_PROPERTY_BOOL(doUndistort);
    INIT_PROPERTY_BOOL(doTwoPassRender);
    INIT_PROPERTY_BOOL(doColorCamera);
    INIT_PROPERTY_BOOL(doRectify);
    INIT_PROPERTY_BOOL(doR16float);
    INIT_PROPERTY_BOOL(doPartialFrame);
    INIT_PROPERTY_NUM(vfov);
    INIT_PROPERTY_NUM(shiftX);
    INIT_PROPERTY_NUM(shiftY);
    INIT_PROPERTY_NUM(shiftZ);
    INIT_PROPERTY_BOOL(useVSTMaps);

	// temporary color camera calibration overrides.
    INIT_PROPERTY_NUM(leftColorTilt);
    INIT_PROPERTY_NUM(leftColorPan);
    INIT_PROPERTY_NUM(leftColorRoll);

    INIT_PROPERTY_NUM(rightColorTilt);
    INIT_PROPERTY_NUM(rightColorPan);
    INIT_PROPERTY_NUM(rightColorRoll);

    INIT_PROPERTY_NUM(partialFrameFillPercentage);

    /*
     * Get the camera name as a system property
     */
    const char propName[PROP_NAME_MAX] = "persist.sxr.camname";
    char propValue[PROP_VALUE_MAX];
    const char* defaultCamName = doColorCamera ?
        QVRSERVICE_CAMERA_NAME_RGB : QVRSERVICE_CAMERA_NAME_TRACKING;
    GetProperty(propName, propValue, sizeof(propValue), defaultCamName);

    /*
     * Allow passing of optional arguments as part of the camera name, e.g.
     *   camname:key1=val1,key2=val2
     */
    std::vector<std::string> tmp = StrSplit(std::string(propValue), ':');
    m_cameraName = strdup(tmp[0].c_str());
    if (tmp.size() > 1)
    {
        std::string args = tmp[1];
        std::map<std::string,std::string> params = GetKeyValuePairs(args, ',');
        for (const auto& param : params)
        {
            // check for optional arguments
            if (param.first == "res")
            {
                if (param.second == QVR_CAMDEVICE_RESOLUTION_MODE_FULL ||
                    param.second == QVR_CAMDEVICE_RESOLUTION_MODE_QUARTER)
                {
                    resSel = param.second;
                }
            }
        }
    }
}

/**********************************************************
 * GetProperty
 **********************************************************/
void SvrCameraManager::GetProperty(const char *name, char *value, size_t len, const char *default_value)
{
    if (0 == __system_property_get(name, value))
    {
        strncpy(value, default_value, len);
    }
    else
    {
        if (0 == strlen(value))
        {
            strncpy(value, default_value, len);
        }
    }
}

/**********************************************************
 * Shutdown -  Call this when VR mode stops
 **********************************************************/
void SvrCameraManager::Shutdown()
{
    if (m_syncCtrl != NULL)
    {
        QVRCameraDevice_ReleaseSyncCtrl(m_cameraDevice, m_syncCtrl);
        m_syncCtrl = NULL;
    }

    if (m_cameraDevice != NULL)
    {
        if (m_cameraStarted == true)
        {
            (void) QVRCameraDevice_Stop(m_cameraDevice);
            m_cameraStarted = false;
        }
        QVRCameraDevice_DetachCamera(m_cameraDevice);
        m_cameraDevice = NULL;
    }

    if (m_cameraClient != NULL)
    {
        QVRCameraClient_Destroy(m_cameraClient);
        m_cameraClient = NULL;
    }

    if (useHardwareBuffers == false && m_numCameraTextures != 0)
    {
        GL(glDeleteTextures(1, m_cameraTexture));
        m_numCameraTextures = 0;
    }

    m_frameNumber = -1;

    if (m_pCameraBuffer != NULL)
    {
        free(m_pCameraBuffer);
        m_pCameraBuffer = NULL;
    }

    if (m_cameraName != NULL)
    {
        free(m_cameraName);
        m_cameraName = NULL;
    }

    m_numTexturesPerFrame = 0;


    m_cameraShader.Destroy();

    for (int i=0; i < 2; i++)
    {
        m_eyeTarget[i].Destroy();
        if (m_cameraModel[i] != NULL)
        {
            m_cameraModel[i]->Destroy();
            m_cameraModel[i] = NULL;
        }
    }
}

/**********************************************************
 * GetCameraDevice - Creates the camera device
 **********************************************************/
int SvrCameraManager::CreateCameraDevice()
{
    int ret;

    if (m_cameraDevice != NULL)
    {
        if (m_syncCtrl != NULL)
        {
            QVRCameraDevice_ReleaseSyncCtrl(m_cameraDevice, m_syncCtrl);
            m_syncCtrl = NULL;
        }
        QVRCameraDevice_DetachCamera(m_cameraDevice);
        m_cameraDevice = NULL;
    }

    m_frameFormat = QVRCAMERA_FRAME_FORMAT_UNKNOWN;

    do
    {
        char format_str[16];
        uint32_t len;

        LOGI("svrCameraManager: UseRawFormat %d\n", useRawFormat);

        if (useRawFormat == true)
        {
            qvr_plugin_param_t params[2];
            params[0].name = QVR_CAMCLIENT_ATTACH_STRING_PREF_FORMAT;

            /*
             * First we'll try requesting just "raw" mode. Since this call is asking for
             * a "preferred" format, it doesn't necessarily fail if the format requested
             * is not supported, and on older devices the only way to tell if it worked
             * is to call GetFrame() and check the format of the frame. On newer devices,
             * we can query the raw frame format to check if it really worked.
             */
            params[0].val = QVR_CAMDEVICE_FORMAT_RAW;
            if (doPartialFrame) {
                params[1].name = QVR_CAMCLIENT_ATTACH_STRING_FRAME_SYNC;
                params[1].val = QVR_CAMDEVICE_FRAME_SYNC_PARTIAL;
            }
            m_cameraDevice = QVRCameraClient_AttachCameraWithParams(m_cameraClient,
                                                                    m_cameraName,
                                                                    params,
																	doPartialFrame?2:1);

            if (m_cameraDevice == NULL)
            {
                /*
                 * Partial Frame Delivery not supported
                 */
                m_cameraDevice = QVRCameraClient_AttachCameraWithParams(m_cameraClient,
                                                                        m_cameraName,
                                                                        params,
                                                                        1);

                doPartialFrame = false;
            }

            if (m_cameraDevice != NULL)
            {
                len = sizeof(format_str);
                ret = QVRCameraDevice_GetParam(m_cameraDevice,
                                               QVR_CAMDEVICE_STRING_RDI_FRAME_FORMAT,
                                               &len,
                                               format_str);
                if (0 == ret)
                {
                    LOGI("svrCameraManager: attached using 'raw' format");
                    m_frameFormat = CamFrameFormatFromString(format_str);
                    break;
                }

                /*
                 * Asking for the frame format failed, so it's likely we're running on
                 * an older device. Destroy this device so we attempt to create it with
                 * an explicit format.
                 */
                QVRCameraDevice_DetachCamera(m_cameraDevice);
                m_cameraDevice = NULL;
            }

            /*
             * try raw10
             */
            params[0].val = QVR_CAMDEVICE_FORMAT_RAW10;
            m_cameraDevice = QVRCameraClient_AttachCameraWithParams(m_cameraClient,
                                                                    m_cameraName,
                                                                    params,
																	doPartialFrame?2:1);
            if (m_cameraDevice != NULL)
            {
                LOGI("svrCameraManager: attached using 'raw10' format");
                m_frameFormat = QVRCAMERA_FRAME_FORMAT_RAW10_MONO;
                break;
            }

            /*
             * try raw16
             */
            params[0].val = QVR_CAMDEVICE_FORMAT_RAW16;
            m_cameraDevice = QVRCameraClient_AttachCameraWithParams(m_cameraClient,
                                                                    m_cameraName,
                                                                    params,
																	doPartialFrame?2:1);
            if (m_cameraDevice != NULL)
            {
                LOGI("svrCameraManager: attached using 'raw16' format");
                m_frameFormat = QVRCAMERA_FRAME_FORMAT_RAW16_MONO;
                break;
            }
        }

        /*
         * If the resolution is specified, try attaching to it
         */
        if (!resSel.empty())
        {
            qvr_plugin_param_t params[2];
            params[0].name = QVR_CAMCLIENT_ATTACH_STRING_RES_MODE;
            params[0].val = resSel.c_str();
            if (doPartialFrame) {
                params[1].name = QVR_CAMCLIENT_ATTACH_STRING_FRAME_SYNC;
                params[1].val = QVR_CAMDEVICE_FRAME_SYNC_PARTIAL;
            }
            m_cameraDevice = QVRCameraClient_AttachCameraWithParams(m_cameraClient,
                                                                    m_cameraName,
                                                                    params,
																	doPartialFrame?2:1);
            if (m_cameraDevice != NULL)
            {
                LOGI("svrCameraManager: attached using '%s' resolution", resSel.c_str());
            }
        }

        /*
         * We don't have a camera yet, so try AttachCamera().
         */
        if (m_cameraDevice == NULL)
        {
        	if (doPartialFrame) {
                qvr_plugin_param_t params[1];
                params[0].name = QVR_CAMCLIENT_ATTACH_STRING_FRAME_SYNC;
                params[0].val = QVR_CAMDEVICE_FRAME_SYNC_PARTIAL;
                m_cameraDevice = QVRCameraClient_AttachCameraWithParams(m_cameraClient,
                                                                        m_cameraName,
                                                                        params,
    																	1);

        	} else {
                m_cameraDevice = QVRCameraClient_AttachCamera(m_cameraClient, m_cameraName);
        	}
            if (m_cameraDevice == NULL)
            {

                LOGI("svrCameraManager: CreateCameraDevice Failing to create a camera device.\n");
                return -1;
            }
        }

        /*
         * Get the frame format. This API may not be supported on older devices, so if
         * it fails we just assume y8.
         */
        len = sizeof(format_str);
        int ret = QVRCameraDevice_GetParam(m_cameraDevice,
                                           QVR_CAMDEVICE_STRING_FRAME_FORMAT,
                                           &len,
                                           format_str);
        if (0 == ret)
        {
            LOGI("svrCameraManager: attached using default");
            m_frameFormat = CamFrameFormatFromString(format_str);
            break;
        }

        LOGI("svrCameraManager: couldn't query frame format, so assuming y8.");
        m_frameFormat = QVRCAMERA_FRAME_FORMAT_Y8;
        break;

    } while (0);

    LOGI("svrCameraManager: Frame format is '%s'", CamFrameFormatToString(m_frameFormat));

    /*
     * Try to enable sync
     */
    m_syncCtrl = QVRCameraDevice_GetSyncCtrl(m_cameraDevice, QVR_SYNC_SOURCE_CAMERA_FRAME_CLIENT_READ);
    if (m_syncCtrl != NULL)
    {
        LOGI("svrCameraManager: Camera sync: enabled\n");
    }
    else
    {
        LOGI("svrCameraManager: Camera sync: disabled\n");
    }
    if(doPartialFrame)
    {
        /*
        * Init partial frame structs
        */
        m_pfi = {
            .type = XR_TYPE_QTI_CAM_PARTIAL_FRAME_REQ_INFO_INPUT,
            .next = NULL,
            .fillPercentage = 0,
        };
        m_gfi = {
            .type = XR_TYPE_QTI_CAM_FRAME_REQ_INFO_INPUT,
            .next = (XrBaseStructQTI *)&m_pfi,
            .frameNum = 0,
            .blockMode = XR_CAMERA_BLOCK_MODE_QTI_NON_BLOCKING,
            .dropMode = XR_CAMERA_DROP_MODE_QTI_NEWER_IF_AVAILABLE,
        };

        for (int i = 0; i < ARRAY_SIZE(m_hwBufs); ++i)
        {
            m_hwBufs[i].type = XR_TYPE_QTI_CAM_HW_BUFFER_OUTPUT;
            m_hwBufs[i].next = NULL;
            m_hwBufs[i].bufVAddr = NULL;
            m_hwBufs[i].buf = NULL;
            m_hwBufs[i].flags = 0;
            m_hwBufs[i].offset = {.x = 0, .y = 0};
        }

        m_pfo = {
            .type = XR_TYPE_QTI_CAM_PARTIAL_FRAME_REQ_INFO_OUTPUT,
            .next = NULL,
        };
        m_gfo = {
            .type = XR_TYPE_QTI_CAM_FRAME_REQ_INFO_OUTPUT,
            .next = (XrBaseStructQTI *)&m_pfo,
            .frameInfo = {
                .type = XR_TYPE_QTI_CAM_FRAME_INFO_OUTPUT,
                .next = NULL},
            .bufferInfo = {.type = XR_TYPE_QTI_CAM_FRAME_BUFFER_INFO_OUTPUT, .next = NULL, .bufferCapacityIn = 0, .buffers = NULL},
            .hwBufferInfo = {.type = XR_TYPE_QTI_CAM_HW_FRAME_BUFFER_INFO_OUTPUT, .next = NULL, .hwBufferCapacityIn = ARRAY_SIZE(m_hwBufs), .hwBuffers = m_hwBufs}

        };
    }
    return 0;
}

/**********************************************************
 * SetDisplayProperties
 **********************************************************/
int SvrCameraManager::SetDisplayProperties(sxrDeviceInfo* di)
{

    m_leftDisplay = {
        .intrinsics = {
            .size= { 
                .width = di->targetEyeWidthPixels,
                .height = di->targetEyeHeightPixels,
            },
            .principal_point = {
                .x = di->targetEyeWidthPixels/2.0,
                .y = di->targetEyeHeightPixels/2.0,
            },
            .focal_length = {
                .x = (di->targetEyeWidthPixels/2.0)/tan(di->targetFovXRad/2.0),
                .y = (di->targetEyeHeightPixels/2.0)/tan(di->targetFovYRad/2.0),
            },
            .model = LINEAR
        },
        .extrinsic = {
            .position = {
                .x = di->leftEyeFrustum.position.x+gSensorHeadOffsetX, 
                .y = di->leftEyeFrustum.position.y+gSensorHeadOffsetY, 
                .z = di->leftEyeFrustum.position.z+gSensorHeadOffsetZ,
            },
            .orientation = {
                .w = 1.0,
                .x = 0.0,
                .y = 0.0,
                .z = 0.0,
            },
        }
    };

    m_rightDisplay = {
        .intrinsics = {
            .size= { 
                .width = di->targetEyeWidthPixels,
                .height = di->targetEyeHeightPixels,
            },
            .principal_point = {
                .x = di->targetEyeWidthPixels/2.0,
                .y = di->targetEyeHeightPixels/2.0,
            },
            .focal_length = {
                .x = (di->targetEyeWidthPixels/2.0)/tan(di->targetFovXRad/2.0),
                .y = (di->targetEyeHeightPixels/2.0)/tan(di->targetFovYRad/2.0),
            },
            .model = LINEAR
        },
        .extrinsic = {
            .position = {
                .x = di->rightEyeFrustum.position.x+gSensorHeadOffsetX, 
                .y = di->rightEyeFrustum.position.y+gSensorHeadOffsetY, 
                .z = di->rightEyeFrustum.position.z+gSensorHeadOffsetZ 
            },
            .orientation = {
                .w = 1.0,
                .x = 0.0,
                .y = 0.0,
                .z = 0.0,
            },
        }
    };
    //0 is left
    float width = static_cast<float>(di->targetEyeWidthPixels);
    float height = static_cast<float>(di->targetEyeHeightPixels);

    m_displayInfo[0] = {
        .m_id = 0,
        .m_width = width,
        .m_height =  height,
        .m_centerX = width/2.0f,
        .m_centerY = height/2.0f,
        .m_focalX = static_cast<float>((width/2.0f)/tan(di->targetFovXRad/2.0)),
        .m_focalY = static_cast<float>((height/2.0f)/tan(di->targetFovYRad/2.0)) 
    };
    m_displayInfo[1] = {
        .m_id = 1,
        .m_width = width,
        .m_height = height,
        .m_centerX =width/2.0f,
        .m_centerY = height/2.0f,
        .m_focalX = static_cast<float>((width/2.0)/tan(di->targetFovXRad/2.0)),
        .m_focalY = static_cast<float>((height/2.0)/tan(di->targetFovYRad/2.0))
    };
    

    return 0;
}

/**********************************************************
 * GetCameraCalibInfo - Ask QVRService for the camera calib info.
 **********************************************************/
int SvrCameraManager::GetCameraCalibInfo(qvrcamera_device_helper_t* pCamDevice, SvrCameraInfo *pCamera)
{
    int result;
    XrCameraDevicePropertiesQTI cameraPropertyList;

    /*
     * Set up sensor orientation
     */
    SetSensorOrientation();

    /*
     * Get properties of all cameras.
     */
    result = QVRCameraDevice_GetProperties(pCamDevice, &cameraPropertyList);
    if (result < 0)
    {
        LOGE("svrCameraManager: Error: failed to get camera properties: %d\n", result);
        return -1;
    }

    /*
     * Sanity check camera device STRUCTURE type
     */
    if(cameraPropertyList.base.type != XR_TYPE_QTI_CAM_DEVICE_PROPS)
    {
        LOGE("svrCameraManager: Error: Camera device props wrong struct type: 0x%x\n", cameraPropertyList.base.type);
        return -1;
    }

    /*
     * Sanity check Camera device hardware TYPE itself (hell if I know)
     */
    if (cameraPropertyList.base.deviceType != XR_HW_DEVICE_TYPE_QTI_CAMERA)
    {
        LOGE("svrCameraManager: error: camera sensor props wrong device type: 0x%x\n", cameraPropertyList.base.deviceType);
        return -1;
    }

    /*
     * Only handle stereo for now
     */
    if (cameraPropertyList.base.componentCount != 2)
    {
        LOGE("svrCameraManager: Error: number of camera components is %d, not 2\n", cameraPropertyList.base.componentCount);
        return -1;
    }

    XrHardwareComponentBaseQTI * pComponent = cameraPropertyList.base.components;

    for (int i=0; i < cameraPropertyList.base.componentCount &&  pComponent != NULL ; i++)
    {
        if (pComponent->type != XR_TYPE_QTI_CAM_SENSOR_PROPS)
        {
            LOGE("svrCameraManager: Error: Camera component is not a sensor structure type: %d\n", pComponent->type);
            return -1;
        }

        if (pComponent->componentType != XR_HW_COMP_TYPE_QTI_CAMERA_SENSOR)
        {
            LOGE("svrCameraManager: Error: Camera component type is not of type sensor: %d\n", pComponent->componentType);
            return -1;
        }

        XrCameraSensorPropertiesQTI * pSensor = (XrCameraSensorPropertiesQTI*) pComponent;

        /*
         * Intrinsics
         */
        pCamera[i].m_id = i;
        pCamera[i].m_width = pSensor->calibrationInfo.intrinsics.size.width;
        pCamera[i].m_height = pSensor->calibrationInfo.intrinsics.size.height;
        pCamera[i].m_centerX = pSensor->calibrationInfo.intrinsics.principalPoint.x;
        pCamera[i].m_centerY = pSensor->calibrationInfo.intrinsics.principalPoint.y;
        pCamera[i].m_focalX = pSensor->calibrationInfo.intrinsics.focalLength.x;
        pCamera[i].m_focalY = pSensor->calibrationInfo.intrinsics.focalLength.y;
        pCamera[i].m_radialDisortion[0] = pSensor->calibrationInfo.intrinsics.radialDistortion[0];
        pCamera[i].m_radialDisortion[1] = pSensor->calibrationInfo.intrinsics.radialDistortion[1];
        pCamera[i].m_radialDisortion[2] = pSensor->calibrationInfo.intrinsics.radialDistortion[2];
        pCamera[i].m_radialDisortion[3] = pSensor->calibrationInfo.intrinsics.radialDistortion[3];
        pCamera[i].m_radialDisortion[4] = pSensor->calibrationInfo.intrinsics.radialDistortion[4];
        pCamera[i].m_radialDisortion[5] = pSensor->calibrationInfo.intrinsics.radialDistortion[5];
        pCamera[i].m_radialDisortion[6] = pSensor->calibrationInfo.intrinsics.tangentialDistortion[0];
        pCamera[i].m_radialDisortion[7] = pSensor->calibrationInfo.intrinsics.tangentialDistortion[1];

        /*
         * Extrinsics
         */
        XrPosedQTI extrinsic = pComponent->extrinsic;

        /*
         * If the extrinsic is not valid, it is likely still in 6DOF coordinate system instead of
         * world coordinate system, so convert it to be Y-Up Right Handed.
         */
        if ((pComponent->flags & XR_HARDWARE_COMPONENT_EXTRINSIC_VALID_BIT) == 0)
        {
            LOGE("svrCameraManager: converting extrinsic\n");
            Convert6DOFtoQVR(&pComponent->extrinsic, &extrinsic);
        }

        /*
         * Convert quarternion to Euler rotations
         */
        glm::highp_dquat rotationQuat;
        rotationQuat.x = extrinsic.orientation.x;
        rotationQuat.y = extrinsic.orientation.y;
        rotationQuat.z = extrinsic.orientation.z;
        rotationQuat.w = extrinsic.orientation.w;

        pCamera[i].m_position = glm::vec3(extrinsic.position.x, extrinsic.position.y, extrinsic.position.z);
        pCamera[i].m_rotation = glm::eulerAngles(rotationQuat);

        m_cameraImageWidth = pSensor->calibrationInfo.intrinsics.size.width;
        m_cameraImageHeight = pSensor->calibrationInfo.intrinsics.size.height;

        pComponent = (XrHardwareComponentBaseQTI*) pComponent->next;
    }

    return 0;
}


typedef void(GL_APIENTRYP PFNEGLIMAGETARGETTEXTURESTORAGEEXTPROC)(GLuint texture, GLeglImageOES image, const GLint* attrib_list);

typedef void(EGLAPIENTRYP PFNEGLIMAGETARGETTEXSTORAGEEXTPROC)(GLenum target, GLeglImageOES image, const int * attrib_list);


/**********************************************************
 * AddFrame - Return value is number of textures. The
 * texure IDs are returned via the pTextureID parameter.
 **********************************************************/
int SvrCameraManager::AddFrame(qvrcamera_device_helper_t * pCameraDevice,
                               qvrcamera_frame_t cameraFrame,
                               unsigned int * pTextureID)
{

    /*
     * Get camera image via Android Hardware Buffer
     */
    int result =-1;
    uint32_t numHardwareBuffers = -1;
    qvrcamera_hwbuffer_t *pHardwareBuffers = NULL;
    if(doPartialFrame)
    {
        numHardwareBuffers = m_gfo.hwBufferInfo.hwBufferCount; //setting this for PF. Is there a way to query this?
        result = QVR_CAM_SUCCESS; //ensure compatibility with legacy
    }
    else
    {
        numHardwareBuffers = 0;
        result = QVRCameraDevice_FrameToHardwareBuffer(pCameraDevice, &cameraFrame, &numHardwareBuffers, &pHardwareBuffers);
    }
    if (result != QVR_CAM_SUCCESS)
    {
        LOGE("svrCameraManager: Unable to get image from Hardware Buffer.\n");
        return 0;
    }


    /*
     * Sanity checks
     */
    if (numHardwareBuffers == 0)
    {
        LOGE("svrCameraManager: FrameToHardwareBuffer() returned no HardwareBuffers.\n");
        return 0;
    }

    if (numHardwareBuffers > MAX_IMAGES_PER_FRAME)
    {
        LOGE("svrCameraManager: FrameToHardwareBuffer() returned too many HardwareBuffers.\n");
        return 0;
    }
    if(!doPartialFrame)
    {
        if (NULL == pHardwareBuffers || NULL == pHardwareBuffers[0].buf)
        {
            LOGE("svrCameraManager: pHardwareBuffers is NULL, how can this be?\n");
            return 0;
        }
    }

    /*
     * Each hardware buffer is represented by an OpenGL texture.
     * Remember how many textures per camera frame. This can be
     * either 1 or 2. 1 texture means the 2 camera images are side
     * by side in a single texture image. 2 means there are 2 separate
     * textures, one camera image per texture.
     */
    if (m_numTexturesPerFrame == 0)
    {
        m_numTexturesPerFrame = numHardwareBuffers;
    }

    if (getNativeClientBuffer == (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)NULL)
    {
        getNativeClientBuffer = (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)eglGetProcAddress("eglGetNativeClientBufferANDROID");
    }

    for (int i=0; i < numHardwareBuffers; i++)
    {
        EGLint EglAttribs[] = {
            EGL_NONE
        };

        /*
         * Create EGL image for each hardware buffer
         */
        EGLClientBuffer clientBuffer = 0;
        if(doPartialFrame)
            clientBuffer = getNativeClientBuffer(m_gfo.hwBufferInfo.hwBuffers[i].buf);
        else
            clientBuffer = getNativeClientBuffer(pHardwareBuffers[i].buf);
        EGLint errorCode = eglGetError();
        if (errorCode != EGL_SUCCESS)
        {
            LOGE("svrCameraManager: getNativeClientBuffer() failed.\n");
            return 0;
        }

        /*
         * Create the EGL image.
         */
        EGLImageKHR eglImage = eglCreateImageKHR(m_eglDisplay,
                                                 EGL_NO_CONTEXT,
                                                 EGL_NATIVE_BUFFER_ANDROID,
                                                 clientBuffer,
                                                 EglAttribs);

        errorCode = eglGetError();
        if (errorCode != EGL_SUCCESS)
        {
            LOGE("svrCameraManager: eglCreateImageKHR() failed.\n");
            return 0;
        }

        float borderColor[] = { 0.0, 0.0, 0.0, 1.0 };
        /*
         * Create the OpenGL texture
         */
        glGenTextures(1, &m_rawTexture[i]);
        GL(glActiveTexture(GL_TEXTURE0));

        /*
         * Set up the OpenGL texture from the eglImage
         */
        if (m_frameFormat != QVRCAMERA_FRAME_FORMAT_RAW16_MONO)
        {
            /*
             * Doing 10 bit raw (probably)
             */
            GL(glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_rawTexture[i]));

            glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglImage);
            if (glGetError() != GL_NO_ERROR)
            {
                LOGE("svrCameraManager: glEGLImageTargetTexture2DOES() failed.\n");
                return 0;
            }

            GL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            if(m_frameFormat==QVRCAMERA_FRAME_FORMAT_YUV420)
            {
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else
            {
                glTexParameterfv(GL_TEXTURE_EXTERNAL_OES, TEXTURE_BORDER_COLOR, &borderColor[0]);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            }

            pTextureID[i] = m_rawTexture[i];
        }
        else
        {
            /*
             * Using 16 bit RAW format.
             */
            GL(glBindTexture(GL_TEXTURE_2D, m_rawTexture[i]));
            if (doR16float == true)
            {
                bool success = true;
                eglImageTargetTexStorageEXT(GL_TEXTURE_2D, eglImage, NULL);
                EGLint errorCode = eglGetError();
                if (errorCode != EGL_SUCCESS)
                {
                    success = false;
                }

                /*
                 * The view texture interprets the 16 bit textures as if it was 16 bit float.
                 * The float format allows for GL_LINEAR texture filtering. The int format does not.
                 */
                if (success == true)
                {
                    glGenTextures(1, &m_viewTexture[i]);
                    glTextureViewOES(m_viewTexture[i],
                                     GL_TEXTURE_2D,
                                     m_rawTexture[i],
                                     GL_R16_EXT,
                                     0,
                                     1,
                                     0,
                                     1);

                    errorCode = glGetError();
                    if (errorCode != GL_NO_ERROR)
                    {
                        success = false;
                    }
                }

                /*
                 * Set up the view texture
                 */
                if (success == true)
                {
                    pTextureID[i] = m_viewTexture[i];
                    glActiveTexture(GL_TEXTURE0);
                    GL(glBindTexture(GL_TEXTURE_2D, m_viewTexture[i]));

                    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                    glTexParameterfv(GL_TEXTURE_2D, TEXTURE_BORDER_COLOR, &borderColor[0]);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);


                }
                else
                {
                    /*
                     * 16 bit FLOAT setup failed, so set up as INTEGER texture, must be GL_NEAREST filtering.
                     */
                    doR16float = false; // for future frames, avoid this code path.
                    GL(glDeleteTextures(1, (const GLuint*) &m_viewTexture[i]));

                    InitializeShaderFromBuffer(m_cameraShader, camTextureVertex, camTextureFragment2D_mono_16_integer , "Vs", "Fs");
                    GL(glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage));
                    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
                    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

                    GL(glBindTexture(GL_TEXTURE_2D, m_rawTexture[i]));
                    pTextureID[i] = m_rawTexture[i];
                    LOGE("svrCameraManager: 16 bit float texture setup failed, switching to 16 bit integer texture mode.\n");
                }
            }
            else
            {
                /*
                 * doR16foat was false, so use 16 bit int texture, must be GL_NEAREST filtering.
                 */
                GL(glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
                pTextureID[i] = m_rawTexture[i];
            }
        } // 16 bit

        if (eglImage!= EGL_NO_IMAGE_KHR)
        {
            EGLBoolean status = eglDestroyImageKHR(m_eglDisplay, eglImage);
            eglImage = EGL_NO_IMAGE_KHR;
            if (status != EGL_TRUE)
            {
                LOGE("svrCameraManager: destroyImage failed: %d)\n", status);
            }
        }
    }

    /*
     * Rreturn the number of hardware buffers
     */
    result = numHardwareBuffers;

    return result;
}

/**********************************************************
 * GetCorrectedTexturedID - OpenGL handle to camera image
 * that has been unwarped.
 **********************************************************/
int32_t SvrCameraManager::GetCorrectedTextureID(sxrEyeMask eyeMask)
{
    int index = 0;

    if (eyeMask == kEyeMaskRight)
    {
        index = 1;
    }

    int32_t textureID =  m_eyeTarget[index].GetColorAttachment();
    return textureID;
}


/**********************************************************
 * GetUncorrectedTexturedID - OpenGL handle to camera image
 * without any lens correction applied.
 **********************************************************/
int32_t SvrCameraManager::GetUncorrectedTextureID(sxrEyeMask eyeMask)
{
    int32_t textureID = 0;

    if (m_numCameraTextures == 2)
    {
        /*
         * There are 2 textures
         */
        int index = 0;
        if (eyeMask == kEyeMaskRight)
        {
            index = 1;
        }

        textureID = m_cameraTexture[index];

    }
    else if (m_numCameraTextures == 1)
    {
        /*
         * If only one stereo pair texture, return its ID
         */
        textureID =  m_cameraTexture[0];
    }
    else
    {
        LOGE("svrCameraManager: ERROR - Unexpected number of camera textures.\n");
    }

    return textureID;
}


/**********************************************************
 * InitializeShaderFromBuffer
 **********************************************************/
void SvrCameraManager::InitializeShaderFromBuffer(Svr::SvrShader &whichShader,
                                                  char *pVertexSource,
                                                  char *pFragmentSource,
                                                  const char *vertexName,
                                                  const char *fragmentName)
{
    int numVertStrings = 0;
    int numFragStrings = 0;
    const char *vertShaderStrings[16];
    const char *fragShaderStrings[16];

    vertShaderStrings[numVertStrings++] = pVertexSource;
    fragShaderStrings[numFragStrings++] = pFragmentSource;
    bool success = whichShader.Initialize(numVertStrings, vertShaderStrings, numFragStrings, fragShaderStrings,
                                          vertexName, fragmentName);
    if (success == false)
    {
        LOGE("svrCameraManager: Shader failed to build. FRAG %s", pFragmentSource);
    }
}


/**********************************************************
 * GetPoseState
 **********************************************************/
sxrHeadPoseState SvrCameraManager::GetPoseState()
{
    return m_poseState;
}

/**********************************************************
 * GetPassCount
 **********************************************************/
bool SvrCameraManager::IsSinglePass()
{
    bool result = !doTwoPassRender;
    return result;
}

/**********************************************************
 * IsOK
 **********************************************************/
bool SvrCameraManager::IsOK()
{
    return m_okToRender;
}
/**********************************************************
 * Get PFD Support info
 **********************************************************/
bool SvrCameraManager::IsPFDSupported()
{
    return m_pfdSupport;
}
/**********************************************************
 * camTextureFragmentEXT_mono - Fragment shader for 10-bit and
 * 8-bit monochrome textures.
 **********************************************************/
char camTextureFragmentEXT_mono [] =
"#version 320 es\n"
"#extension GL_OES_EGL_image_external_essl3 : require\n"
"precision highp float;\n"
"precision highp samplerExternalOES;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform highp samplerExternalOES srcTex;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    vec4 srcColor;\n"
"    srcColor = texture(srcTex, vTexcoord0);\n"
"    outColor      = vec4(srcColor.r, srcColor.r, srcColor.r, 1.); \n"
"}\n"
;

/**********************************************************
 * camTextureFragment2D_mono_16_integer - Fragment shader for RAW16,
 * where the 16 bits are an unsigned integer, GL_NEAREST textures.
 **********************************************************/
char camTextureFragment2D_mono_16_integer [] =
"#version 320 es\n"
"precision highp float;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform highp usampler2D srcTex;\n"
"\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"    float color = float(texture(srcTex, vTexcoord0).x) / 1024.0f;\n"
"    fragColor   = vec4(color, color, color, 1.0f);\n"
"}\n"
;

/**********************************************************
 * camTextureFragment2D_mono_16_float - Fragment shader for RAW16,
 * where the 16 bits are a float, and allow GL_LINEAR textures.
 **********************************************************/
char camTextureFragment2D_mono_16_float [] =
"#version 320 es\n"
"precision highp float;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform highp sampler2D srcTex;\n"
"\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"    float color = texture(srcTex, vTexcoord0).x * 64.0f;\n"
"    fragColor   = vec4(color, color, color, 1.0f);\n"
"}\n"
;

/**********************************************************
 * camTextureFragmentEXT_color  - Fragment shader for color
 * textures.
 **********************************************************/
char camTextureFragmentEXT_color [] =
"#version 320 es\n"
"#extension GL_OES_EGL_image_external_essl3 : require\n"
"precision highp float;\n"
"precision highp samplerExternalOES;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform highp samplerExternalOES srcTex;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    vec4 srcColor;\n"
"    const float border_width = 0.0001;\n"
"                                       \n"
"    if(vTexcoord0.y > border_width && vTexcoord0.y<(1.0-border_width)){\n"
"        srcColor = texture(srcTex, vTexcoord0);\n"
"    }\n"
"    else {\n"
"        srcColor=vec4(0.0,0.0,0.0,1.0);\n"
"    }\n"
"    outColor = vec4(srcColor.r, srcColor.g, srcColor.b, 1.); \n"
"}\n"
;
//PFD support enabled
char camTextureFragmentEXT_color_swap [] =
"#version 320 es\n"
"#extension GL_OES_EGL_image_external_essl3 : require\n"
"precision highp float;\n"
"precision highp samplerExternalOES;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform highp samplerExternalOES srcTex;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    vec4 srcColor;\n"
"    const float border_width = 0.0001;\n"
"                                       \n"
"    if(vTexcoord0.y > border_width && vTexcoord0.y<(1.0-border_width)){\n"
"        srcColor = texture(srcTex, vTexcoord0);\n"
"    }\n"
"    else {\n"
"        srcColor=vec4(0.0,0.0,0.0,1.0);\n"
"    }\n"
"    outColor = vec4(srcColor.b, srcColor.g, srcColor.r, 1.); \n"
"}\n"
;


/**********************************************************
 * camTextureFragment2D_simple - Fragment shader for simple texture lookup
 **********************************************************/
char camTextureFragment2D_simple [] =
"#version 320 es\n"
"precision highp float;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform sampler2D srcTex;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    vec4 srcColor = texture(srcTex, vTexcoord0);\n"
"    outColor      = srcColor;\n"
"}\n"
;

/**********************************************************
 * camTextureFragmentUV - Fragment shader for rendering UV map as color.
 **********************************************************/
char camTextureFragmentUV [] =
"#version 320 es\n"
"#extension GL_OES_EGL_image_external_essl3 : require\n"
"precision highp float;\n"
"precision highp samplerExternalOES;\n"
"in vec2 vTexcoord0;\n"
"in vec4 vWorldPos;\n"
"\n"
"uniform highp samplerExternalOES srcTex;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    outColor =  vec4(vTexcoord0.s, vTexcoord0.t, vWorldPos.x/vWorldPos.w, vWorldPos.y/vWorldPos.w);\n"
"}\n"
;

char camTextureVertex [] =
"#version 320 es\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"in vec2 texcoord0;\n"
"\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 modelMatrix;\n"
"\n"
"out vec4 vWorldPos;\n"
"out vec3 vWorldNormal;\n"
"out vec2 vTexcoord0;\n"
"\n"
"void main()\n"
"{\n"
"  gl_PointSize = 2.f;"
"    gl_Position = projectionMatrix * (viewMatrix * (modelMatrix * vec4(position, 1.0)));\n"
"    vWorldPos = gl_Position;\n"
"    // Only rotate the rest of these!\n"
"    vWorldNormal = (modelMatrix * vec4(normal.xyz, 0.0)).xyz;\n"
"\n"
"    vTexcoord0 = texcoord0;\n"
"}\n"
;

/**********************************************************
 * displayMeshVertex - Vertex shader for 1-pass mesh creation
 *      position  => Position
 *      normal    => TexCoordR
 *      color     => TexCoordG
 *      texcoord0 => TexCoordB
 **********************************************************/
char displayMeshVertex[] =
"#version 320 es\n"
"in vec4 position;      // Position\n"
"in vec4 normal;        // Red\n"
"in vec4 color;         // Green\n"
"in vec4 texcoord0;     // Blue\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 modelMatrix;\n"
"uniform uint channel;\n"
"uniform uint showXYZ;\n"
"out vec4 vWorldPos;\n"
"out vec2 vTexcoord0;\n"
"void main()\n"
"{\n"
"    gl_Position = projectionMatrix * (viewMatrix * (modelMatrix * position));\n"
"//    gl_Position.x = (gl_Position.x * 2.0) + 1.0; \n"
"//    gl_Position = position; \n"
"//    gl_Position.x = (gl_Position.x * 2.0) + 1.0; \n"
"    vWorldPos = gl_Position;\n"    
"    if (channel == 0u)\n"
"    {\n"
"        vTexcoord0.xy = normal.xy;\n"
"    }\n"
"    else if (channel == 1u)\n"
"    {\n"
"        vTexcoord0.xy = color.xy;\n"
"    }\n"
"    else if (channel == 2u)\n"
"    {\n"
"        vTexcoord0.xy = texcoord0.xy;\n"
"    }\n"
"    else\n"
"    {\n"
"        vTexcoord0.xy = vec2(0., 0.);\n"
"    }\n"
"    \n"
"    vTexcoord0 = (vTexcoord0 * 0.5) + vec2(0.5, 0.5);\n"
"}\n"
;

/**********************************************************
 * displayMeshFragment - used in 1-pass mesh creation
 **********************************************************/
char displayMeshFragment[] =
"#version 320 es\n"
"precision highp float;\n"
"in vec4 vWorldPos;\n"
"in vec2 vTexcoord0;\n"
"\n"
"uniform sampler2D srcTex;\n"
"uniform uint showXYZ;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    vec4 srcColor = texture(srcTex, vTexcoord0);\n"
//"    outColor =  vec4(vTexcoord0.x, vTexcoord0.y, 0., 1.);\n"
"    outColor = srcColor;\n"
"    if (showXYZ == 1u)\n"
"    {\n"
"        outColor =  vec4(vWorldPos.x, vWorldPos.y, 0., 1.);\n"
"    }\n"
"}\n"
;


#endif  // ENABLE_CAMERA


/**********************************************************
 * Cross product of 2 vectors of size 3 (ie, XYZ)
 **********************************************************/
void  CrossProduct (float* a, float* b, float* result)
{
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
}


/**********************************************************
 * MulitplyQuatPoint - Transforma a point by a Quaternion
 **********************************************************/
void  MultiplyQuatPoint(float* quat, float* point, float* result)
{
    float uv[3];
    CrossProduct(quat, point, uv);

    float uuv[3];
    CrossProduct(quat, uv, uuv);

    result[0] = point[0] + ((uv[0] * quat[3]) + uuv[0]) * 2.f;
    result[1] = point[1] + ((uv[1] * quat[3]) + uuv[1]) * 2.f;
    result[2] = point[2] + ((uv[2] * quat[3]) + uuv[2]) * 2.f;
    result[3] = point[3];
}


/**********************************************************
 * Convert6DOFtoQVR - Does 2 things:
 *  1) Convert the rotation to be Y-Up Right handed
 *  2) Applies inverse of the rotation to the given postion to
 *     get the actual camera positions relative to IMU.
 **********************************************************/
void SvrCameraManager::Convert6DOFtoQVR(XrPosedQTI *pPoseIn, XrPosedQTI *pPoseOut)
{
    /*
     *  PART 1 - Convert Rotation
     */
    /*
     * Convert to Euler angles, taking sensor orientation into account
     */
    float vals[3];
    vals[0] = pPoseIn->orientation.x * m_axis_id_signs[0];
    vals[1] = pPoseIn->orientation.y * m_axis_id_signs[1];
    vals[2] = pPoseIn->orientation.z * m_axis_id_signs[2];
    float x = vals[m_axis_ids[0]];
    float y = vals[m_axis_ids[1]];
    float z = vals[m_axis_ids[2]];
    float w = pPoseIn->orientation.w;

    float rotX = atan2(2.f * (y * z + w * x), w * w - x * x - y * y + z * z);
    float rotY = asin(-2.f * (x * z - w * y));
    float rotZ = atan2(2.f * (x * y + w * z), w * w + x * x - y * y - z * z);

    /*
     * Adjust so a camera with no tilt equals zero, instead of -180 degrees.
     */
    rotX += M_PI;

    /*
     * Adjust so that rotations go in the right direction (RHS).
     */
    rotX *= -1.f;

    /*
     * Some calibrations report Z rotated by 90 degrees for some reason
     */
    if (rotZ < -1.f)
    {
        rotZ += M_PI_2;
    }
    else if (rotZ > 1.f)
    {
        rotZ -= M_PI_2;
    }

    /*
     * Convert back to quaternion
     */
    float cosX = cos(rotX * 0.5f);
    float cosY = cos(rotY * 0.5f);
    float cosZ = cos(rotZ * 0.5f);

    float sinX = sin(rotX * 0.5f);
    float sinY = sin(rotY * 0.5f);
    float sinZ = sin(rotZ * 0.5f);


    float quatX = sinX * cosY * cosZ - cosX * sinY * sinZ;
    float quatY = cosX * sinY * cosZ + sinX * cosY * sinZ;
    float quatZ = cosX * cosY * sinZ - sinX * sinY * cosZ;
    float quatW = cosX * cosY * cosZ + sinX * sinY * sinZ;

    /*
     * Set result values
     */
    pPoseOut->orientation.x = quatX;
    pPoseOut->orientation.y = quatY;
    pPoseOut->orientation.z = quatZ;
    pPoseOut->orientation.w = quatW;

    /*
     *  PART 2 - Convert Position
     */
    float conjugateX = -pPoseIn->orientation.x; // calculate conjugate for inverse quaternion
    float conjugateY = -pPoseIn->orientation.y;
    float conjugateZ = -pPoseIn->orientation.z;
    float conjugateW = pPoseIn->orientation.w;

    // dot product of quat with itself
    float x2 = x*x;
    float y2 = y*y;
    float z2 = z*z;
    float w2 = w*w;
    float dotProduct = x2 + y2 + z2 + w2;

    // Make inverse quaternion
    float invQuatX = conjugateX / dotProduct;
    float invQuatY = conjugateY / dotProduct;
    float invQuatZ = conjugateZ / dotProduct;
    float invQuatW = conjugateW / dotProduct;

    /*
     * Put into array for next multiplication step
     */
    float invQuat[4];
    invQuat[0] = invQuatX;
    invQuat[1] = invQuatY;
    invQuat[2] = invQuatZ;
    invQuat[3] = invQuatW;

    /*
     * Position as array
     */
    float point[4];
    point[0] = pPoseIn->position.x;
    point[1] = pPoseIn->position.y;
    point[2] = pPoseIn->position.z;
    point[3] = 1.f;

    /*
     * Transform the point by the inverse quaternion
     */
    float result[4];
    MultiplyQuatPoint(invQuat, point, result);

    /*
     * Set the position results, taking sensor orientation into account
     */
    vals[0] = -result[0] * m_axis_id_signs[0];
    vals[1] = -result[1] * m_axis_id_signs[1];
    vals[2] = -result[2] * m_axis_id_signs[2];
    pPoseOut->position.x = vals[m_axis_ids[0]];
    pPoseOut->position.y = vals[m_axis_ids[1]];
    pPoseOut->position.z = vals[m_axis_ids[2]];
}

/**********************************************************
 * ConvertQVRto6DOF
 * Convert the rotation and position to be in the 6DOF
 * coordinate system, which is Y-down.
 **********************************************************/
void ConvertQVRto6DOF(XrPosedQTI *pPoseIn, XrPosedQTI *pPoseOut)
{
    /*
     * TODO: this function still needs to handle sensor_orientation and
     *       the miscellaneous orientation adjustments.
     */

    /*
     *  PART 1 - Convert Rotation
     */
    /*
     * Convert to Euler angles
     */
    float x = pPoseIn->orientation.x;
    float y = pPoseIn->orientation.y;
    float z = pPoseIn->orientation.z;
    float w = pPoseIn->orientation.w;

    float rotX = atan2(2.f * (y * z + w * x), w * w - x * x - y * y + z * z);
    float rotY = asin(-2.f * (x * z - w * y));
    float rotZ = atan2(2.f * (x * y + w * z), w * w + x * x - y * y - z * z);

    /*
     * Adjust so a camera with no tilt equals +/- 180, the 6DOF convention.
     * We add here instead of subtract only to get the same quaternion
     * signage convention on the XYZ(axis) vs W(angle).  -(XYZ)W == XYZ(-W).
     */
    rotX += M_PI;

    /*
     * Convert back to quaternion
     */
    float cosX = cos(rotX * 0.5f);
    float cosY = cos(rotY * 0.5f);
    float cosZ = cos(rotZ * 0.5f);

    float sinX = sin(rotX * 0.5f);
    float sinY = sin(rotY * 0.5f);
    float sinZ = sin(rotZ * 0.5f);


    float quatX = sinX * cosY * cosZ - cosX * sinY * sinZ;
    float quatY = cosX * sinY * cosZ + sinX * cosY * sinZ;
    float quatZ = cosX * cosY * sinZ - sinX * sinY * cosZ;
    float quatW = cosX * cosY * cosZ + sinX * sinY * sinZ;

    /*
     * Set result values
     */
    pPoseOut->orientation.x = quatX;
    pPoseOut->orientation.y = quatY;
    pPoseOut->orientation.z = quatZ;
    pPoseOut->orientation.w = quatW;

    /*
     *  PART 2 - Convert Position
     */
    float point[4];
    point[0] = pPoseIn->position.x;
    point[1] = pPoseIn->position.y;
    point[2] = pPoseIn->position.z;
    point[3] = 1.f;

    /*
     * Rotation quaternion (6DOF)
     */
    float rotationQuaternion[4];
    rotationQuaternion[0] = quatX;
    rotationQuaternion[1] = quatY;
    rotationQuaternion[2] = quatZ;
    rotationQuaternion[3] = quatW;

    /*
     * Transform position by quaternion
     */
    float result[4];
    MultiplyQuatPoint(rotationQuaternion, point, result);

    /*
     * Set the position results
     */
    pPoseOut->position.x = -result[0];
    pPoseOut->position.y = -result[1];
    pPoseOut->position.z = -result[2];
}

static QVRCAMERA_FRAME_FORMAT CamFrameFormatFromString(const char* str)
{
    const char* table[QVRCAMERA_FRAME_FORMAT_LAST] = {
        "unknown",                            // QVRCAMERA_FRAME_FORMAT_UNKNOWN
        QVR_CAMDEVICE_FRAME_FORMAT_Y8,        // QVRCAMERA_FRAME_FORMAT_Y8
        QVR_CAMDEVICE_FRAME_FORMAT_YUV420,    // QVRCAMERA_FRAME_FORMAT_YUV420
        QVR_CAMDEVICE_FORMAT_RAW10,           // QVRCAMERA_FRAME_FORMAT_RAW10_MONO
        "depth16",                            // QVRCAMERA_FRAME_FORMAT_DEPTH16
        "rawdepth",                           // QVRCAMERA_FRAME_FORMAT_RAW_DEPTH
        QVR_CAMDEVICE_FORMAT_RAW16            // QVRCAMERA_FRAME_FORMAT_RAW16_MONO
    };
    for (uint32_t i=0; i<ARRAY_SIZE(table); i++)
    {
        if (0 == strcmp(table[i], str))
        {
            return (QVRCAMERA_FRAME_FORMAT) i;
        }
    }
    return QVRCAMERA_FRAME_FORMAT_UNKNOWN;
}

static const char* CamFrameFormatToString(QVRCAMERA_FRAME_FORMAT format)
{
    switch (format)
    {
        case QVRCAMERA_FRAME_FORMAT_Y8:         return QVR_CAMDEVICE_FRAME_FORMAT_Y8; break;
        case QVRCAMERA_FRAME_FORMAT_YUV420:     return QVR_CAMDEVICE_FRAME_FORMAT_YUV420; break;
        case QVRCAMERA_FRAME_FORMAT_RAW10_MONO: return QVR_CAMDEVICE_FORMAT_RAW10; break;
        case QVRCAMERA_FRAME_FORMAT_DEPTH16:    return "depth16"; break;
        case QVRCAMERA_FRAME_FORMAT_RAW_DEPTH:  return "rawdepth"; break;
        case QVRCAMERA_FRAME_FORMAT_RAW16_MONO: return QVR_CAMDEVICE_FORMAT_RAW16; break;
        case QVRCAMERA_FRAME_FORMAT_UNKNOWN:
        default:                                return "unknown"; break;
    }
}

/**********************************************************
 * GetSensorOrientationFromDevice
 * Attempts to read the sensor_orientation setting from the
 * device. If that fails, it returns a sensor_orientation
 * based on the device type, which really only works for the
 * XR reference devices we know about.
 **********************************************************/
void SvrCameraManager::GetSensorOrientationFromDevice(std::string &s)
{
    char so[32] = "1 2 3";
    uint32_t len = sizeof(so);

    /*
     * First attempt to read the sensor_orientation parameter from the device.
     */
    if (QVRServiceClient_GetParam(gAppContext->qvrHelper,
                                  QVRSERVICE_SENSORS_ORIENTATION,
                                  &len, so) == 0)
    {
        LOGI("svrCameraManager: device returned sensor_orientation: %s", so);
    }
    else
    {
        LOGI("svrCameraManager: returning default sensor_orientation for this device: %s", so);
    }

    s = std::string(so);
}

/**********************************************************
 * SetSensorOrientation
 * Sets the sensor axis values and signs using the sensor
 * orientation retrieved from the device, or default values
 * if those are invalid.
 **********************************************************/
void SvrCameraManager::SetSensorOrientation()
{
    std::string s;
    std::vector<int> sensor_orientation;

    GetSensorOrientationFromDevice(s);

    std::istringstream iss(s);
    sensor_orientation.clear();
    const int error = -123;
    while (iss.eof() == false)
    {
        int token = error;
        iss >> token;
        if (token != error)
        {
            sensor_orientation.push_back(token);
        }
    }

    /*
     * format of the config is: x y z
     * if we don't have 3 entries, we reject the input
     */
    if (sensor_orientation.size() != 3)
    {
        LOGE("svrCameraManager: sensor orientation: unexpected input size: %d. Using default '1 2 3'.", (int) sensor_orientation.size());
        sensor_orientation.clear();
        sensor_orientation.push_back(1);
        sensor_orientation.push_back(2);
        sensor_orientation.push_back(3);
    }

    m_axis_ids[0] = abs(sensor_orientation[0])-1;
    m_axis_ids[1] = abs(sensor_orientation[1])-1;
    m_axis_ids[2] = abs(sensor_orientation[2])-1;

    m_axis_id_signs[0] = sensor_orientation[0] < 0 ? -1 : 1;
    m_axis_id_signs[1] = sensor_orientation[1] < 0 ? -1 : 1;
    m_axis_id_signs[2] = sensor_orientation[2] < 0 ? -1 : 1;
}

/**********************************************************
 * MakeOnePassMesh
 **********************************************************/
SxrResult SvrCameraManager::MakeOnePassMesh()
{
    SxrResult result = SXR_ERROR_UNKNOWN;
    /*
     * Render the camera UVs for both eyes.
     */
    result = RenderCameraUVs();
    return result;
}


/**********************************************************
 * EndOnePass - Called after usually mulitple calls to 
 * MakeOnePassMesh() to free up buffers, etc.
 **********************************************************/
void SvrCameraManager::EndOnePass()
{
    for (int eyeIndex = 0; eyeIndex < 2; eyeIndex++)
    {
        if (m_cameraUV[eyeIndex] != NULL)
        {
            free(m_cameraUV[eyeIndex]);
            m_cameraUV[eyeIndex] = NULL;
        }
        if(m_pMapU[eyeIndex] !=NULL)
        {
            free(m_pMapU[eyeIndex]);
            m_pMapU[eyeIndex] = NULL;
        }
        if (m_pMapV[eyeIndex] != NULL)
        {
            free(m_pMapV[eyeIndex]);
            m_pMapV[eyeIndex] = NULL;
        }
    }
        GL(glDeleteTextures(1, &m_frameBufferTextureID));
    GL(glDeleteFramebuffers(1, &m_frameBufferID));

    m_doneCameraUV = false;
}

/**********************************************************
 * IsFloat16
 **********************************************************/
QVRCAMERA_FRAME_FORMAT SvrCameraManager::GetCameraFrameFormat()
{
    return m_frameFormat;
}

/**********************************************************
 * IsMonoFloat16
 **********************************************************/
bool SvrCameraManager::IsMonoFloat16()
{
    if (doR16float == true && m_frameFormat == QVRCAMERA_FRAME_FORMAT_RAW16_MONO)
    {
        // 16 bit floating point
        return true;
    }
    else
    {
        // 16 bit integer
        return false;
    }
}

std::vector<std::string> SvrCameraManager::StrSplit(const std::string& s, char sep)
{
    std::vector<std::string> out;
    size_t beg = 0;
    size_t pos;
    while ((pos = s.find(sep, beg)) != std::string::npos)
    {
        std::string sub = s.substr(beg, pos - beg);
        out.push_back(sub);
        beg = pos + 1;
    }
    std::string sub = s.substr(beg);
    if (sub.length() > 0)
    {
        out.push_back(sub);
    }
    return out;
}

std::map<std::string,std::string> SvrCameraManager::GetKeyValuePairs(const std::string& s, char sep)
{
    std::map<std::string,std::string> KVPairs;
    std::vector<std::string> paramList = StrSplit(s, sep);
    for (const auto& param : paramList)
    {
        std::vector<std::string> kv = StrSplit(param, '=');
        switch (kv.size()) {
        case 1:  KVPairs[kv[0]] = "true"; break;
        case 2:  KVPairs[kv[0]] = kv[1];  break;
        default: break;
        }
    }
    return KVPairs;
}
