//=============================================================================
// FILE: svrGeometry.h
//
//                  Copyright (c) 2015 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
#pragma once

#define MAX_ATTRIBUTES 8

namespace Svr
{
    struct SvrProgramAttribute
    {
        unsigned int    index;
        int             size;
        unsigned int    type;
        bool            normalized;
        int             stride;
		int				offset;
    };

    class SvrGeometry
    {
    public:
        SvrGeometry();
        ~SvrGeometry();

        void Initialize(SvrProgramAttribute* pAttribs, int nAttribs,
                        unsigned int* pIndices, int nIndices,
                        const void* pVertexData, int bufferSize, int nVertices,
						bool doCopy = false);

        void Update(const void* pVertexData, int bufferSize, int nVertices);

        void Update(const void* pVertexData, int bufferSize, int nVertices, unsigned int* pIndices, int nIndices);

        void Destroy();
        void Submit();
        void Submit(SvrProgramAttribute* pAttribs, int nAttribs);
        void SubmitPoints();
        void SubmitLines();

        static void CreateFromObjFile(const char* pObjFilePath, SvrGeometry** pOutGeometry, int& outNumGeometry);
		void SaveOBJ(char *pFileName);

        unsigned int GetVbId() { return mVbId; }
        unsigned int GetIbId() { return mIbId; }
        unsigned int GetVaoId() { return mVaoId; }
        int GetVertexCount() { return mVertexCount; }
        int GetIndexCount() { return mIndexCount; }

		void InitializeWireframe(SvrGeometry * pInput);


    private:
        unsigned int    mVbId;
        unsigned int    mIbId;
        unsigned int    mVaoId;
        int             mVertexCount;
        int             mIndexCount;
        unsigned int    *mpIndexBuffer;
		int             mNumAttributes;
		SvrProgramAttribute *mpAttributes;
        int             mVertexBufferSize;
        void            *mpVertexBuffer;
    };

    class SvrPoints
    {
    public:
        SvrPoints();

        void Initialize(SvrProgramAttribute* pAttribs, int nAttribs,
            const void* pVertexData, int bufferSize, int nVertices);

        void Update(const void* pVertexData, int bufferSize, int nVertices);

        void Destroy();
        void Submit();
        void Submit(SvrProgramAttribute* pAttribs, int nAttribs);

        static void CreateFromObjFile(const char* pObjFilePath, SvrPoints** pOutGeometry, int& outNumGeometry);

        unsigned int GetVbId() { return mVbId; }
        int GetVertexCount() { return mVertexCount; }

    private:
        unsigned int    mVbId;
        int             mVertexCount;
    };
}
