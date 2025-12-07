#define  ENABLE_CUSTOM_MESH 1
#ifdef ENABLE_CUSTOM_MESH
//=============================================================================
// FILE: svrCustomMesh.h
// Copyright (c) 2020 QUALCOMM Technologies Inc.
// All Rights Reserved. Some Lefts too.
//==============================================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "private/svrApiTimeWarp.h"
#include "svrGeometry.h"

using namespace Svr;
using namespace std;

/**********************************************************
 * SvrCustomMesh 
 **********************************************************/
class SvrCustomMesh
{
public:
    SvrCustomMesh();
    ~SvrCustomMesh();

    SxrResult InitializeCustomWarpMesh(SvrGeometry& warpGeom, svrWarpMeshArea whichMesh, unsigned int numRows, unsigned int numCols);

    SxrResult ResampleCustomWarpMesh(SvrGeometry& warpGeom, 
                                     svrWarpMeshArea whichMesh, 
                                     unsigned int numRows, 
                                     unsigned int numCols);

    SxrResult ReleaseBuffers(bool doFree);

    float * m_pBufferXYZ[3];
    float * m_pBufferUV[3];

private:
    void InitializeShaderFromBuffer(Svr::SvrShader &whichShader,
                                    char *pVertexSource,
                                    char *pFragmentSource,
                                    const char *vertexName,
                                    const char *fragmentName);

    void ResampleMesh(struct warpMeshVert  *warpVertexData, 
                      unsigned int *pIndexBuffer,
                      int * indexBufferCount, 
                      int numRows, 
                      int numColumns, 
                      bool doXYZ,
                      int channel);

    bool BorderCheck(float uCoord, 
                     float vCoord, 
                     float *pBufferXYZ,
                     float *pBufferUV,
                     float *pX, 
                     float *pY,
                     float *pZ,
                     float *pU, 
                     float *pV);

    int ParseLine(std::vector<std::string> &lineData, char **ppCurrent);

    void MakeCustomMeshes(SvrGeometry meshArray[3]);

    SxrResult PreprocessCSV();


    int m_beginRow, m_endRow;
    char * m_pBufferCSV;
    char * m_pEnd;
};

#endif
