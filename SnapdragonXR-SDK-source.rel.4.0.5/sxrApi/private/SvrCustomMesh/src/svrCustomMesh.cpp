#define ENABLE_CUSTOM_MESH 1
#ifdef ENABLE_CUSTOM_MESH
//=============================================================================
// FILE: svrCustomMesh.cpp
// Copyright (c) 2020-2021 QUALCOMM Technologies Inc.
// All Rights Reserved.
//==============================================================================

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <android/native_window.h>

#include "svrUtil.h"
#include "svrCustomMesh.h"
#include "svrConfig.h"

#define QVRSERVICE_SDK_LENS_LEFT_FILE                   "sdk-lens-left-file"
#define QVRSERVICE_SDK_LENS_RIGHT_FILE                  "sdk-lens-right-file"

EXTERN_VAR(int, gWarpMeshRows);
extern SvrProgramAttribute gMeshAttribs[NUM_VERT_ATTRIBS];

extern char remeshVertex [];
extern char remeshUVpixel [];
extern char remeshXYZpixel [];

using namespace std;
/**********************************************************
 * Constructor
 **********************************************************/
SvrCustomMesh::SvrCustomMesh()
{
    for (int i=0; i < 3; i++)
    {
        m_pBufferXYZ[i] = NULL;
        m_pBufferUV[i] = NULL;
    }
}

/**********************************************************
 * Destructor
 **********************************************************/
SvrCustomMesh::~SvrCustomMesh()
{
}

/**********************************************************
 * ResampleCustomWarpMesh
 **********************************************************/
SxrResult SvrCustomMesh::ResampleCustomWarpMesh(SvrGeometry& warpGeom,
                                                svrWarpMeshArea whichMesh,
                                                unsigned int numRows,
                                                unsigned int numCols)
{
    /*
     * See if wee need to chop up the mesh into upper/lower sections.
     */
    m_beginRow = 0;
    m_endRow = numRows;

    if (whichMesh == kMeshLL || whichMesh == kMeshLR)
    {
        m_beginRow = 0;
        m_endRow = (numRows / 2);
    }
    if (whichMesh == kMeshUL || whichMesh == kMeshUR)
    {
        m_beginRow = (numRows / 2);
        m_endRow = numRows;
    }

    /*
     * Allocate data arrays
     */
    int numVerts = ((numRows + 1) * (numCols + 1));
    warpMeshVert * pWarpVertexData = new warpMeshVert[numVerts];

    int numIndices = numRows * numCols * 6; // 2 triangles, 3 verts per triangle
    unsigned int * pIndexBuffer = new unsigned int[numIndices];
    unsigned int * pIndex = pIndexBuffer;

    if (pWarpVertexData == NULL)
    {
        LOGE("Unable to allocate memory for render mesh!");
        return SXR_ERROR_UNKNOWN;
    }
    memset(pWarpVertexData, 0, numVerts * sizeof(warpMeshVert));

    int indexBufferCount = 0;

    /*
     * Load and resmaple the OBJ file for the RED channel
     */
    ResampleMesh(pWarpVertexData, pIndexBuffer, &indexBufferCount, numRows, numCols, true, 0);

    /*
     * Load and resmaple the OBJ file for the GREEN channel
     */

    ResampleMesh(pWarpVertexData, pIndexBuffer, &indexBufferCount,numRows, numCols, false, 1);


    /*
     * Load and resmaple the OBJ file for the BLUE channel
     */
    ResampleMesh(pWarpVertexData, pIndexBuffer, &indexBufferCount, numRows, numCols, false, 2);

    /*
     * Resize and shift the left eye mesh verticies into the correct range (-1..0 in X))
     */
    if (whichMesh == kMeshUL || whichMesh == kMeshLL ||whichMesh == kMeshLeft)
    {
        for (int i = 0; i < numVerts; i++)
        {
            pWarpVertexData[i].Position[0] = (pWarpVertexData[i].Position[0] / 2.f) - 0.5f;
        }
    }

    /*
     * Resize and shift the left eye mesh verticies into the correct range (0..+1 in X))
     */
    if (whichMesh == kMeshUR || whichMesh == kMeshLR || whichMesh == kMeshRight)
    {
        for (int i = 0; i < numVerts; i++)
        {
            pWarpVertexData[i].Position[0] = (pWarpVertexData[i].Position[0] / 2.f) + 0.5f;
        }
    }

    /*
     * Set the UV texture coordinates to be -1..1 horizontally and vertically.
     */
    for (int i = 0; i < numVerts; i++)
    {
        pWarpVertexData[i].TexCoordR[0] = (pWarpVertexData[i].TexCoordR[0] * 2.f) - 1.f;
        pWarpVertexData[i].TexCoordR[1] = (pWarpVertexData[i].TexCoordR[1] * 2.f) - 1.f;

        pWarpVertexData[i].TexCoordG[0] = (pWarpVertexData[i].TexCoordG[0] * 2.f) - 1.f;
        pWarpVertexData[i].TexCoordG[1] = (pWarpVertexData[i].TexCoordG[1] * 2.f) - 1.f;

        pWarpVertexData[i].TexCoordB[0] = (pWarpVertexData[i].TexCoordB[0] * 2.f) - 1.f;
        pWarpVertexData[i].TexCoordB[1] = (pWarpVertexData[i].TexCoordB[1] * 2.f) - 1.f;
    }

    /*
     * Initialize the SDK warp mesh
     */
    warpGeom.Initialize(&gMeshAttribs[0], NUM_VERT_ATTRIBS,
                        pIndexBuffer, indexBufferCount,
                        pWarpVertexData, sizeof(warpMeshVert) * numVerts,
                        numVerts);

    delete [] pWarpVertexData;
    delete [] pIndexBuffer;

    return SXR_ERROR_NONE;
}

/**********************************************************
 * IntializeCustomWarpMesh - Called from SvrApiTimeWarp.cpp
 * to use externally defined data to create display lens mesh
 * when gUseCustomMesh is set to true in svrapi_config.txt.
 **********************************************************/
SxrResult SvrCustomMesh::InitializeCustomWarpMesh(SvrGeometry& warpGeom,
                                                  svrWarpMeshArea whichMesh,
                                                  unsigned int numRows,
                                                  unsigned int numCols)
{
    ifstream inputFile;

    /*
     * Load and parse CSV files.
     */
    if (whichMesh == kMeshUL || whichMesh == kMeshLL ||whichMesh == kMeshLeft)
    {
        unsigned int size = 0;
        int qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_LEFT_FILE, &size, NULL);
        if (qRes == QVR_SUCCESS)
        {
            LOGI("svrCustomMesh: Loading QVRSERVICE_SDK_LENS_LEFT_FILE from QVR Service [size=%d]", size);
            if (size > 0)
            {
                m_pBufferCSV = (char*)malloc(size);
                qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_LEFT_FILE, &size, m_pBufferCSV);
                if (qRes == QVR_SUCCESS)
                {
                    m_pEnd = m_pBufferCSV + size - 1;
                }
                else
                {
                    LOGE("svrCustomMesh: QVRServiceClient_GetParam(QVRSERVICE_SDK_LENS_LEFT_FILE): %s", QVRErrorToString(qRes));
                    return SXR_ERROR_UNKNOWN;
                }
            }
        }
        else
        {
            LOGE("svrCustomMesh: QVRServiceClient_GetParam(QVRSERVICE_SDK_LENS_LEFT_FILE): %s", QVRErrorToString(qRes));

            string fileName = "/system/etc/qvr/svrapi_lens_left.csv";
            LOGI("svrCustomMesh: Loading %s from file", fileName.c_str());
            inputFile.open(fileName.c_str());
            if(inputFile.fail())
            {
                LOGE("svrCustomMesh: Could not find or open file %s", fileName.c_str());
                return SXR_ERROR_UNKNOWN;
            }

            filebuf* pFileBuffer = inputFile.rdbuf();
            size = pFileBuffer->pubseekoff(0, inputFile.end, inputFile.in);
            LOGI("svrCustomMesh: Loaded %s [size=%u]", fileName.c_str(), size);
            pFileBuffer->pubseekpos(0, inputFile.in);
            m_pBufferCSV = (char*)malloc(size);
            pFileBuffer->sgetn(m_pBufferCSV, size);
            inputFile.close();
            m_pEnd = m_pBufferCSV + size - 1;
        }
    }

    if (whichMesh == kMeshUR || whichMesh == kMeshLR || whichMesh == kMeshRight)
    {
        unsigned int size = 0;
        int qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_RIGHT_FILE, &size, NULL);
        if (qRes == QVR_SUCCESS)
        {
            LOGI("svrCustomMesh: Loading QVRSERVICE_SDK_LENS_RIGHT_FILE from QVR Service [size=%d]", size);
            if (size > 0)
            {
                m_pBufferCSV = (char*)malloc(size);
                qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_RIGHT_FILE, &size, m_pBufferCSV);
                if (qRes == QVR_SUCCESS && size > 0)
                {
                    m_pEnd = m_pBufferCSV + size - 1;
                }
                else
                {
                    LOGE("svrCustomMesh: QVRServiceClient_GetParam(QVRSERVICE_SDK_LENS_RIGHT_FILE): %s", QVRErrorToString(qRes));
                    return SXR_ERROR_UNKNOWN;
                }
            }
        }
        else
        {
            LOGE("svrCustomMesh: QVRServiceClient_GetParam(QVRSERVICE_SDK_LENS_RIGHT_FILE): %s", QVRErrorToString(qRes));

            string fileName = "/system/etc/qvr/svrapi_lens_right.csv";
            LOGI("svrCustomMesh: Loading %s from file", fileName.c_str());
            inputFile.open(fileName.c_str());
            if (inputFile.fail())
            {
                LOGE("svrCustomMesh: Could not find or open file %s", fileName.c_str());
                return SXR_ERROR_UNKNOWN;
            }

            filebuf* pFileBuffer = inputFile.rdbuf();
            size = pFileBuffer->pubseekoff(0, inputFile.end, inputFile.in);
            LOGI("svrCustomMesh: Loaded %s [size=%u]", fileName.c_str(), size);
            pFileBuffer->pubseekpos(0, inputFile.in);
            m_pBufferCSV = (char*)malloc(size);
            pFileBuffer->sgetn(m_pBufferCSV, size);
            inputFile.close();
            m_pEnd = m_pBufferCSV + size - 1;
        }
    }

    /*
     * Create SvrGeometry mesh from above data, render XYZ, UV, and
     * read back pixels. Pixels go into m_pBufferXYZ[3] and m_pBufferUV[3]
     * internal buffers.
     */
    SxrResult result;
    result = PreprocessCSV();
    if (result == SXR_ERROR_UNKNOWN)
    {
        return SXR_ERROR_UNKNOWN;
    }

    /*
     *  Create a new mesh using the internal buffers created above.
     */
    result = ResampleCustomWarpMesh(warpGeom, whichMesh, numRows, numCols);
    ReleaseBuffers(true);
    return result;
}

/**********************************************************
 * ReleaseBuffers
 **********************************************************/
SxrResult SvrCustomMesh::ReleaseBuffers(bool doFree)
{
    /*
     * Clean up the render buffers.  The memory buffers may
     * be allocated by this class, so free.  Or they may
     * be allocated by svrCameraManager, so don't free.
     */
    for (int i=0; i < 3; i++)
    {
        if (m_pBufferXYZ[i] != NULL)
        {
            if (doFree == true)
            {
                free(m_pBufferXYZ[i]);
            }
            m_pBufferXYZ[i] = NULL;
        }

        if (m_pBufferUV[i] != NULL)
        {
            if (doFree == true)
            {
                free(m_pBufferUV[i]);
            }
            m_pBufferUV[i] = NULL;
        }
    }

    return SXR_ERROR_NONE;
}

/**********************************************************
 * InitializeShaderFromBuffer
 **********************************************************/
void SvrCustomMesh::InitializeShaderFromBuffer(Svr::SvrShader &whichShader,
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
        LOGE("svrCustomMesh: Shader failed to build. FRAG %s", pFragmentSource);
    }
}


/**********************************************************
 * ResampleMesh - Make a warp mesh (which has 3 sets of
 * UVs for RGB, using the previously rendered XYZ, UV images
 * as the sources to sample from.
 **********************************************************/
void SvrCustomMesh::ResampleMesh(struct warpMeshVert  *warpVertexData,
                                 unsigned int *pIndexBuffer,
                                 int * indexBufferCount,
                                 int numRows,
                                 int numColumns,
                                 bool doXYZ,
                                 int channel)
{
    int targetEyeWidth = gAppContext->deviceInfo.targetEyeWidthPixels;
    int targetEyeHeight = gAppContext->deviceInfo.targetEyeHeightPixels;

    /*
     * Mesh data buffers
     */
    int vertexBufferSize = (numRows+1) * (numColumns+1);
    bool * pValidBuffer = (bool*)malloc(vertexBufferSize * sizeof(bool));
    memset(pValidBuffer, 0, vertexBufferSize*sizeof(bool));
    float * pVertexBufferXYZ = (float*)malloc(vertexBufferSize * 3 * sizeof(float));
    memset(pVertexBufferXYZ, 0, vertexBufferSize * 3 * sizeof(float));

    float * pVertexBufferUV = (float*)malloc(vertexBufferSize * 2 * sizeof(float));
    memset(pVertexBufferUV, 0, vertexBufferSize * 2 * sizeof(float));

    /*
     * Pointer for iteration
     */
    float * pVertXYZ = pVertexBufferXYZ;
    float * pVertUV = pVertexBufferUV;
    bool * pValid = pValidBuffer;

    int searchWidth  = targetEyeWidth / numColumns;
    int searchHeight = targetEyeHeight / numRows;

    /*
     * Process
     */
    int warpIndex = 0;

    for (int yIndex = m_beginRow; yIndex <= m_endRow; yIndex++)
    {
        for (int xIndex=0; xIndex <= numColumns; xIndex++)
        {
            float uCoord = (float)xIndex / (float)numColumns;
            float vCoord = (float)yIndex / (float)numRows;

            float x, y, z, alpha, u, v;
            float alphaRedXYZ, alphaGreenXYZ, alphaBlueXYZ;
            float alphaRedUV, alphaGreenUV, alphaBlueUV;
            z = 0.f;

            int indexOffset;
            int sampleU, sampleV;

            sampleU = (int)((uCoord * (float)targetEyeWidth) + 0.5f);
            sampleV = (int)((vCoord * (float)targetEyeHeight) + 0.5f);

            if (sampleU < 0) sampleU = 0;
            if (sampleU >= targetEyeWidth-1) sampleU = targetEyeWidth-1;

            if (sampleV < 0) sampleV = 0;
            if (sampleV >= targetEyeHeight-1) sampleV = targetEyeHeight-1;

            //X
            indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 0;
            x = *(m_pBufferXYZ[channel] + indexOffset);

            //Y
            indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 1;
            y = *(m_pBufferXYZ[channel] + indexOffset);

            //Z
            indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 2;
            z = *(m_pBufferXYZ[channel] + indexOffset);

            /*
             * Check alpha channel(s) for transparency. Checking both XYZ and UV
             * may seem extreme, but there are situations were disortions are done
             * in XYZ (per color channel), and situations were its done in UV. Check both.
             */
            indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 3;

            // ALPHA XYZ check.
            alphaRedXYZ = *(m_pBufferXYZ[0] + indexOffset);   // Red
            alphaGreenXYZ = *(m_pBufferXYZ[1] + indexOffset); // Green
            alphaBlueXYZ = *(m_pBufferXYZ[2] + indexOffset);  // Blue

            // ALPHA UV check
            alphaRedUV = *(m_pBufferUV[0] + indexOffset);   // Red
            alphaGreenUV = *(m_pBufferUV[1] + indexOffset); // Green
            alphaBlueUV = *(m_pBufferUV[2] + indexOffset);  // Blue


            if (alphaRedXYZ >= 0.99 && alphaGreenXYZ >= 0.99 && alphaBlueXYZ >= 0.99 &&
                alphaRedUV >= 0.99 && alphaGreenUV >= 0.99 && alphaBlueUV >= 0.99)
            {
                alpha = 1.;
            }
            else
            {
                alpha = 0.;
            }

            //U
            indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 0;
            u = *(m_pBufferUV[channel] + indexOffset);

            //V
            indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 1;
            v = *(m_pBufferUV[channel] + indexOffset);

            bool isValid = false;

            /*
             * Check to see if value is on a mesh
             */
            if (alpha >= 0.99f)
            {
                isValid = true;
            }

            if (isValid == false)
            {
                isValid = BorderCheck(uCoord,
                                      vCoord,
                                      m_pBufferXYZ[channel],
                                      m_pBufferUV[channel],
                                      &x,
                                      &y,
                                      &z,
                                      &u,
                                      &v);
            }

            *pVertXYZ++ = x;
            *pVertXYZ++ = y;
            *pVertXYZ++ = z;

            *pVertUV++ = u;
            *pVertUV++ = v;

            *pValid++ = isValid;

            if (doXYZ == true)
            {
                warpVertexData[warpIndex].Position[0] = x;
                warpVertexData[warpIndex].Position[1] = y;
                warpVertexData[warpIndex].Position[2] = z;
                warpVertexData[warpIndex].Position[3] = 1.f;
            }


            if (channel == 0)
            {
                warpVertexData[warpIndex].TexCoordR[0] = u;
                warpVertexData[warpIndex].TexCoordR[1] = v;
                warpVertexData[warpIndex].TexCoordR[2] = 0.f;
                warpVertexData[warpIndex].TexCoordR[3] = 1.f;
            }
            else if (channel == 1)
            {
                warpVertexData[warpIndex].TexCoordG[0] = u;
                warpVertexData[warpIndex].TexCoordG[1] = v;
                warpVertexData[warpIndex].TexCoordG[2] = 0.f;
                warpVertexData[warpIndex].TexCoordG[3] = 1.f;
            }
            else if (channel == 2)
            {
                warpVertexData[warpIndex].TexCoordB[0] = u;
                warpVertexData[warpIndex].TexCoordB[1] = v;
                warpVertexData[warpIndex].TexCoordB[2] = 0.f;
                warpVertexData[warpIndex].TexCoordB[3] = 1.f;
            }

            warpIndex++;
        }
    }


    int a, b, c, d;
    int inner = 0;

    /*
     * Make index list
     */
    if (doXYZ == true)
    {
        int validIndexCount = 0;
        pValid = pValidBuffer;
        unsigned int *pIndex = pIndexBuffer;
        inner = 0;
        for (int y = m_beginRow; y < m_endRow; y++)
        {
            for (int x = 0; x < numColumns; x++)
            {
                a = inner;
                b = inner + 1;
                c = b + (numColumns + 1);
                d = a + (numColumns + 1);

                if (pValid[a] == true &&
                    pValid[b] == true &&
                    pValid[c] == true &&
                    pValid[d] == true)
                {
                    *pIndex++ = a;
                    *pIndex++ = b;
                    *pIndex++ = c;

                    *pIndex++ = a;
                    *pIndex++ = c;
                    *pIndex++ = d;

                    validIndexCount += 6;
                }

                inner++;
            }
            inner++;
        }

        *indexBufferCount = validIndexCount;
    }

    free(pValidBuffer);
    free(pVertexBufferXYZ);
    free(pVertexBufferUV);
    LOGI("svrCustomMesh: Custom Mesh Initialized Successfully");
}

/**********************************************************
 * BorderCheck - return true & new U,V values if a substitute
 * vertex location was found.
 **********************************************************/
bool SvrCustomMesh::BorderCheck(float uCoord,
                                float vCoord,
                                float *pBufferXYZ,
                                float *pBufferUV,
                                float *pX,
                                float *pY,
                                float *pZ,
                                float *pU,
                                float *pV)
{
    int targetEyeWidth = gAppContext->deviceInfo.targetEyeWidthPixels;
    int targetEyeHeight = gAppContext->deviceInfo.targetEyeHeightPixels;

    float uIndexFloat = uCoord * (float)targetEyeWidth;
    float vIndexFloat = vCoord * (float)targetEyeHeight;

    int uIndex = (int)uIndexFloat;
    int vIndex = (int)vIndexFloat;

    int radius = targetEyeWidth / gWarpMeshRows;
    radius = radius + (radius / 2);

    // check right/up/left/down first, then the 45 degrees diagonals next.
    // 0.707 is sqrt(0.5), the x,y for diagonal radus of 1.0
    float axisX[8] = {1.f, 0.f, -1.f, 0.f, 0.707f, -0.707f, -0.707f, 0.707f};
    float axisY[8] = {0.f, 1.f, 0.f, -1.f, 0.707f,  0.707f, -0.707f, -0.707f};

    int sampleU, sampleV;

    for (int i=1; i <= radius; i++)
    {
        for (int direction = 0; direction < 8; direction++)
        {
            /*
             * Check left/right/down/up directions
             */
            sampleU = (int)((uIndexFloat + ((float)i * axisX[direction])) + 0.5f);
            sampleV = (int)((vIndexFloat + ((float)i * axisY[direction])) + 0.5f);

            if (sampleU < 0) sampleU = 0;
            if (sampleU >= targetEyeWidth-1) sampleU = targetEyeWidth-1;

            if (sampleV < 0) sampleV = 0;
            if (sampleV >= targetEyeHeight-1) sampleV = targetEyeHeight-1;


            int indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 3;
            float alphaRedXYZ, alphaGreenXYZ, alphaBlueXYZ;
            float alphaRedUV, alphaGreenUV, alphaBlueUV;

            // ALPHA XYZ check.
            alphaRedXYZ = *(m_pBufferXYZ[0] + indexOffset);   // Red
            alphaGreenXYZ = *(m_pBufferXYZ[1] + indexOffset); // Green
            alphaBlueXYZ = *(m_pBufferXYZ[2] + indexOffset);  // Blue

            // ALPHA UV check
            alphaRedUV = *(m_pBufferUV[0] + indexOffset);   // Red
            alphaGreenUV = *(m_pBufferUV[1] + indexOffset); // Green
            alphaBlueUV = *(m_pBufferUV[2] + indexOffset);  // Blue

            float alpha = 0.;
            if (alphaRedXYZ >= 0.99 && alphaGreenXYZ >= 0.99 && alphaBlueXYZ >= 0.99 &&
                alphaRedUV >= 0.99 && alphaGreenUV >= 0.99 && alphaBlueUV >= 0.99)
            {
                alpha = 1.;
            }
            else
            {
                alpha = 0.;
            }


            if (alpha > 0.999f)
            {

                //X
                indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 0;
                *pX = *(pBufferXYZ + indexOffset);

                //Y
                indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 1;
                *pY = *(pBufferXYZ + indexOffset);

                //Z
                indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 2;
                *pZ = *(pBufferXYZ + indexOffset);

                //U
                indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 0;
                *pU = *(pBufferUV + indexOffset);

                //V
                indexOffset = (sampleV * targetEyeWidth * 4) + (sampleU * 4) + 1;
                *pV = *(pBufferUV + indexOffset);

                return true;
            }
        }
    }


    return false;
}

/**********************************************************
 * PreprocessCSV - renders and stores the XYZ and UV buffers for
 * later mesh generation.
 **********************************************************/
SxrResult SvrCustomMesh::PreprocessCSV()
{
    LOGI("SvrCustomMesh: PreprocessCSV\n");

    int nObjGeom;
    SvrGeometry meshArray[3];

    int targetEyeWidth = gAppContext->deviceInfo.targetEyeWidthPixels;
    int targetEyeHeight = gAppContext->deviceInfo.targetEyeHeightPixels;

    /*
     * Make floating point frame buffer
     */
    unsigned int bufferTextureID;
    glGenTextures(1, &bufferTextureID);
    glBindTexture( GL_TEXTURE_2D, bufferTextureID);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, targetEyeWidth, targetEyeHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    /*
     * Make frame buffer
     */
    unsigned int frameBufferID;
    glGenFramebuffers(1, &frameBufferID);
    glBindFramebuffer( GL_FRAMEBUFFER, frameBufferID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferTextureID, 0);
    int errorCode = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(errorCode != GL_FRAMEBUFFER_COMPLETE)
    {
        LOGE("svrCustomMesh: Custom mesh frame buffer creation failed\n");
        glDeleteTextures(1, &bufferTextureID);
        return SXR_ERROR_UNKNOWN; // error
    }


    /*
     * Viewport
     */
    glClearColor (0., 0., 1., 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, targetEyeWidth, targetEyeHeight);

    /*
     * Matrices
     */
    glm::mat4 viewMatrix;
    viewMatrix = glm::mat4(1.f);

    float aspect = (float)targetEyeWidth / (float)targetEyeHeight;
    glm::mat4 projectionMatrix = glm::frustum(-aspect, aspect,
                                              -1.f, 1.f,
                                              1.f, 100.f);

    glm::mat4 modelMatrix;
    modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, -1.));

    glm::vec3 eyePos = glm::vec3(0, 0, 0);

    /*
     * Build XYZ Shader
     */
    Svr::SvrShader shaderXYZ;
    InitializeShaderFromBuffer(shaderXYZ, remeshVertex, remeshXYZpixel, "Vs", "Fs");
    shaderXYZ.Bind();
    shaderXYZ.SetUniformMat4("projectionMatrix", projectionMatrix);
    shaderXYZ.SetUniformMat4("viewMatrix", viewMatrix);
    shaderXYZ.SetUniformVec3("eyePos", eyePos);
    shaderXYZ.SetUniformMat4("modelMatrix", modelMatrix);
    shaderXYZ.Unbind();

    /*
     * Build UV Shader
     */
    Svr::SvrShader shaderUV;
    InitializeShaderFromBuffer(shaderUV, remeshVertex, remeshUVpixel, "Vs", "Fs");
    shaderUV.Bind();
    shaderUV.SetUniformMat4("projectionMatrix", projectionMatrix);
    shaderUV.SetUniformMat4("viewMatrix", viewMatrix);
    shaderUV.SetUniformVec3("eyePos", eyePos);
    shaderUV.SetUniformMat4("modelMatrix", modelMatrix);
    shaderUV.Unbind();

    /*
     * Load CSV files and render them
     */
    MakeCustomMeshes(meshArray);

    /*
     * For each channel's custom mesh, render and grab pixels
     */
    for (int i=0; i < 3; i++)
    {
        float * pBufferXYZ  = (float*)malloc(targetEyeWidth * targetEyeHeight * 4 * sizeof(float));
        float * pBufferUV  = (float*)malloc(targetEyeWidth * targetEyeHeight * 4 * sizeof(float));

        /*
         * Render/grab XYZ image
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        shaderXYZ.Bind();
        meshArray[i].Submit();
        glReadPixels(0, 0, targetEyeWidth, targetEyeHeight, GL_RGBA, GL_FLOAT, pBufferXYZ);
        shaderXYZ.Unbind();

        /*
         * Render UV image
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        shaderUV.Bind();
        meshArray[i].Submit();
        glReadPixels(0, 0, targetEyeWidth, targetEyeHeight, GL_RGBA, GL_FLOAT, pBufferUV);
        shaderUV.Unbind();

        m_pBufferXYZ[i] = pBufferXYZ;
        m_pBufferUV[i] = pBufferUV;

        meshArray[i].Destroy();

    } // for Red, Green, Blue

    glDeleteTextures(1, &bufferTextureID);
    glDeleteFramebuffers(1, &frameBufferID);

    shaderXYZ.Destroy();
    shaderUV.Destroy();

    return SXR_ERROR_NONE;
}

/**********************************************************
 * Read a line from CSV file, return vector of strings, and
 * count of strings read.
 **********************************************************/
int SvrCustomMesh::ParseLine(std::vector<std::string> &lineData, char **ppCurrent)
{
    char *pCurrent =  *ppCurrent;

    unsigned char delimiter = ',';
    string field;
    string line;

    if (pCurrent == m_pEnd)
    {
        return 0;
    }

    if (*pCurrent == '\n')
    {
        return 0;
    }

    if (*pCurrent == '\0')
    {
        return 0;
    }

    lineData.clear();

    while (pCurrent != NULL && *pCurrent != '\n' && pCurrent != m_pEnd)
    {
        field.clear();
        while (pCurrent != NULL && *pCurrent != ',' && *pCurrent != '\n' && pCurrent != m_pEnd)
        {
            field.push_back(*pCurrent);
            pCurrent++;
        }
        lineData.push_back(field);

        if (field.size() == 0)
        {
            LOGI("SvrCustomMesh: Empty data field found in custom mesh CSV file\n");
        }

        if (*pCurrent == '\n')
        {
            pCurrent++;
            *ppCurrent = (char*) pCurrent;
            return lineData.size();
        }

        if (pCurrent == m_pEnd)
        {
            *ppCurrent = (char*) pCurrent;
            return lineData.size();
        }
        pCurrent++;
    }

    if (pCurrent != NULL && pCurrent != m_pEnd && field.size() == 0)
    {
        pCurrent++;
        lineData.push_back(field);
    }

    *ppCurrent = (char*) pCurrent;

    //LOGI("%s", lineData.data()->c_str());

    return lineData.size();
}

/**********************************************************
 * MakeCustomMeshes - Prase the file data, which is CSV format,
 * and create 3 SvrGeometr meshes, for the 3 color channels
 **********************************************************/
void SvrCustomMesh::MakeCustomMeshes(SvrGeometry meshArray[3])
{
    char buf[1024];
    vector<string> lineData;

    char *pCurrent = m_pBufferCSV;

    // Get File Version
    int count = ParseLine(lineData, &pCurrent);
    float version = atoi(lineData[1].c_str());

    // Distortion Method (0: XYZ distortion 1: UV distortion)
    count = ParseLine(lineData, &pCurrent);
    int distortionMethod = atoi(lineData[1].c_str());

    // get width, height (could be mm, or pixels, we don't care)
    count = ParseLine(lineData, &pCurrent);
    float width = atof(lineData[1].c_str());

    count = ParseLine(lineData, &pCurrent);
    float height = atof(lineData[1].c_str());

    // number of samples in width, height
    count = ParseLine(lineData, &pCurrent);
    int numSamplesHoriz = atoi(lineData[1].c_str());

    count = ParseLine(lineData, &pCurrent);
    int numSamplesVertical = atoi(lineData[1].c_str());

    // Center X, Y
    count = ParseLine(lineData, &pCurrent);
    float centerX = atof(lineData[0].c_str());

    count = ParseLine(lineData, &pCurrent);
    float centerY = atof(lineData[1].c_str());

    count = ParseLine(lineData, &pCurrent);
    count = ParseLine(lineData, &pCurrent);
    count = ParseLine(lineData, &pCurrent);
    count = ParseLine(lineData, &pCurrent);
    count = ParseLine(lineData, &pCurrent);
    count = ParseLine(lineData, &pCurrent);

    SvrProgramAttribute attribs[3];
    int vertexSize = 8 * sizeof(float);

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

    int numVertices = numSamplesHoriz  * numSamplesVertical;

    // Red
    float * pVertexBufferRED = (float*)malloc(vertexSize * numVertices);
    memset(pVertexBufferRED, 0, vertexSize * numVertices);
    float * ptrRed = pVertexBufferRED;

    // Green
    float * pVertexBufferGREEN = (float*)malloc(vertexSize * numVertices);
    memset(pVertexBufferGREEN, 0, vertexSize * numVertices);
    float * ptrGreen = pVertexBufferGREEN;

    // Blue
    float * pVertexBufferBLUE = (float*)malloc(vertexSize * numVertices);
    memset(pVertexBufferBLUE, 0, vertexSize * numVertices);
    float * ptrBlue = pVertexBufferBLUE;

    /*
     * Allocate a 2D buffer, to store 2D ideal points, and 2D actual, RGB for actual.
     * We will use the Index X, Y to know where to store the 2D points. This is because
     * we can't assume the data layout in the CSV is bottom-up raster order.
     */
    int numsPerSample = 10;
    float *pInputBuffer = (float*)malloc(numVertices * sizeof(float) * numsPerSample);
    memset(pInputBuffer, 0, numVertices * sizeof(float) * numsPerSample);
    int rowSize = numsPerSample * numSamplesHoriz;
    int offsetX = (numSamplesHoriz - 1) / 2;
    int offsetY = (numSamplesVertical - 1) / 2;

    /*
     * Read the data, place in the right place
     */
    while (ParseLine(lineData, &pCurrent) != 0)
    {
        int indexX = atoi(lineData[0].c_str());
        int indexY = atoi(lineData[1].c_str());

        indexX = indexX + offsetX;
        indexY = indexY + offsetY;

        int arrayOffset = ((indexY * rowSize) + (indexX * numsPerSample));
        float *ptr = pInputBuffer + arrayOffset;
        for (int n=0; n < numsPerSample; n++)
        {
            ptr[n] = atof(lineData[n].c_str());
        }
    }

    /*
     * Traverse the data in row order, making vertices.
     */
    for (int y=0; y < numSamplesVertical; y++)
    {
        for (int x=0; x < numSamplesHoriz; x++)
        {
            float *ptr = pInputBuffer + ((y * rowSize) + (x * numsPerSample));
            float uRed, vRed, uGreen, vGreen, uBlue, vBlue;
            float xRed, yRed, xGreen, yGreen, xBlue, yBlue;

            if (distortionMethod == 0)
            {
                /*
                 * warp XYZ
                 */
                // xyz
                xRed = ptr[4];
                yRed = ptr[5];

                xGreen = ptr[6];
                yGreen = ptr[7];

                xBlue = ptr[8];
                yBlue = ptr[9];

                // uv
                uRed = ptr[2];
                vRed = ptr[3];

                uGreen = ptr[2];
                vGreen =  ptr[3];

                uBlue = ptr[2];
                vBlue =  ptr[3];
            }
            else
            {
                /*
                 * warp UV
                 */
                // xyz
                xRed = ptr[2];
                yRed = ptr[3];

                xGreen = ptr[2];
                yGreen = ptr[3];

                xBlue = ptr[2];
                yBlue = ptr[3];

                // uv
                uRed = ptr[4];
                vRed = ptr[5];

                uGreen = ptr[6];
                vGreen = ptr[7];

                uBlue = ptr[8];
                vBlue = ptr[9];
            }

            /*
             * Normalize UVs and set to correct range
             */
            uRed = (uRed / width) + 0.5f;
            uGreen = (uGreen / width) + 0.5f;
            uBlue = (uBlue / width) + 0.5f;

            vRed = (vRed / height) + 0.5f;
            vGreen = (vGreen / height) + 0.5f;
            vBlue = (vBlue / height) + 0.5f;

            /*
             * Normalize XYZ  and set to correct range
             */
            xRed = (xRed / width) * 2.f;
            xGreen = (xGreen / width) * 2.f;
            xBlue = (xBlue / width) * 2.f;

            yRed = (yRed / height) * 2.f;
            yGreen = (yGreen / height) * 2.f;
            yBlue = (yBlue / height) * 2.f;

            /*
             * Red
             */
            // XYZ
            *ptrRed = xRed;
            ptrRed++;
            *ptrRed = yRed;
            ptrRed++;
            *ptrRed = 0;
            ptrRed++;

            // Normals
            *ptrRed = 0.;
            ptrRed++;
            *ptrRed = 0.;
            ptrRed++;
            *ptrRed = 1.;
            ptrRed++;

            // Texture Coordinates
            *ptrRed = uRed;
            ptrRed++;
            *ptrRed = vRed;
            ptrRed++;

            /*
             * Green
             */
            // XYZ
            *ptrGreen = xGreen;
            ptrGreen++;
            *ptrGreen = yGreen;
            ptrGreen++;
            *ptrGreen = 0;
            ptrGreen++;

            // Normals
            *ptrGreen = 0.;
            ptrGreen++;
            *ptrGreen = 0.;
            ptrGreen++;
            *ptrGreen = 1.;
            ptrGreen++;

            // Texture Coordinates
            *ptrGreen = uGreen;
            ptrGreen++;
            *ptrGreen = vGreen;
            ptrGreen++;

            /*
             * Blue
             */
            // XYZ
            *ptrBlue = xBlue;
            ptrBlue++;
            *ptrBlue = yBlue;
            ptrBlue++;
            *ptrBlue = 0;
            ptrBlue++;

            // Normals
            *ptrBlue = 0.;
            ptrBlue++;
            *ptrBlue = 0.;
            ptrBlue++;
            *ptrBlue = 1.;
            ptrBlue++;

            // Texture Coordinates
            *ptrBlue = uBlue;
            ptrBlue++;
            *ptrBlue = vBlue;
            ptrBlue++;
        }
    }

    /*
     * Make Index
     */
    int numIndices = (numSamplesHoriz-1) * (numSamplesVertical-1) * 6;
    unsigned int * pIndexBuffer = new unsigned int[numIndices];
    unsigned int * pIndex = pIndexBuffer;
    unsigned int inner = 0;
    unsigned int a, b, c, d;
    unsigned int numPerRow = (unsigned int)numSamplesHoriz;

    for (int y = 0; y < numSamplesVertical-1; y++)
    {
        for (int x = 0; x < numSamplesHoriz-1; x++)
        {
            a = inner;
            b = inner + 1;
            c = b + numPerRow;
            d = a + numPerRow;

            *pIndex = a;
            pIndex++;
            *pIndex = b;
            pIndex++;
            *pIndex = c;
            pIndex++;

            *pIndex = a;
            pIndex++;
            *pIndex = c;
            pIndex++;
            *pIndex = d;
            pIndex++;

            inner++;
        }
        inner++;
    }

    /*
     * Make Red SvrGeometry
     */
    meshArray[0].Initialize(attribs,
                            3,
                            pIndexBuffer,
                            numIndices,
                            pVertexBufferRED,
                            vertexSize * numVertices,
                            numVertices);

    /*
     * Make Green SvrGeometry
     */
    meshArray[1].Initialize(attribs,
                            3,
                            pIndexBuffer,
                            numIndices,
                            pVertexBufferGREEN,
                            vertexSize * numVertices,
                            numVertices);

    /*
     * Make Blue SvrGeometry
     */
    meshArray[2].Initialize(attribs,
                            3,
                            pIndexBuffer,
                            numIndices,
                            pVertexBufferBLUE,
                            vertexSize * numVertices,
                            numVertices);

    free(pVertexBufferRED);
    free(pVertexBufferGREEN);
    free(pVertexBufferBLUE);
    free(pInputBuffer);
}

/**********************************************************
 * SHADERS
 **********************************************************/
char remeshVertex [] =
"#version 320 es\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"in vec2 texcoord0;\n"
"\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 modelMatrix;\n"
"\n"
"out vec3 vWorldPos;\n"
"out vec2 vTexcoord0;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = projectionMatrix * (viewMatrix * (modelMatrix * vec4(position, 1.0)));\n"
"    vWorldPos = position.xyz;\n"
"    vTexcoord0 = texcoord0;\n"
"}\n"
;


/**********************************************************
 * remeshUV - Fragment shader for rendering UV map as color.
 **********************************************************/
char remeshUVpixel [] =
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
"    outColor =  vec4(vTexcoord0.s, vTexcoord0.t, 0., 1.);\n"
"}\n"
;

/**********************************************************
 * remeshXYZ -  Rendering XYZ as color.
 **********************************************************/
char remeshXYZpixel [] =
"#version 320 es\n"
"precision highp float;\n"
"in vec3 vWorldPos;\n"
"\n"
"uniform highp sampler2D srcTex;\n"
"\n"
"out highp vec4 outColor;\n"
"void main()\n"
"{\n"
"    outColor =  vec4(vWorldPos.x, vWorldPos.y, vWorldPos.z, 1.);\n"
"}\n"
;

#endif

