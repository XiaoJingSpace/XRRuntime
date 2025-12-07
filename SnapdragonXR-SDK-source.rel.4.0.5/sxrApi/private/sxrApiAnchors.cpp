/**********************************************************
 * FILE: sxrApiAnchors.cpp
 *
 * Copyright (c) 2020 QUALCOMM Technologies Inc.
 **********************************************************/
#include "sxrApiAnchors.h"

#include "svrAnchorsCore.h"
#include "svrApiCore.h"
#include "svrUtil.h"

SvrAnchorsCore gSvrAnchorsCore;

SxrResult sxrAnchorsInitialize()
{
    LOGI("sxrAnchorsInitialize");

    if (Svr::gAppContext == nullptr)
    {
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    return gSvrAnchorsCore.Initialize(Svr::gAppContext->qvrHelper);
}

SxrResult sxrAnchorsShutdown()
{
    LOGI("sxrAnchorsShutdown");

    if (Svr::gAppContext == nullptr)
    {
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    return gSvrAnchorsCore.Shutdown(Svr::gAppContext->qvrHelper);
}

const char* sxrAnchorsToString(sxrAnchorUuid& anchorId)
{
    return gSvrAnchorsCore.ToString(anchorId).c_str();
}

SxrResult sxrAnchorsCreate(sxrAnchorPose *anchorPose, sxrAnchorUuid *anchorId)
{
    return gSvrAnchorsCore.Create(anchorPose, anchorId);
}

SxrResult sxrAnchorsDestroy(sxrAnchorUuid& anchorId)
{
    return gSvrAnchorsCore.Destroy(anchorId);
}

SxrResult sxrAnchorsGetData(uint32_t *numAnchors, sxrAnchorInfo **anchors)
{
    return gSvrAnchorsCore.GetData(numAnchors, anchors);
}

SxrResult sxrAnchorsReleaseData()
{
    return gSvrAnchorsCore.ReleaseData();
}

sxrAnchorPose sxrAnchorsGetPose(sxrAnchorPose anchor_pose, sxrMatrix conversion)
{
    return gSvrAnchorsCore.GetPose(anchor_pose, conversion);
}

SxrResult sxrAnchorsSave(sxrAnchorUuid& anchorId, const char* fmapFolder)
{
    return gSvrAnchorsCore.SaveAnchor(anchorId, fmapFolder);
}

SxrResult sxrAnchorsStartRelocating(const char* fmapFolder)
{
    return gSvrAnchorsCore.StartRelocating(fmapFolder);
}

SxrResult sxrAnchorsStopRelocating()
{
    return gSvrAnchorsCore.StopRelocating();
}
