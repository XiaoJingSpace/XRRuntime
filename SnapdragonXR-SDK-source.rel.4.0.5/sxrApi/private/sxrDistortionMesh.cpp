#include <GLES3/gl3.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "svrUtil.h"
#include "svrGeometry.h"
#include "private/svrApiTimeWarp.h"
#include "sxrDistortionMesh.h"

#define QVRSERVICE_SDK_LENS_LEFT_FILE                   "sdk-lens-left-file" 
#define QVRSERVICE_SDK_LENS_RIGHT_FILE                  "sdk-lens-right-file" 

#define NUM_VERT_ATTRIBS    4

// Need to create the layout attributes for the data.
// Must fit these in to reserved types:
//      kPosition  => Position
//      kNormal    => TexCoordR
//      kColor     => TexCoordG
//      kTexcoord0 => TexCoordB
//      kTexcoord1 => Not Used
extern SvrProgramAttribute gMeshAttribs[NUM_VERT_ATTRIBS]; 
//SvrProgramAttribute gMeshAttribs[NUM_VERT_ATTRIBS] =
//        {
//                //  Index               Size    Type        Normalized      Stride                      Offset
//                {kPosition,  4, GL_FLOAT, false, sizeof(warpMeshVert), 0},
//                {kNormal,    4, GL_FLOAT, false, sizeof(warpMeshVert), 16},
//                {kColor,     4, GL_FLOAT, false, sizeof(warpMeshVert), 32},
//                {kTexcoord0, 4, GL_FLOAT, false, sizeof(warpMeshVert), 48}
//        };


/**********************************************************
* SHADERS
**********************************************************/
char gRemeshVertex [] =
        "#version 320 es\n"
        "precision highp float;\n"
        "in vec3 position;\n"
        "in vec2 texcoord0;\n"
        "\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 modelMatrix;\n"
        "\n"
        "out vec3 vWorldPos;\n"
        "out vec2 vTexcoord0;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projectionMatrix * (modelMatrix * vec4(position, 1.0));\n"
        "    vWorldPos = position.xyz;\n"
        "    vTexcoord0 = texcoord0;\n"
        "}\n"
;


/**********************************************************
* remeshUV - Fragment shader for rendering UV map as color.
**********************************************************/
char gRemeshUVpixel [] =
        "#version 320 es\n"
        "precision highp float;\n"
        "in vec2 vTexcoord0;\n"
        "\n"
        "uniform highp sampler2D srcTex;\n"
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
char gRemeshXYZpixel [] =
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


bool SxrDistortionMesh::Initialize(
    glm::vec2 const& leftMeshOffsets,
    glm::vec2 const& rightMeshOffsets,
    uint32_t const surfaceWidth,
    uint32_t const surfaceHeight,
    sxrDeviceInfo const& displayProperties)
{
    LOGI("SxrDistortionMesh::Initialize() Begin");
    assert(surfaceHeight && surfaceWidth);

    FreeLookupTables();

    // Setup GL resources /////////////////////////////////////////////////////
    mBufferWidth = surfaceWidth / 2; // for left & right eye
    mBufferHeight = surfaceHeight;

    GLuint bufferTextureID;
    glGenTextures(1, &bufferTextureID);
    glBindTexture(GL_TEXTURE_2D, bufferTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mBufferWidth, mBufferHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

    GLuint frameBufferID;
    glGenFramebuffers(1, &frameBufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferTextureID, 0);
    int32_t errorCode = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (errorCode != GL_FRAMEBUFFER_COMPLETE)
    {
        LOGE("SxrDistortionMesh::Initialize: Custom mesh frame buffer creation failed");
        glDeleteTextures(1, &bufferTextureID);
        return false;
    }

    glClearColor(0., 0., 1., 0.);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, mBufferWidth, mBufferHeight);

    float aspect = static_cast<float>(mBufferWidth) / static_cast<float>(mBufferHeight);
    glm::mat4 projectionMatrix = glm::frustum(-aspect, aspect,
        -1.f, 1.f,
        1.f, 100.f);

    glm::mat4 modelMatrix;
    modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0f, -1.));


    SvrShader shaderXYZ;
    {
        int32_t numVertStrings = 0;
        int32_t numFragStrings = 0;
        char const* vertShaderStrings[16];
        char const* fragShaderStrings[16];

        vertShaderStrings[numVertStrings++] = gRemeshVertex;
        fragShaderStrings[numFragStrings++] = gRemeshXYZpixel;
        shaderXYZ.Initialize(numVertStrings, vertShaderStrings, numFragStrings, fragShaderStrings,
            "Vs", "Fs");
    }
    shaderXYZ.Bind();
    shaderXYZ.SetUniformMat4("projectionMatrix", projectionMatrix);
    shaderXYZ.SetUniformMat4("modelMatrix", modelMatrix);
    shaderXYZ.Unbind();

    SvrShader shaderUV;
    {
        int32_t numVertStrings = 0;
        int32_t numFragStrings = 0;
        char const* vertShaderStrings[16];
        char const* fragShaderStrings[16];

        vertShaderStrings[numVertStrings++] = gRemeshVertex;
        fragShaderStrings[numFragStrings++] = gRemeshUVpixel;
        shaderUV.Initialize(numVertStrings, vertShaderStrings, numFragStrings, fragShaderStrings,
            "Vs", "Fs");
    }
    shaderUV.Bind();
    shaderUV.SetUniformMat4("projectionMatrix", projectionMatrix);
    shaderUV.SetUniformMat4("modelMatrix", modelMatrix);
    shaderUV.Unbind();
    ///////////////////////////////////////////////////////////////////////////
    SvrGeometry meshArray[3];

    // LEFT EYE ///////////////////////////////////////////////////////////////
    char* gridData = nullptr;
    unsigned int size = 0;
    int qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_LEFT_FILE, &size, NULL);
    if (qRes == QVR_SUCCESS)
    {
        LOGI("sxrDistortionMesh: Loading QVRSERVICE_SDK_LENS_LEFT_FILE from QVR Service [size=%d]", size);
        if (size > 0)
        {
            gridData = (char*)malloc(size);
            qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_LEFT_FILE, &size, gridData);
            if (qRes != QVR_SUCCESS)
            {
                LOGE("sxrDistortionMesh: QVRServiceClient_SetClientStatusCallback(QVRSERVICE_SDK_LENS_LEFT_FILE): %s", QVRErrorToString(qRes));
                return false;
            }
        }
    }
    if (!MakeCustomMeshes(gridData, meshArray))
        return false;

    for (int32_t i = 0; i < 3; ++i)
    {
        mLeftBufferXyz[i] = new float[mBufferWidth * mBufferHeight * 4];
        mLeftBufferUv[i] = new float[mBufferWidth * mBufferHeight * 4];

        glClear(GL_COLOR_BUFFER_BIT);
        shaderXYZ.Bind();
        meshArray[i].Submit();
        glReadPixels(0, 0, mBufferWidth, mBufferHeight, GL_RGBA, GL_FLOAT, &mLeftBufferXyz[i][0]);
        shaderXYZ.Unbind();

        glClear(GL_COLOR_BUFFER_BIT);
        shaderUV.Bind();
        meshArray[i].Submit();
        glReadPixels(0, 0, mBufferWidth, mBufferHeight, GL_RGBA, GL_FLOAT, &mLeftBufferUv[i][0]);
        shaderUV.Unbind();
    }
    ///////////////////////////////////////////////////////////////////////////

    delete gridData;
    for (int32_t i = 0; i < 3; ++i)
        meshArray[i].Destroy();

    // RIGHT EYE //////////////////////////////////////////////////////////////
    qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_RIGHT_FILE, &size, NULL);
    if (qRes == QVR_SUCCESS)
    {
        LOGI("sxrDistortionMesh: Loading QVRSERVICE_SDK_LENS_RIGHT_FILE from QVR Service [size=%d]", size);
        if (size > 0)
        {
            gridData = (char*)malloc(size);
            qRes = QVRServiceClient_GetParam(gAppContext->qvrHelper, QVRSERVICE_SDK_LENS_RIGHT_FILE, &size, gridData);
            if (qRes != QVR_SUCCESS && size > 0)
            {
                LOGE("sxrDistortionMesh: QVRServiceClient_SetClientStatusCallback(QVRSERVICE_SDK_LENS_RIGHT_FILE): %s", QVRErrorToString(qRes));
                return false;
            }
        }
    }
    if (!MakeCustomMeshes(gridData, meshArray))
        return false;

    for (int32_t i = 0; i < 3; ++i)
    {
        mRightBufferXyz[i] = new float[mBufferWidth * mBufferHeight * 4];
        mRightBufferUv[i] = new float[mBufferWidth * mBufferHeight * 4];

        glClear(GL_COLOR_BUFFER_BIT);
        shaderXYZ.Bind();
        meshArray[i].Submit();
        glReadPixels(0, 0, mBufferWidth, mBufferHeight, GL_RGBA, GL_FLOAT, &mRightBufferXyz[i][0]);
        shaderXYZ.Unbind();

        glClear(GL_COLOR_BUFFER_BIT);
        shaderUV.Bind();
        meshArray[i].Submit();
        glReadPixels(0, 0, mBufferWidth, mBufferHeight, GL_RGBA, GL_FLOAT, &mRightBufferUv[i][0]);
        shaderUV.Unbind();
    }
    ///////////////////////////////////////////////////////////////////////////

    // Clean up resources
    delete gridData;
    for (int32_t i = 0; i < 3; ++i)
        meshArray[i].Destroy();

    glDeleteTextures(1, &bufferTextureID);
    glDeleteFramebuffers(1, &frameBufferID);

    shaderXYZ.Destroy();
    shaderUV.Destroy();

    bool hasError = false; // CheckGlError(nullptr, true, "SxrDistortionMesh::Initialize", "failed to create static custom meshes.");
    SvrCheckGlError(__FILE__, __LINE__);

    LOGI("SxrDistortionMesh::Initialize() End");

    return !hasError;
}

void SxrDistortionMesh::Finalize()
{
    FreeLookupTables();
    for (int32_t i=0; i < 3; ++i)
    {
        if (mLeftBufferXyz[i])
        {
            delete [] mLeftBufferXyz[i];
            mLeftBufferXyz[i] = nullptr;
        }

        if (mRightBufferXyz[i])
        {
            delete [] mRightBufferXyz[i];
            mRightBufferXyz[i] = nullptr;
        }

        if (mLeftBufferUv[i])
        {
            delete [] mLeftBufferUv[i];
            mLeftBufferUv[i] = nullptr;
        }

        if (mRightBufferUv[i])
        {
            delete [] mRightBufferUv[i];
            mRightBufferUv[i] = nullptr;
        }
    }

    for (int32_t i = 0; i < kNumEyes; ++i)
    {
        mProjectionMeshes[i].Destroy();
        mEquirectMeshes[i].Destroy();
    }
}

/**********************************************************
    * Read a line from CSV file, return vector of strings, and
    * count of strings read.
    **********************************************************/
int SxrDistortionMesh::ParseLine(std::vector<std::string> &lineData, char **ppCurrent)
{
    char *pCurrent = *ppCurrent;

    unsigned char delimiter = ',';
    string field;
    string line;

    if (*pCurrent == '\n')
    {
        return 0;
    }

    if (*pCurrent == '\0')
    {
        return 0;
    }

    lineData.clear();

    while (pCurrent != NULL && *pCurrent != '\n' && *pCurrent != '\0')
    {
        field.clear();
        while (pCurrent != NULL && *pCurrent != ',' && *pCurrent != '\n' && *pCurrent != '\0')
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
            *ppCurrent = (char*)pCurrent;
            return lineData.size();
        }

        if (*pCurrent == '\0')
        {
            *ppCurrent = (char*)pCurrent;
            return lineData.size();
        }
        pCurrent++;
    }

    if (pCurrent != NULL && *pCurrent != '\0' && field.size() == 0)
    {
        pCurrent++;
        lineData.push_back(field);
    }

    *ppCurrent = (char*)pCurrent;

    //LOGI("%s", lineData.data()->c_str());

    return lineData.size();
}

/**********************************************************
* MakeCustomMeshes - Parse the file data, which is CSV format,
* and create 3 SvrGeometr meshes, for the 3 color channels
**********************************************************/
bool SxrDistortionMesh::MakeCustomMeshes(char* pBufferCSV, SvrGeometry meshArray[3])
{
    char buf[1024];
    vector<string> lineData;

    char *pCurrent = pBufferCSV;

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

    int numVertices = numSamplesHoriz * numSamplesVertical;

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
        for (int n = 0; n < numsPerSample; n++)
        {
            ptr[n] = atof(lineData[n].c_str());
        }
    }

    /*
    * Traverse the data in row order, making vertices.
    */
    for (int y = 0; y < numSamplesVertical; y++)
    {
        for (int x = 0; x < numSamplesHoriz; x++)
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
                vGreen = ptr[3];

                uBlue = ptr[2];
                vBlue = ptr[3];
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
    int numIndices = (numSamplesHoriz - 1) * (numSamplesVertical - 1) * 6;
    unsigned int * pIndexBuffer = new unsigned int[numIndices];
    unsigned int * pIndex = pIndexBuffer;
    unsigned int inner = 0;
    unsigned int a, b, c, d;
    unsigned int numPerRow = (unsigned int)numSamplesHoriz;

    for (int y = 0; y < numSamplesVertical - 1; y++)
    {
        for (int x = 0; x < numSamplesHoriz - 1; x++)
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

    bool hasError = false; // CheckGlError(nullptr, true, "SxrDistortionMesh::CreateCustomMeshes", "failed to create custom meshes.");
    SvrCheckGlError(__FILE__, __LINE__);

    return !hasError;
}

#if 0
bool SxrDistortionMesh::CreateCustomMeshes(SvrDistortionGridData const& grid, SvrGeometry meshArray[3])
{
    SvrProgramAttribute attribs[2];
    int32_t const numElementsPerVert = 5;
    int32_t const vertexSize = numElementsPerVert * sizeof(float);

    attribs[0].index = QtiGL::kPosition;
    attribs[0].size = 3;
    attribs[0].type = GL_FLOAT;
    attribs[0].normalized = false;
    attribs[0].stride = vertexSize;
    attribs[0].offset = 0;

    attribs[1].index = QtiGL::kTexcoord0;
    attribs[1].size = 2;
    attribs[1].type = GL_FLOAT;
    attribs[1].normalized = false;
    attribs[1].stride = vertexSize;
    attribs[1].offset = 3 * sizeof(float);

    int32_t numVertices = grid.numCols * grid.numRows;

    // Red
    std::vector<float> vertexBufferRED;
    vertexBufferRED.resize(numElementsPerVert * numVertices, 0.f);

    // Green
    std::vector<float> vertexBufferGREEN;
    vertexBufferGREEN.resize(numElementsPerVert * numVertices, 0.f);

    // Blue
    std::vector<float> vertexBufferBLUE;
    vertexBufferBLUE.resize(numElementsPerVert * numVertices, 0.f);

    // Tmp buffer (can't assume the data layout is bottom-up raster order)
    int32_t const numsPerSample = 10; // TODO: this might change with future versions?  Will need to do the right thing based on version #
    std::vector<float> inputBuffer;
    inputBuffer.resize(numVertices * numsPerSample, 0.f);
    int32_t const rowSize = numsPerSample * grid.numCols;
    int32_t const offsetX = (grid.numCols - 1) / 2;
    int32_t const offsetY = (grid.numRows - 1) / 2;

    std::vector<bool> validBuffer;
    validBuffer.resize(numVertices, false);

    for (QtiCompositor::DistortionGridData::Data const& data : grid.data)
    {
        int32_t const indexX = data.indexX + offsetX;
        int32_t const indexY = data.indexY + offsetY;

        int32_t const arrayOffset = ((indexY * rowSize) + (indexX * numsPerSample));

        validBuffer[(indexY * grid.numRows) + indexX] = true;

        inputBuffer[arrayOffset + 0] = data.indexX;
        inputBuffer[arrayOffset + 1] = data.indexY;
        inputBuffer[arrayOffset + 2] = data.refLocX;
        inputBuffer[arrayOffset + 3] = data.refLocY;
        inputBuffer[arrayOffset + 4] = data.rDistX;
        inputBuffer[arrayOffset + 5] = data.rDistY;
        inputBuffer[arrayOffset + 6] = data.gDistX;
        inputBuffer[arrayOffset + 7] = data.gDistY;
        inputBuffer[arrayOffset + 8] = data.bDistX;
        inputBuffer[arrayOffset + 9] = data.bDistY;
    }

    // We can now generate verts
    int32_t redIndex = 0;
    int32_t greenIndex = 0;
    int32_t blueIndex = 0;
    for (int32_t y = 0; y < grid.numRows; ++y)
    {
        for (int32_t x = 0; x < grid.numCols; ++x)
        {
            int32_t offset = ((y * rowSize) + (x * numsPerSample));
            float uRed, vRed, uGreen, vGreen, uBlue, vBlue;
            float xRed, yRed, xGreen, yGreen, xBlue, yBlue;

            if (grid.distortionMethod == 0)
            {
                /*
                * warp XYZ
                */
                // xyz
                xRed = inputBuffer[offset+4];
                yRed = inputBuffer[offset+5];

                xGreen = inputBuffer[offset+6];
                yGreen = inputBuffer[offset+7];

                xBlue = inputBuffer[offset+8];
                yBlue = inputBuffer[offset+9];

                // uv
                uRed = inputBuffer[offset+2];
                vRed = inputBuffer[offset+3];

                uGreen = inputBuffer[offset+2];
                vGreen =  inputBuffer[offset+3];

                uBlue = inputBuffer[offset+2];
                vBlue = inputBuffer[offset+3];
            }
            else
            {
                /*
                * warp UV
                */
                // xyz
                xRed = inputBuffer[offset+2];
                yRed = inputBuffer[offset+3];

                xGreen = inputBuffer[offset+2];
                yGreen = inputBuffer[offset+3];

                xBlue = inputBuffer[offset+2];
                yBlue = inputBuffer[offset+3];

                // uv
                uRed = inputBuffer[offset+4];
                vRed = inputBuffer[offset+5];

                uGreen = inputBuffer[offset+6];
                vGreen = inputBuffer[offset+7];

                uBlue = inputBuffer[offset+8];
                vBlue = inputBuffer[offset+9];
            }

            /*
            * Normalize UVs and set to correct range
            */
            uRed = (uRed / grid.width) + 0.5f;
            uGreen = (uGreen / grid.width) + 0.5f;
            uBlue = (uBlue / grid.width) + 0.5f;

            vRed = (vRed / grid.height) + 0.5f;
            vGreen = (vGreen / grid.height) + 0.5f;
            vBlue = (vBlue / grid.height) + 0.5f;

            /*
            * Normalize XYZ  and set to correct range
            */
            xRed = (xRed / grid.width) * 2.f;
            xGreen = (xGreen / grid.width) * 2.f;
            xBlue = (xBlue / grid.width) * 2.f;

            yRed = (yRed / grid.height) * 2.f;
            yGreen = (yGreen / grid.height) * 2.f;
            yBlue = (yBlue / grid.height) * 2.f;

            /*
            * Red
            */
            // XYZ
            vertexBufferRED[redIndex++] = xRed;
            vertexBufferRED[redIndex++] = yRed;
            vertexBufferRED[redIndex++] = 0;

            // Texture Coordinates
            vertexBufferRED[redIndex++] = uRed;
            vertexBufferRED[redIndex++] = vRed;

            /*
            * Green
            */
            // XYZ
            vertexBufferGREEN[greenIndex++] = xGreen;
            vertexBufferGREEN[greenIndex++] = yGreen;
            vertexBufferGREEN[greenIndex++] = 0;

            // Texture Coordinates
            vertexBufferGREEN[greenIndex++] = uGreen;
            vertexBufferGREEN[greenIndex++] = vGreen;

            /*
            * Blue
            */
            // XYZ
            vertexBufferBLUE[blueIndex++] = xBlue;
            vertexBufferBLUE[blueIndex++] = yBlue;
            vertexBufferBLUE[blueIndex++] = 0;

            // Texture Coordinates
            vertexBufferBLUE[blueIndex++] = uBlue;
            vertexBufferBLUE[blueIndex++] = vBlue;
        }
    }

    // Generate indices
    std::vector<uint32_t> indexBuffer;
    uint32_t inner = 0;
    uint32_t a, b, c, d;
    uint32_t const numPerRow = static_cast<uint32_t>(grid.numCols);

    for (int32_t y = 0; y < grid.numRows-1; ++y)
    {
        for (int32_t x = 0; x < grid.numCols-1; ++x)
        {
            a = inner;
            b = inner + 1;
            c = b + numPerRow;
            d = a + numPerRow;

            if (validBuffer[a] &&
                validBuffer[b] &&
                validBuffer[c] &&
                validBuffer[d])
            {
                indexBuffer.push_back(a);
                indexBuffer.push_back(b);
                indexBuffer.push_back(c);

                indexBuffer.push_back(a);
                indexBuffer.push_back(c);
                indexBuffer.push_back(d);
            }

            inner++;
        }
        inner++;
    }

    // Init the actual meshes
    meshArray[0].Initialize(
            attribs, 2,
            &indexBuffer[0], indexBuffer.size(),
            &vertexBufferRED[0], vertexSize * numVertices,
            numVertices);

    meshArray[1].Initialize(
            attribs, 2,
            &indexBuffer[0], indexBuffer.size(),
            &vertexBufferGREEN[0], vertexSize * numVertices,
            numVertices);

    meshArray[2].Initialize(
            attribs, 2,
            &indexBuffer[0], indexBuffer.size(),
            &vertexBufferBLUE[0], vertexSize * numVertices,
            numVertices);

    bool hasError = CheckGlError(nullptr, true, "SxrDistortionMesh::CreateCustomMeshes",
                                    "failed to create custom meshes.");

    return !hasError;
}
#endif

bool SxrDistortionMesh::CreateProjectionLayerMesh(SvrGeometry& projectionMesh, int32_t const whichEye, int32_t const whichMesh, int32_t const mNumRows, int32_t const mNumColumns)
{
    LOGI("SxrDistortionMesh::CreateProjectionLayerMeshes(%d)", whichEye);

    float* bufferXyz[3] = {nullptr, nullptr, nullptr};
    float* bufferUv[3] = {nullptr, nullptr, nullptr};

    if (kLeftEye == whichEye)
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            bufferXyz[i] = mLeftBufferXyz[i];
            bufferUv[i] = mLeftBufferUv[i];
        }
    }
    else
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            bufferXyz[i] = mRightBufferXyz[i];
            bufferUv[i] = mRightBufferUv[i];
        }
    }

    /*
        * See if wee need to chop up the mesh into upper/lower sections.
        */

    int32_t beginRow = 0;
    int32_t endRow = mNumRows;

    if (whichMesh == kMeshLL || whichMesh == kMeshLR)
    {
        beginRow = 0;
        endRow = (mNumRows / 2);
    }
    if (whichMesh == kMeshUL || whichMesh == kMeshUR)
    {
        beginRow = (mNumRows / 2);
        endRow = mNumRows;
    }

    int32_t const vertexBufferSize = (mNumRows+1) * (mNumColumns+1);

    std::vector<warpMeshVert> vertices;
    vertices.resize(vertexBufferSize);

    std::vector<bool> validBuffer;
    validBuffer.resize(vertexBufferSize);

    int32_t warpIndex = 0;

    for (int32_t yIndex = beginRow; yIndex <= endRow; ++yIndex)
    {
        for (int32_t xIndex = 0; xIndex <= mNumColumns; ++xIndex)
        {
            float const uCoord = static_cast<float>(xIndex) / static_cast<float>(mNumColumns);
            float const vCoord = static_cast<float>(yIndex) / static_cast<float>(mNumRows);

            float x, y, z, u[3], v[3];
            float alphaRed, alphaGreen, alphaBlue;

            int32_t indexOffset;
            int32_t sampleU, sampleV;

            sampleU = static_cast<int32_t>((uCoord * static_cast<float>(mBufferWidth)) + 0.5f);
            sampleV = static_cast<int32_t>((vCoord * static_cast<float>(mBufferHeight)) + 0.5f);

            if (sampleU < 0)
                sampleU = 0;
            if (sampleU >= mBufferWidth-1)
                sampleU = mBufferWidth-1;

            if (sampleV < 0)
                sampleV = 0;
            if (sampleV >= mBufferHeight-1)
                sampleV = mBufferHeight-1;

            //X
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            x = bufferXyz[0][indexOffset];

            //Y
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            y = bufferXyz[0][indexOffset];

            //Z
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 2;
            z = bufferXyz[0][indexOffset];

            // ALPHA RED - The alpha value from the RED mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaRed = bufferXyz[0][indexOffset];

            // ALPHA GREEN - The alpha value from the GREEN mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaGreen = bufferXyz[1][indexOffset];

            // ALPHA BLUE - The alpha value from the BLUE mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaBlue = bufferXyz[2][indexOffset];

            bool alpha = false;
            if (alphaRed >= 0.99f && alphaGreen >= 0.99f && alphaBlue >= 0.99f)
                alpha = true;

            //U
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            u[0] = bufferUv[0][indexOffset];
            u[1] = bufferUv[1][indexOffset];
            u[2] = bufferUv[2][indexOffset];

            //V
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            v[0] = bufferUv[0][indexOffset];
            v[1] = bufferUv[1][indexOffset];
            v[2] = bufferUv[2][indexOffset];

            // Also ensure proper UVs
            bool uvsValid = true;
            if ((u[0] < 0.f || u[0] > 1.f) ||
                (u[1] < 0.f || u[1] > 1.f) ||
                (u[2] < 0.f || u[2] > 1.f) ||
                (v[0] < 0.f || v[0] > 1.f) ||
                (v[1] < 0.f || v[1] > 1.f) ||
                (v[2] < 0.f || v[2] > 1.f))
                uvsValid = false;

            bool isValid = false;

            /*
            * Check to see if value is on a mesh
            */
            if (alpha && uvsValid)
            {
                isValid = true;
            }
            else
            {
                isValid = BorderCheck(uCoord,
                                        vCoord,
                                        bufferXyz,
                                        bufferUv,
                                        x, y, z,
                                        u[0], v[0],
                                        u[1], v[1],
                                        u[2], v[2],
                                        mNumRows);
            }

            validBuffer[warpIndex] = isValid;

            vertices[warpIndex].Position[0] = x;
            vertices[warpIndex].Position[1] = y;
            vertices[warpIndex].Position[2] = z;
            vertices[warpIndex].Position[3] = 1.f;

            if (kLeftEye == whichEye)
                vertices[warpIndex].Position[0] = (vertices[warpIndex].Position[0] / 2.f) - 0.5f;
            else
                vertices[warpIndex].Position[0] = (vertices[warpIndex].Position[0] / 2.f) + 0.5f;

            vertices[warpIndex].TexCoordR[0] = u[0];
            vertices[warpIndex].TexCoordR[1] = v[0];
            vertices[warpIndex].TexCoordR[2] = 0.f;
            vertices[warpIndex].TexCoordR[3] = 1.f;

            vertices[warpIndex].TexCoordR[0] = (vertices[warpIndex].TexCoordR[0] * 2.f) - 1.f;
            vertices[warpIndex].TexCoordR[1] = (vertices[warpIndex].TexCoordR[1] * 2.f) - 1.f;

            vertices[warpIndex].TexCoordG[0] = u[1];
            vertices[warpIndex].TexCoordG[1] = v[1];
            vertices[warpIndex].TexCoordG[2] = 0.f;
            vertices[warpIndex].TexCoordG[3] = 1.f;

            vertices[warpIndex].TexCoordG[0] = (vertices[warpIndex].TexCoordG[0] * 2.f) - 1.f;
            vertices[warpIndex].TexCoordG[1] = (vertices[warpIndex].TexCoordG[1] * 2.f) - 1.f;

            vertices[warpIndex].TexCoordB[0] = u[2];
            vertices[warpIndex].TexCoordB[1] = v[2];
            vertices[warpIndex].TexCoordB[2] = 0.f;
            vertices[warpIndex].TexCoordB[3] = 1.f;

            vertices[warpIndex].TexCoordB[0] = (vertices[warpIndex].TexCoordB[0] * 2.f) - 1.f;
            vertices[warpIndex].TexCoordB[1] = (vertices[warpIndex].TexCoordB[1] * 2.f) - 1.f;

            warpIndex++;
        }
    }

    // Index buffer time
    std::vector<uint32_t> indices;
    uint32_t inner = 0;
    for (int32_t y = beginRow; y < endRow; ++y)
    {
        for (int x = 0; x < mNumColumns; ++x)
        {
            uint32_t a = inner;
            uint32_t b = inner + 1;
            uint32_t c = b + (mNumColumns + 1);
            uint32_t d = a + (mNumColumns + 1);

            if (validBuffer[a] &&
                validBuffer[b] &&
                validBuffer[c] &&
                validBuffer[d])
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);

                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);
            }
            inner++;
        }
        inner++;
    }

    // Create geometry
    // If mesh has been created already, need to destroy it first
    if (projectionMesh.GetVertexCount() ||
        projectionMesh.GetIndexCount())
    {
        projectionMesh.Destroy();
    }

    projectionMesh.Initialize(
            &gMeshAttribs[0], NUM_VERT_ATTRIBS,
            &indices[0], indices.size(),
            &vertices[0], vertices.size() * sizeof(warpMeshVert), vertices.size());

    return true;
}

bool SxrDistortionMesh::CreateEquirectLayerMesh(SvrGeometry& equirectMesh, int32_t const whichEye, int32_t const whichMesh, int32_t const mNumRows, int32_t const mNumColumns, sxrDeviceInfo const& displayProperties)
{
    LOGI("SxrDistortionMesh::CreateEquirectLayerMeshes(%d)", whichEye);

    float const leftThetaA = atan2(fabs(displayProperties.leftEyeFrustum.left), displayProperties.leftEyeFrustum.near);
    float const leftThetaB = atan2(fabs(displayProperties.leftEyeFrustum.right), displayProperties.leftEyeFrustum.near);
    float const leftPhiA = atan2(fabs(displayProperties.leftEyeFrustum.top), displayProperties.leftEyeFrustum.near);
    float const leftPhiB = atan2(fabs(displayProperties.leftEyeFrustum.bottom), displayProperties.leftEyeFrustum.near);

    float const rightThetaA = atan2(fabs(displayProperties.rightEyeFrustum.left), displayProperties.rightEyeFrustum.near);
    float const rightThetaB = atan2(fabs(displayProperties.rightEyeFrustum.right), displayProperties.rightEyeFrustum.near);
    float const rightPhiA = atan2(fabs(displayProperties.rightEyeFrustum.top), displayProperties.rightEyeFrustum.near);
    float const rightPhiB = atan2(fabs(displayProperties.rightEyeFrustum.bottom), displayProperties.rightEyeFrustum.near);

    glm::mat4 const leftProjMtx = glm::frustum(displayProperties.leftEyeFrustum.left,
                                                    displayProperties.leftEyeFrustum.right,
                                                    displayProperties.leftEyeFrustum.bottom,
                                                    displayProperties.leftEyeFrustum.top,
                                                    displayProperties.leftEyeFrustum.near,
                                                    displayProperties.leftEyeFrustum.far);
    glm::mat4 const rightProjMtx = glm::frustum(displayProperties.rightEyeFrustum.left,
                                                    displayProperties.rightEyeFrustum.right,
                                                    displayProperties.rightEyeFrustum.bottom,
                                                    displayProperties.rightEyeFrustum.top,
                                                    displayProperties.rightEyeFrustum.near,
                                                    displayProperties.rightEyeFrustum.far);

    glm::mat4 const leftInvProjMtx = inverse(leftProjMtx);
    glm::mat4 const rightInvProjMtx = inverse(rightProjMtx);

    float minTheta = 0.0f;
    float deltaTheta = 0.0f;
    float minPhi = 0.0f;
    float deltaPhi = 0.0f;

    glm::mat4 thisProjMtx;
    glm::mat4 thisInvProjMtx;

    switch (whichEye)
    {
        case kLeftEye:
            minTheta = -leftThetaA;
            deltaTheta = (leftThetaA + leftThetaB) /
                            (float) mNumColumns;    // These are both positive angles
            minPhi = -leftPhiB;
            deltaPhi = (leftPhiA + leftPhiB) /
                        (float) mNumRows;          // These are both positive angles
            thisProjMtx = leftProjMtx;
            thisInvProjMtx = leftInvProjMtx;
            break;

        case kRightEye:
            minTheta = -rightThetaA;
            deltaTheta = (rightThetaA + rightThetaB) /
                            (float) mNumColumns;  // These are both positive angles
            minPhi = -rightPhiB;
            deltaPhi = (rightPhiA + rightPhiB) /
                        (float) mNumRows;        // These are both positive angles
            thisProjMtx = rightProjMtx;
            thisInvProjMtx = rightInvProjMtx;
            break;

        default:
            LOGE("SxrDistortionMesh::CreateEquirectLayerMeshes: Invalid eye");
            return false;
    }


    float* bufferXyz[3] = {nullptr, nullptr, nullptr};
    float* bufferUv[3] = {nullptr, nullptr, nullptr};

    if (kLeftEye == whichEye)
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            bufferXyz[i] = mLeftBufferXyz[i];
            bufferUv[i] = mLeftBufferUv[i];
        }
    }
    else
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            bufferXyz[i] = mRightBufferXyz[i];
            bufferUv[i] = mRightBufferUv[i];
        }
    }

    int32_t beginRow = 0;
    int32_t endRow = mNumRows;

    if (whichMesh == kMeshLL || whichMesh == kMeshLR)
    {
        beginRow = 0;
        endRow = (mNumRows / 2);
    }
    if (whichMesh == kMeshUL || whichMesh == kMeshUR)
    {
        beginRow = (mNumRows / 2);
        endRow = mNumRows;
    }

    int32_t const vertexBufferSize = (mNumRows+1) * (mNumColumns+1);

    std::vector<warpMeshVert> vertices;
    vertices.resize(vertexBufferSize);

    std::vector<bool> validBuffer;
    validBuffer.resize(vertexBufferSize);

    int32_t warpIndex = 0;

    for (uint32_t whichX = beginRow; whichX <= endRow; ++whichX)
    {
        for (uint32_t whichY = 0; whichY <= mNumRows; ++whichY)
        {
            float const thisTheta = minTheta + deltaTheta * (float)whichX;
            float const thisPhi = minPhi + deltaPhi * (float)whichY;

            glm::vec4 thisPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec4 screenPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec4 warpWorldPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            // Looking down Z-Axis (Building from Left to Right and Bottom to Top)
            float zPos = -mEquirectRad * cos(thisPhi) * cos(thisTheta);
            float xPos = mEquirectRad * cos(thisPhi) * sin(thisTheta);
            float yPos = mEquirectRad * sin(thisPhi);

            // Need Screen Coordinates
            thisPos = glm::vec4(xPos, yPos, zPos, 1.0f);
            screenPos = thisProjMtx * thisPos;
            screenPos /= screenPos.w;   // Want NDC coordinates so divide by W component

            float const uCoord = (screenPos.x / 2.f) + 0.5f;
            float const vCoord = (screenPos.y / 2.f) + 0.5f;

            float x, y, z, u[3], v[3];
            float alphaRed, alphaGreen, alphaBlue;

            int32_t indexOffset;
            int32_t sampleU, sampleV;

            sampleU = static_cast<int32_t>((uCoord * static_cast<float>(mBufferWidth)) + 0.5f);
            sampleV = static_cast<int32_t>((vCoord * static_cast<float>(mBufferHeight)) + 0.5f);

            if (sampleU < 0)
                sampleU = 0;
            if (sampleU >= mBufferWidth-1)
                sampleU = mBufferWidth-1;

            if (sampleV < 0)
                sampleV = 0;
            if (sampleV >= mBufferHeight-1)
                sampleV = mBufferHeight-1;

            //X
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            x = bufferXyz[0][indexOffset];

            //Y
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            y = bufferXyz[0][indexOffset];

            //Z
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 2;
            z = bufferXyz[0][indexOffset];

            // ALPHA RED - The alpha value from the RED mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaRed = bufferXyz[0][indexOffset];

            // ALPHA GREEN - The alpha value from the GREEN mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaGreen = bufferXyz[1][indexOffset];

            // ALPHA BLUE - The alpha value from the BLUE mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaBlue = bufferXyz[2][indexOffset];

            bool alpha = false;
            if (alphaRed >= 0.99f && alphaGreen >= 0.99f && alphaBlue >= 0.99f)
                alpha = true;

            //U
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            u[0] = bufferUv[0][indexOffset];
            u[1] = bufferUv[1][indexOffset];
            u[2] = bufferUv[2][indexOffset];

            //V
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            v[0] = bufferUv[0][indexOffset];
            v[1] = bufferUv[1][indexOffset];
            v[2] = bufferUv[2][indexOffset];

            // Also ensure proper UVs
            bool uvsValid = true;
            if ((u[0] < 0.f || u[0] > 1.f) ||
                (u[1] < 0.f || u[1] > 1.f) ||
                (u[2] < 0.f || u[2] > 1.f) ||
                (v[0] < 0.f || v[0] > 1.f) ||
                (v[1] < 0.f || v[1] > 1.f) ||
                (v[2] < 0.f || v[2] > 1.f))
                uvsValid = false;

            bool isValid = false;

            /*
            * Check to see if value is on a mesh
            */
            if (alpha && uvsValid)
            {
                isValid = true;
            }
            else
            {
                isValid = BorderCheck(uCoord,
                                        vCoord,
                                        bufferXyz,
                                        bufferUv,
                                        x, y, z,
                                        u[0], v[0],
                                        u[1], v[1],
                                        u[2], v[2],
                                        mNumRows);
            }

            validBuffer[warpIndex] = isValid;

            vertices[warpIndex].Position[0] = x;
            vertices[warpIndex].Position[1] = y;
            vertices[warpIndex].Position[2] = screenPos.z;
            vertices[warpIndex].Position[3] = 1.f;

            if (kLeftEye == whichEye)
                vertices[warpIndex].Position[0] = (vertices[warpIndex].Position[0] / 2.f) - 0.5f;
            else
                vertices[warpIndex].Position[0] = (vertices[warpIndex].Position[0] / 2.f) + 0.5f;


            float uvRatioU = 1.0f;
            float uvRatioV = 1.0f;

            // Base UV coordinates are in [0, 1] and we need them in [-1, 1]
            float baseUPrime = 2.0f * uCoord - 1.0f;
            float baseVPrime = 2.0f * vCoord - 1.0f;

            // ****************
            // Red Channel
            // ****************
            u[0] = (u[0] * 2.f) - 1.f;
            v[0] = (v[0] * 2.f) - 1.f;

            if (baseUPrime != 0.0f)
                uvRatioU = u[0] / baseUPrime;

            if (baseVPrime != 0.0f)
                uvRatioV = v[0] / baseVPrime;

            // Start with screen coordinates...
            thisPos = glm::vec4(xPos, yPos, zPos, 1.0f);
            screenPos = thisProjMtx * thisPos;

            // ... adjust for warp coords (Kind of flattens out since ignores Z, but not noticable)
            screenPos.x *= uvRatioU;
            screenPos.y *= uvRatioV;

            // ... then reproject back the new position
            thisPos = glm::vec4(screenPos.x, screenPos.y, screenPos.z, screenPos.w);
            warpWorldPos = thisInvProjMtx * thisPos;

            vertices[warpIndex].TexCoordR[0] = warpWorldPos.x;
            vertices[warpIndex].TexCoordR[1] = warpWorldPos.y;
            vertices[warpIndex].TexCoordR[2] = warpWorldPos.z;
            vertices[warpIndex].TexCoordR[3] = 1.f;

            // ****************
            // Green Channel
            // ****************
            u[1] = (u[1] * 2.f) - 1.f;
            v[1] = (v[1] * 2.f) - 1.f;

            if (baseUPrime != 0.0f)
                uvRatioU = u[1] / baseUPrime;

            if (baseVPrime != 0.0f)
                uvRatioV = v[1] / baseVPrime;

            // Start with screen coordinates...
            thisPos = glm::vec4(xPos, yPos, zPos, 1.0f);
            screenPos = thisProjMtx * thisPos;

            // ... adjust for warp coords (Kind of flattens out since ignores Z, but not noticable)
            screenPos.x *= uvRatioU;
            screenPos.y *= uvRatioV;

            // ... then reproject back the new position
            thisPos = glm::vec4(screenPos.x, screenPos.y, screenPos.z, screenPos.w);
            warpWorldPos = thisInvProjMtx * thisPos;

            vertices[warpIndex].TexCoordG[0] = warpWorldPos.x;
            vertices[warpIndex].TexCoordG[1] = warpWorldPos.y;
            vertices[warpIndex].TexCoordG[2] = warpWorldPos.z;
            vertices[warpIndex].TexCoordG[3] = 1.f;

            // ****************
            // Green Channel
            // ****************
            u[2] = (u[2] * 2.f) - 1.f;
            v[2] = (v[2] * 2.f) - 1.f;

            if (baseUPrime != 0.0f)
                uvRatioU = u[2] / baseUPrime;

            if (baseVPrime != 0.0f)
                uvRatioV = v[2] / baseVPrime;

            // Start with screen coordinates...
            thisPos = glm::vec4(xPos, yPos, zPos, 1.0f);
            screenPos = thisProjMtx * thisPos;

            // ... adjust for warp coords (Kind of flattens out since ignores Z, but not noticable)
            screenPos.x *= uvRatioU;
            screenPos.y *= uvRatioV;

            // ... then reproject back the new position
            thisPos = glm::vec4(screenPos.x, screenPos.y, screenPos.z, screenPos.w);
            warpWorldPos = thisInvProjMtx * thisPos;

            vertices[warpIndex].TexCoordB[0] = warpWorldPos.x;
            vertices[warpIndex].TexCoordB[1] = warpWorldPos.y;
            vertices[warpIndex].TexCoordB[2] = warpWorldPos.z;
            vertices[warpIndex].TexCoordB[3] = 1.f;

            warpIndex++;
        }
    }

    // Index buffer time
    std::vector<uint32_t> indices;
    uint32_t inner = 0;
    for (uint32_t whichX = beginRow; whichX < endRow; ++whichX)
    {
        for (uint32_t whichY = 0; whichY < mNumRows; ++whichY)
        {
            uint32_t a = inner;
            uint32_t b = inner + 1;
            uint32_t c = b + (mNumColumns + 1);
            uint32_t d = a + (mNumColumns + 1);

            if (validBuffer[a] &&
                validBuffer[b] &&
                validBuffer[c] &&
                validBuffer[d])
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);

                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);
            }
            inner++;
        }
        inner++;
    }

    // Create geometry
    // If mesh has been created already, need to destroy it first
    if (equirectMesh.GetVertexCount() ||
        equirectMesh.GetIndexCount())
    {
        equirectMesh.Destroy();
    }

    equirectMesh.Initialize(
            &gMeshAttribs[0], NUM_VERT_ATTRIBS,
            &indices[0], indices.size(),
            &vertices[0], vertices.size() * sizeof(warpMeshVert), vertices.size());


    return true;
}

void SxrDistortionMesh::InitQuadLayerMeshes(int32_t const mNumRows, int32_t const mNumColumns)
{
    LOGI("SxrDistortionMesh::InitQuadLayerMeshes()");

    m3dQuadMeshNumRows = mNumRows;
    m3dQuadMeshNumColumns = mNumColumns;

    int32_t const vertexBufferSize = (m3dQuadMeshNumRows+1) * (m3dQuadMeshNumColumns+1);

    m3dQuadMeshVertData.resize(vertexBufferSize);

    std::vector<uint32_t> indices;
    uint32_t inner = 0;
    for (uint32_t whichX = 0; whichX < m3dQuadMeshNumRows; ++whichX)
    {
        for (uint32_t whichY = 0; whichY < m3dQuadMeshNumColumns; ++whichY)
        {
            uint32_t a = inner;
            uint32_t b = inner + 1;
            uint32_t c = b + (m3dQuadMeshNumColumns + 1);
            uint32_t d = a + (m3dQuadMeshNumColumns + 1);

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);

            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);

            inner++;
        }
        inner++;
    }

    for (int32_t i = 0; i < NUM_CACHED_3D_QUAD_MESHES; ++i)
    {
        if (m3dQuadMeshes[i].GetVertexCount() ||
            m3dQuadMeshes[i].GetIndexCount())
        {
            m3dQuadMeshes[i].Destroy();
        }

        m3dQuadMeshes[i].Initialize(
                &gMeshAttribs[0], NUM_VERT_ATTRIBS,
                &indices[0], indices.size(),
                &m3dQuadMeshVertData[0], m3dQuadMeshVertData.size() * sizeof(warpMeshVert),
                m3dQuadMeshVertData.size());

        m3dQuadMeshesUsageCount[i] = 0u;
    }

    m3dQuadMeshHashToIndex.reserve(NUM_CACHED_3D_QUAD_MESHES);
    m3dQuadMeshHashToHashInput.reserve(NUM_CACHED_3D_QUAD_MESHES);

    m3dQuadMeshHashToIndex.clear();
    m3dQuadMeshHashToHashInput.clear();
}

SvrGeometry* SxrDistortionMesh::UpdateAndGetQuadLayerMesh(
        sxrLayoutCoords& ndcCoords,
        int32_t const whichEye)
{
    if (mBufferWidth == 0 || mBufferHeight == 0)
        return nullptr;

    glm::vec4 ndcLlPos = glm::vec4(ndcCoords.LowerLeftPos[0], ndcCoords.LowerLeftPos[1], ndcCoords.LowerLeftPos[2], ndcCoords.LowerLeftPos[3]);
    glm::vec4 ndcLrPos = glm::vec4(ndcCoords.LowerRightPos[0], ndcCoords.LowerRightPos[1], ndcCoords.LowerRightPos[2], ndcCoords.LowerRightPos[3]);
    glm::vec4 ndcUlPos = glm::vec4(ndcCoords.UpperLeftPos[0], ndcCoords.UpperLeftPos[1], ndcCoords.UpperLeftPos[2], ndcCoords.UpperLeftPos[3]);
    glm::vec4 ndcUrPos = glm::vec4(ndcCoords.UpperRightPos[0], ndcCoords.UpperRightPos[1], ndcCoords.UpperRightPos[2], ndcCoords.UpperRightPos[3]);

    bool xVisible = (
            (ndcLlPos.x > -1.f && ndcLlPos.x < 1.f) ||
            (ndcLrPos.x > -1.f && ndcLrPos.x < 1.f) ||
            (ndcUlPos.x > -1.f && ndcUlPos.x < 1.f) ||
            (ndcUrPos.x > -1.f && ndcUrPos.x < 1.f));

    bool yVisible = (
            (ndcLlPos.y > -1.f && ndcLlPos.y < 1.f) ||
            (ndcLrPos.y > -1.f && ndcLrPos.y < 1.f) ||
            (ndcUlPos.y > -1.f && ndcUlPos.y < 1.f) ||
            (ndcUrPos.y > -1.f && ndcUrPos.y < 1.f));

    bool zVisible = (
            (ndcLlPos.z > -1.f && ndcLlPos.z < 1.f) ||
            (ndcLrPos.z > -1.f && ndcLrPos.z < 1.f) ||
            (ndcUlPos.z > -1.f && ndcUlPos.z < 1.f) ||
            (ndcUrPos.z > -1.f && ndcUrPos.z < 1.f));

    if (!(xVisible && yVisible && zVisible))
        return nullptr;

    // Check if this mesh is already cached
    int32_t hashInputs[NUM_3D_QUAD_MESH_HASH_INPUTS] = {
            static_cast<int32_t>(ndcLlPos.x * 1000.f),
            static_cast<int32_t>(ndcLlPos.y * 1000.f),
            static_cast<int32_t>(ndcLlPos.z * 1000.f),
            static_cast<int32_t>(ndcLrPos.x * 1000.f),
            static_cast<int32_t>(ndcLrPos.y * 1000.f),
            static_cast<int32_t>(ndcLrPos.z * 1000.f),
            static_cast<int32_t>(ndcUlPos.x * 1000.f),
            static_cast<int32_t>(ndcUlPos.y * 1000.f),
            static_cast<int32_t>(ndcUlPos.z * 1000.f),
            static_cast<int32_t>(ndcUrPos.x * 1000.f),
            static_cast<int32_t>(ndcUrPos.y * 1000.f),
            static_cast<int32_t>(ndcUrPos.z * 1000.f),
            whichEye
    };

    int32_t hashOutput = 0;
    for (int32_t input : hashInputs)
    {
        // Based on boost hash combine
        hashOutput ^=
                std::hash<int>{}(input) + 0x9e3779b9 + (hashOutput << 6) + (hashOutput >> 2);
    }

    std::unordered_map<int32_t, int32_t>::iterator hashToIndexIter = m3dQuadMeshHashToIndex.find(
            hashOutput);

    bool collided = false;

    if (hashToIndexIter != m3dQuadMeshHashToIndex.end())
    {
        // Check collision
        std::unordered_map<int32_t, int32_t[NUM_3D_QUAD_MESH_HASH_INPUTS]>::iterator hashToHashInputs = m3dQuadMeshHashToHashInput.find(
                hashOutput);
        if (hashToHashInputs == m3dQuadMeshHashToHashInput.end())
        {
            LOGE("SxrDistortionMesh::UpdateAndGetQuadLayerMesh: No quad mesh hash inputs found!  This is a bug.");
            return nullptr;
        }

        for (int32_t i = 0; i < NUM_3D_QUAD_MESH_HASH_INPUTS; ++i)
        {
            if (hashInputs[i] != hashToHashInputs->second[i])
            {
                collided = true;
                break;
            }
        }

        // No collision? then set the cached mesh as current and return
        if (!collided)
        {
            // Making sure we up the use count on this mesh
            m3dQuadMeshesUsageCount[hashToIndexIter->second]++;

            //LOGE("&m3dQuadMeshes[hashToIndexIter->second]", "Reusing cached quad mesh (hash %d)", hashOutput);

            return &m3dQuadMeshes[hashToIndexIter->second];
        }
    }

    // Note: if layer has custom UVs, they will be handled with UV scale/offsets in the shader
    glm::vec2 llUV = glm::vec2(0.f, 0.f);
    glm::vec2 lrUV = glm::vec2(1.f, 0.f);
    glm::vec2 ulUV = glm::vec2(0.f, 1.f);
    glm::vec2 urUV = glm::vec2(1.f, 1.f);

    float* bufferXyz[3] = {nullptr, nullptr, nullptr};
    float* bufferUv[3] = {nullptr, nullptr, nullptr};

    if (kLeftEye == whichEye)
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            bufferXyz[i] = mLeftBufferXyz[i];
            bufferUv[i] = mLeftBufferUv[i];
        }
    }
    else
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            bufferXyz[i] = mRightBufferXyz[i];
            bufferUv[i] = mRightBufferUv[i];
        }
    }

    uint32_t const numRows = m3dQuadMeshNumRows;
    uint32_t const numCols = m3dQuadMeshNumColumns;

    uint32_t const numVerts = (numRows + 1) * (numCols + 1);

    m3dQuadMeshVertData.clear();
    m3dQuadMeshVertData.resize(numVerts);

    std::vector<bool> validBuffer;
    validBuffer.resize(numVerts);

    int32_t warpIndex = 0;

    // Fill verts
    for (uint32_t whichX = 0; whichX < (numCols + 1); ++whichX)
    {
        for (uint32_t whichY = 0; whichY < (numRows + 1); ++whichY)
        {
            float rowLerp = static_cast<float>(whichY) / static_cast<float>(numRows);
            float colLerp = static_cast<float>(whichX) / static_cast<float>(numCols);

            glm::vec4 tempPosL = glm::lerp(ndcLlPos, ndcUlPos, rowLerp);
            glm::vec4 tempPosR = glm::lerp(ndcLrPos, ndcUrPos, rowLerp);
            glm::vec4 tempPos = glm::lerp(tempPosL, tempPosR, colLerp);

            glm::vec2 tempUVL = glm::lerp(llUV, ulUV, rowLerp);
            glm::vec2 tempUVR = glm::lerp(lrUV, urUV, rowLerp);
            glm::vec2 tempUV = glm::lerp(tempUVL, tempUVR, colLerp);

            float const uCoord = (tempPos.x / 2.f) + 0.5f;
            float const vCoord = (tempPos.y / 2.f) + 0.5f;

            float x, y, z, u[3], v[3];
            float alphaRed, alphaGreen, alphaBlue;

            int32_t indexOffset;
            int32_t sampleU, sampleV;

            sampleU = static_cast<int32_t>((uCoord * static_cast<float>(mBufferWidth)) + 0.5f);
            sampleV = static_cast<int32_t>((vCoord * static_cast<float>(mBufferHeight)) + 0.5f);

            if (sampleU < 0)
                sampleU = 0;
            if (sampleU >= mBufferWidth-1)
                sampleU = mBufferWidth-1;

            if (sampleV < 0)
                sampleV = 0;
            if (sampleV >= mBufferHeight-1)
                sampleV = mBufferHeight-1;

            //X
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            x = bufferXyz[0][indexOffset];

            //Y
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            y = bufferXyz[0][indexOffset];

            //Z
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 2;
            z = bufferXyz[0][indexOffset];

            // ALPHA RED - The alpha value from the RED mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaRed = bufferXyz[0][indexOffset];

            // ALPHA GREEN - The alpha value from the GREEN mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaGreen = bufferXyz[1][indexOffset];

            // ALPHA BLUE - The alpha value from the BLUE mesh render
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            alphaBlue = bufferXyz[2][indexOffset];

            bool alpha = false;
            if (alphaRed >= 0.99f && alphaGreen >= 0.99f && alphaBlue >= 0.99f)
                alpha = true;

            //U
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            u[0] = bufferUv[0][indexOffset];
            u[1] = bufferUv[1][indexOffset];
            u[2] = bufferUv[2][indexOffset];

            //V
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            v[0] = bufferUv[0][indexOffset];
            v[1] = bufferUv[1][indexOffset];
            v[2] = bufferUv[2][indexOffset];

            // Also ensure proper UVs
            bool uvsValid = true;
            if ((u[0] < 0.f || u[0] > 1.f) ||
                (u[1] < 0.f || u[1] > 1.f) ||
                (u[2] < 0.f || u[2] > 1.f) ||
                (v[0] < 0.f || v[0] > 1.f) ||
                (v[1] < 0.f || v[1] > 1.f) ||
                (v[2] < 0.f || v[2] > 1.f))
            {
                uvsValid = false;
            }

            bool isValid = false;

            /*
            * Check to see if value is on a mesh
            */
            if (alpha && uvsValid)
            {
                isValid = true;
            }
            else
            {
                isValid = BorderCheckFast(uCoord,
                                        vCoord,
                                        bufferXyz,
                                        bufferUv,
                                        x, y, z,
                                        u[0], v[0],
                                        u[1], v[1],
                                        u[2], v[2],
                                        (mBufferHeight / m3dQuadMeshNumRows) / 2,
                                        m3dQuadMeshNumRows);
            }

            validBuffer[warpIndex] = isValid;

            m3dQuadMeshVertData[warpIndex].Position[0] = x;
            m3dQuadMeshVertData[warpIndex].Position[1] = y;
            m3dQuadMeshVertData[warpIndex].Position[2] = tempPos.z;
            m3dQuadMeshVertData[warpIndex].Position[3] = 1.f;


            if (kLeftEye == whichEye)
                m3dQuadMeshVertData[warpIndex].Position[0] = (m3dQuadMeshVertData[warpIndex].Position[0] / 2.f) - 0.5f;
            else
                m3dQuadMeshVertData[warpIndex].Position[0] = (m3dQuadMeshVertData[warpIndex].Position[0] / 2.f) + 0.5f;

            // ****************
            // Red Channel
            // ****************
            float uOffset = u[0] - ((x / 2.f) + 0.5f);
            float vOffset = v[0] - ((y / 2.f) + 0.5f);

            m3dQuadMeshVertData[warpIndex].TexCoordR[0] = (2.f * tempUV.x - 1.f) + uOffset;
            m3dQuadMeshVertData[warpIndex].TexCoordR[1] = (2.f * tempUV.y - 1.f) + vOffset;

            // ****************
            // Green Channel
            // ****************
            uOffset = u[1] - ((x / 2.f) + 0.5f);
            vOffset = v[1] - ((y / 2.f) + 0.5f);

            m3dQuadMeshVertData[warpIndex].TexCoordG[0] = (2.f * tempUV.x - 1.f) + uOffset;
            m3dQuadMeshVertData[warpIndex].TexCoordG[1] = (2.f * tempUV.y - 1.f) + vOffset;

            // ****************
            // Blue Channel
            // ****************
            uOffset = u[2] - ((x / 2.f) + 0.5f);
            vOffset = v[2] - ((y / 2.f) + 0.5f);

            m3dQuadMeshVertData[warpIndex].TexCoordB[0] = (2.f * tempUV.x - 1.f) + uOffset;
            m3dQuadMeshVertData[warpIndex].TexCoordB[1] = (2.f * tempUV.y - 1.f) + vOffset;

            warpIndex++;
        }
    }

    // Index buffer time
    std::vector<uint32_t> indices;
    uint32_t inner = 0;
    for (uint32_t whichX = 0; whichX < numCols; ++whichX)
    {
        for (uint32_t whichY = 0; whichY < numRows; ++whichY)
        {
            uint32_t a = inner;
            uint32_t b = inner + 1;
            uint32_t c = b + (numCols + 1);
            uint32_t d = a + (numCols + 1);

            if (validBuffer[a] &&
                validBuffer[b] &&
                validBuffer[c] &&
                validBuffer[d])
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);

                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);
            }
            inner++;
        }
        inner++;
    }

    // Will we need to evict a cache entry?
    bool evictNeeded = (m3dQuadMeshHashToIndex.size() == NUM_CACHED_3D_QUAD_MESHES);

    int32_t targetIndex = 0;

    // If there was no collision we need to find least used cached mesh.
    // TODO: make this more efficient if cache size set increases.  Should be ok for the time being as it's quite small to traverse
    if (!collided)
    {
        uint32_t usageCount = UINT_MAX;
        for (int32_t i = 0; i < NUM_CACHED_3D_QUAD_MESHES; ++i)
        {
            if (0 == m3dQuadMeshesUsageCount[i])
            {
                targetIndex = i;

                // Sanity check log.  If a count is 0, then evict should be false
                if (evictNeeded)
                {
                    assert(0);
                    LOGE("SxrDistortionMesh::UpdateAndGetQuadLayerMesh: Cache eviction is true, but usage is 0 at index %d.  This is a bug.", i);
                }

                break;
            }

            if (m3dQuadMeshesUsageCount[i] < usageCount)
            {
                usageCount = m3dQuadMeshesUsageCount[i];
                targetIndex = i;
            }
        }
    }
    else
    {
        // Otherwise we use the same index
        targetIndex = hashToIndexIter->second;
    }


    // Update geometry
    m3dQuadMeshes[targetIndex].Update(
            &m3dQuadMeshVertData[0], m3dQuadMeshVertData.size() * sizeof(warpMeshVert), m3dQuadMeshVertData.size(),
            &indices[0], indices.size());

    // If no collision, update caches
    if (!collided)
    {
        // If we need to evict, do so now
        if (evictNeeded)
        {
            //LOGE("AndroidXrCompositor::Update3dQuadLayerMesh", "Evicted %d", targetIndex);
            m3dQuadMeshHashToIndex.erase(m3dQuadIndexToHash[targetIndex]);
            m3dQuadMeshHashToHashInput.erase(m3dQuadIndexToHash[targetIndex]);
        }

        //LOGE("AndroidXrCompositor::Update3dQuadLayerMesh", "Updated cache %d", targetIndex);
        m3dQuadMeshHashToIndex[hashOutput] = targetIndex;
        for (int32_t i = 0; i < NUM_3D_QUAD_MESH_HASH_INPUTS; ++i)
            m3dQuadMeshHashToHashInput[hashOutput][i] = hashInputs[i];
        m3dQuadIndexToHash[targetIndex] = hashOutput;
    }

    m3dQuadMeshesUsageCount[targetIndex] = 1;

    return &m3dQuadMeshes[targetIndex];
}

bool SxrDistortionMesh::BorderCheck(
        float const uCoord,
        float const vCoord,
        float* bufferXyz[3],
        float* bufferUv[3],
        float& xOut,
        float& yOut,
        float& zOut,
        float& uRedOut,
        float& vRedOut,
        float& uGreenOut,
        float& vGreenOut,
        float& uBlueOut,
        float& vBlueOut,
        int32_t const mNumRows)
{
    float const uIndexFloat = uCoord * static_cast<float>(mBufferWidth);
    float const vIndexFloat = vCoord * static_cast<float>(mBufferHeight);

    int32_t radius = mBufferHeight / mNumRows;
    radius = radius + (radius / 2);

    // check right/up/left/down first, then the 45 degrees diagonals next.
    // 0.707 is sqrt(0.5), the x,y for diagonal radius of 1.0
    float const axisX[8] = {1.f, 0.f, -1.f, 0.f, 0.707f, -0.707f, -0.707f, 0.707f};
    float const axisY[8] = {0.f, 1.f, 0.f, -1.f, 0.707f,  0.707f, -0.707f, -0.707f};

    for (int32_t i=1; i <= radius; ++i)
    {
        for (int32_t direction = 0; direction < 8; ++direction)
        {
            /*
                * Check left/right/down/up directions
                */
            int32_t sampleU = static_cast<int32_t>((uIndexFloat + (static_cast<float>(i) * axisX[direction])) + 0.5f);
            int32_t sampleV = static_cast<int32_t>((vIndexFloat + (static_cast<float>(i) * axisY[direction])) + 0.5f);

            if (sampleU < 0)
                sampleU = 0;
            if (sampleU >= mBufferWidth-1)
                sampleU = mBufferWidth-1;

            if (sampleV < 0)
                sampleV = 0;
            if (sampleV >= mBufferHeight-1)
                sampleV = mBufferHeight-1;


            int32_t indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
            float alphaRed = bufferXyz[0][indexOffset];
            float alphaGreen = bufferXyz[1][indexOffset];
            float alphaBlue = bufferXyz[2][indexOffset];

            float u[3], v[3];
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            u[0] = bufferUv[0][indexOffset];
            u[1] = bufferUv[1][indexOffset];
            u[2] = bufferUv[2][indexOffset];

            //V
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            v[0] = bufferUv[0][indexOffset];
            v[1] = bufferUv[1][indexOffset];
            v[2] = bufferUv[2][indexOffset];

            // Also ensure proper UVs
            bool uvsValid = true;
            if ((u[0] < 0.f || u[0] > 1.f) ||
                (u[1] < 0.f || u[1] > 1.f) ||
                (u[2] < 0.f || u[2] > 1.f) ||
                (v[0] < 0.f || v[0] > 1.f) ||
                (v[1] < 0.f || v[1] > 1.f) ||
                (v[2] < 0.f || v[2] > 1.f))
                uvsValid = false;

            if (alphaRed >= 0.99f && alphaGreen >= 0.99f && alphaBlue >= 0.99f && uvsValid)
            {
                //X
                indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
                xOut = bufferXyz[0][indexOffset];

                //Y
                indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
                yOut = bufferXyz[0][indexOffset];

                //Z
                indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 2;
                zOut = bufferXyz[0][indexOffset];

                //U
                uRedOut = u[0];
                uGreenOut = u[1];
                uBlueOut = u[2];

                //V
                vRedOut = v[0];
                vGreenOut = v[1];
                vBlueOut = v[2];

                return true;
            }
        }
    }

    return false;
}

bool SxrDistortionMesh::BorderCheckFast(
        float const uCoord,
        float const vCoord,
        float* bufferXyz[3],
        float* bufferUv[3],
        float& xOut,
        float& yOut,
        float& zOut,
        float& uRedOut,
        float& vRedOut,
        float& uGreenOut,
        float& vGreenOut,
        float& uBlueOut,
        float& vBlueOut,
        int32_t const radius,
        uint32_t const sweepIncrement)
{
    float const uIndexFloat = uCoord * static_cast<float>(mBufferWidth);
    float const vIndexFloat = vCoord * static_cast<float>(mBufferHeight);

    float const absCheckU = fabsf(uCoord * 2.f - 1.f);
    float const absCheckV = fabsf(vCoord * 2.f - 1.f);
    bool const checkDiag = (absCheckU - absCheckV) < 0.5f;

    // Only check 1 direction
    float axisX;
    float axisY;
    if (checkDiag)
    {
        axisX = (uCoord > 0.5) ? -0.707f : 0.707f;
        axisY = (vCoord > 0.5) ? -0.707f : 0.707f;
    }
    else if (absCheckU > absCheckV)
    {
        axisX = (uCoord > 0.5) ? -1.f : 1.f;
        axisY = 0.f;
    }
    else
    {
        axisY = (vCoord > 0.5) ? -1.f : 1.f;
        axisX = 0.f;
    }


    for (int32_t i=1; i <= radius; i+=sweepIncrement)
    {
        int32_t sampleU = static_cast<int32_t>((uIndexFloat + (static_cast<float>(i) * axisX)) + 0.5f);
        int32_t sampleV = static_cast<int32_t>((vIndexFloat + (static_cast<float>(i) * axisY)) + 0.5f);

        if (sampleU < 0)
            sampleU = 0;
        if (sampleU >= mBufferWidth-1)
            sampleU = mBufferWidth-1;

        if (sampleV < 0)
            sampleV = 0;
        if (sampleV >= mBufferHeight-1)
            sampleV = mBufferHeight-1;


        int32_t indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 3;
        float alphaRed = bufferXyz[0][indexOffset];
        float alphaGreen = bufferXyz[1][indexOffset];
        float alphaBlue = bufferXyz[2][indexOffset];

        float u[3], v[3];
        indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
        u[0] = bufferUv[0][indexOffset];
        u[1] = bufferUv[1][indexOffset];
        u[2] = bufferUv[2][indexOffset];

        //V
        indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
        v[0] = bufferUv[0][indexOffset];
        v[1] = bufferUv[1][indexOffset];
        v[2] = bufferUv[2][indexOffset];

        // Also ensure proper UVs
        bool uvsValid = true;
        if ((u[0] < 0.f || u[0] > 1.f) ||
            (u[1] < 0.f || u[1] > 1.f) ||
            (u[2] < 0.f || u[2] > 1.f) ||
            (v[0] < 0.f || v[0] > 1.f) ||
            (v[1] < 0.f || v[1] > 1.f) ||
            (v[2] < 0.f || v[2] > 1.f))
            uvsValid = false;

        if (alphaRed >= 0.99f && alphaGreen >= 0.99f && alphaBlue >= 0.99f && uvsValid)
        {
            //X
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 0;
            xOut = bufferXyz[0][indexOffset];

            //Y
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 1;
            yOut = bufferXyz[0][indexOffset];

            //Z
            indexOffset = (sampleV * mBufferWidth * 4) + (sampleU * 4) + 2;
            zOut = bufferXyz[0][indexOffset];

            //U
            uRedOut = u[0];
            uGreenOut = u[1];
            uBlueOut = u[2];

            //V
            vRedOut = v[0];
            vGreenOut = v[1];
            vBlueOut = v[2];

            return true;
        }
    }

    return false;
}
