#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "svrGeometry.h"
#include "svrApiTimeWarp.h"

using namespace Svr;
using namespace std;

class SxrDistortionMesh
{
public:
    bool Initialize(
            glm::vec2 const& leftMeshOffsets,
            glm::vec2 const& rightMeshOffsets,
            uint32_t const surfaceWidth,
            uint32_t const surfaceHeight,
            sxrDeviceInfo const& displayProperties);

    void Finalize();

    SvrGeometry* GetProjectionLayerMesh(int32_t const whichEye)
    {
        return &mProjectionMeshes[whichEye];
    }

    SvrGeometry* GetEquirectLayerMesh(int32_t const whichEye)
    {
        return &mEquirectMeshes[whichEye];
    }

    bool CreateProjectionLayerMesh(SvrGeometry& projectionMesh, int32_t const whichEye, int32_t const whichMesh, int32_t const mNumRows, int32_t const mNumColumns);
    bool CreateEquirectLayerMesh(SvrGeometry& equireectMesh, int32_t const whichEye, int32_t const whichMesh, int32_t const mNumRows, int32_t const mNumColumns, sxrDeviceInfo const& displayProperties);

    void InitQuadLayerMeshes(int32_t const mNumRows, int32_t const mNumColumns);
    SvrGeometry* UpdateAndGetQuadLayerMesh(
            sxrLayoutCoords& imageCoords,
            int32_t const whichEye);

private:
    bool MakeCustomMeshes(char* pBufferCSV, SvrGeometry meshArray[3]); // From SvrCustomMesh
    //bool CreateCustomMeshes(SvrDistortionGridData const& grid, SvrGeometry meshArray[3]);

    bool BorderCheck(
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
            int32_t const mNumRows);

    bool BorderCheckFast(
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
            uint32_t const sweepIncrement = 2);

    void FreeLookupTables()
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            if (mLeftBufferXyz[i])
            {
                delete[] mLeftBufferXyz[i];
                mLeftBufferXyz[i] = nullptr;
            }
            if (mLeftBufferUv[i])
            {
                delete[] mLeftBufferUv[i];
                mLeftBufferUv[i] = nullptr;
            }

            if (mRightBufferXyz[i])
            {
                delete[] mRightBufferXyz[i];
                mRightBufferXyz[i] = nullptr;
            }
            if (mRightBufferUv[i])
            {
                delete[] mRightBufferUv[i];
                mRightBufferUv[i] = nullptr;
            }
        }
    }

    int ParseLine(std::vector<std::string> &lineData, char **ppCurrent);

    // TODO: don't hard code these once everything works //////////////
    float const mEquirectRad = 1.f;
    ///////////////////////////////////////////////////////////////////

    // Custom meshes from csv data (3 of them for RGB)
    // These are temporary and get init'ed/cleaned up in Initialize(...)
    //SvrGeometry mCustomMesh[3];

    // Lookup tables (3 of them for RGB)
    float* mLeftBufferXyz[3] = {nullptr, nullptr, nullptr};
    float* mLeftBufferUv[3] = {nullptr, nullptr, nullptr};
    float* mRightBufferXyz[3] = {nullptr, nullptr, nullptr};
    float* mRightBufferUv[3] = {nullptr, nullptr, nullptr};

    // Their dimensions (should be 1/2 surface width and full surface height)
    uint32_t mBufferWidth = 0;
    uint32_t mBufferHeight = 0;

    // Finalized meshes
    SvrGeometry mProjectionMeshes[kNumEyes];
    SvrGeometry mEquirectMeshes[kNumEyes];

    // Quad meshes are cached so we don't recreate them if not needed
    enum
    {
        NUM_CACHED_3D_QUAD_MESHES = SXR_MAX_RENDER_LAYERS * 2,
        NUM_3D_QUAD_MESH_HASH_INPUTS = 13
    };
    int32_t m3dQuadMeshNumRows = 0;
    int32_t m3dQuadMeshNumColumns = 0;
    std::unordered_map<int32_t, int32_t> m3dQuadMeshHashToIndex;
    std::unordered_map<int32_t, int32_t[NUM_3D_QUAD_MESH_HASH_INPUTS]> m3dQuadMeshHashToHashInput; // used to test hash collisions
    int32_t m3dQuadIndexToHash[NUM_CACHED_3D_QUAD_MESHES];
    SvrGeometry m3dQuadMeshes[NUM_CACHED_3D_QUAD_MESHES];
    uint32_t m3dQuadMeshesUsageCount[NUM_CACHED_3D_QUAD_MESHES];
    std::vector<warpMeshVert> m3dQuadMeshVertData;
};
