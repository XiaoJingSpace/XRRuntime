#include "svrAnchorsCore.h"

#include <cassert>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "svrApiCore.h"
#include "svrUtil.h"

#include <fstream>
#include <map>
#include <thread>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

SvrAnchorsCore::SvrAnchorsCore() :
        mAnchorsHandle(nullptr),
        mAnchors(nullptr)
{
}

SvrAnchorsCore::~SvrAnchorsCore()
{
    if (mAnchorsHandle)
    {
        LOGW("Anchors module was not shutdown");
    }
}

SxrResult SvrAnchorsCore::Initialize(qvrservice_client_helper_t *helper)
{
    if (mAnchorsHandle)
    {
        return SXR_ERROR_NONE;
    }

    if (!helper)
    {
        return SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    mAnchorsHandle = QVRServiceClient_GetClassHandle(helper, (uint32_t)CLASS_ID_ANCHORS_BETA_2, nullptr);
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    return SXR_ERROR_NONE;
}

SxrResult SvrAnchorsCore::Shutdown(qvrservice_client_helper_t *helper)
{
    SxrResult result = SXR_ERROR_NONE;
    if (helper)
    {
        if (mAnchorsHandle)
        {
            QVRServiceClient_ReleaseClassHandle(helper, mAnchorsHandle);
            mAnchorsHandle = nullptr;
        }
    }
    else
    {
        result = SXR_ERROR_VRMODE_NOT_INITIALIZED;
    }

    return result;
}

SxrResult SvrAnchorsCore::Create(sxrAnchorPose *anchor_pose, sxrAnchorUuid *anchor_id)
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    SxrResult result = SXR_ERROR_NONE;

    assert(sizeof(sxrAnchorPose) == sizeof(XrAnchorPosefQTI));
    int qvrResult = QVRAnchors_Create(mAnchorsHandle, (XrAnchorPosefQTI *)anchor_pose, (XrAnchorUuidQTI *)anchor_id);

    if (qvrResult != QVR_SUCCESS)
    {
        LOGW("QVRAnchors_Create error %s", QVRErrorToString(qvrResult));
        result = SXR_ERROR_UNKNOWN;
        //*anchor_id = 0;
    }

    return result;
}

SxrResult SvrAnchorsCore::Destroy(sxrAnchorUuid& anchorId)
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    SxrResult result = SXR_ERROR_NONE;
    int qvrResult = QVRAnchors_Destroy(mAnchorsHandle, (XrAnchorUuidQTI *)&anchorId);
    if (qvrResult != QVR_SUCCESS)
    {
        LOGW("QVRAnchors_Destroy anchor %s failed: Error %s", uuidValToString(anchorId.uuid).c_str(), QVRErrorToString(qvrResult));
        result = SXR_ERROR_UNKNOWN;
    }

    return result;
}

SxrResult SvrAnchorsCore::GetData(uint32_t *numAnchors, sxrAnchorInfo **anchors)
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    assert(sizeof(sxrAnchorInfo) == sizeof(XrAnchorInfoQTI));

    if (mAnchors)
    {
        // Something wrong here, maybe the user didn't call ReleaseData.
        LOGW("GetData previous data is not released");
        ReleaseData();
    }

    SxrResult result = SXR_ERROR_NONE;

    int qvrResult = QVRAnchors_GetAnchorData(mAnchorsHandle, &mAnchors);
    if (qvrResult == QVR_SUCCESS)
    {
        *numAnchors = mAnchors->numAnchors;
        *anchors = (sxrAnchorInfo *) mAnchors->anchors;
    }
    else
    {
        LOGW("QVRAnchors_GetAnchorData failed: Error %s", QVRErrorToString(qvrResult));

        *numAnchors = 0;
        *anchors = nullptr;

        result = SXR_ERROR_UNKNOWN;
    }

    return result;
}

SxrResult SvrAnchorsCore::ReleaseData()
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    if (!mAnchors)
    {
        LOGW("ReleaseData data does not exist");
        return SXR_ERROR_UNKNOWN;
    }

    SxrResult result = SXR_ERROR_NONE;
    int qvrResult = QVRAnchors_ReleaseAnchorData(mAnchorsHandle, mAnchors);
    if (qvrResult != QVR_SUCCESS)
    {
        LOGE("QVRAnchors_ReleaseAnchorData failed: Error %s", QVRErrorToString(qvrResult));
        result = SXR_ERROR_UNKNOWN;
    }
    mAnchors = nullptr;

    return result;
}

sxrAnchorPose SvrAnchorsCore::GetPose(sxrAnchorPose anchor_pose, sxrMatrix conversion)
{
    return Convert(anchor_pose, conversion);
}

std::string SvrAnchorsCore::ToString(sxrAnchorUuid& anchorId)
{
    return uuidValToString(anchorId.uuid);
}

SxrResult SvrAnchorsCore::SaveAnchor(sxrAnchorUuid& anchorId, const char* fmapFolder)
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }


    char anchorFilename[128];
    sprintf(anchorFilename, "%s/anchors/", fmapFolder);
    LOGI("SaveAnchor foldername: %s", anchorFilename);

    // create the 'anchors' folder if it doesn't exist
    struct stat fmapFolderStat;
    if (stat(anchorFilename, &fmapFolderStat) != 0)
    {
        if (mkdir(anchorFilename, 0755) != 0)
        {
            LOGE("SaveAnchor Error: Failed to create '%s': %s", anchorFilename, strerror(errno));
            return SXR_ERROR_UNKNOWN;
        }
    }

    sprintf(anchorFilename, "%s/anchors/%s", fmapFolder, uuidValToString(anchorId.uuid).c_str());
    LOGI("SaveAnchor filename: %s", anchorFilename);

    int anchorFd = open(anchorFilename, O_CREAT | O_RDWR, 0664);
    if (anchorFd < 0)
    {
        LOGE("SaveAnchor Error: Failed to open '%s': %s", anchorFilename, strerror(errno));
        return SXR_ERROR_UNKNOWN;
    }

    uint32_t anchorSize;
    int qvrResult = QVRAnchors_Save(mAnchorsHandle, (XrAnchorUuidQTI *)&anchorId, anchorFd, &anchorSize);
    if (qvrResult != 0)
    {
        LOGE("QVRAnchors_Save failed: Error %s", QVRErrorToString(qvrResult));
        return SXR_ERROR_UNKNOWN;
    }

    LOGI("SaveAnchor succeeded: %s size: %d", anchorFilename, anchorSize);
    return SXR_ERROR_NONE;
}

SxrResult SvrAnchorsCore::StopRelocating()
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    for (auto anchorId : mAnchorUuids)
    {
        int qvrResult = QVRAnchors_RemoveFromSearch(mAnchorsHandle, &anchorId);
        if (qvrResult < 0)
        {
            LOGW("StopRelocating: Failed to remove anchor %s from qvrservice relocate: %s", uuidValToString(anchorId.uuid).c_str(), QVRErrorToString(qvrResult));
            //return SXR_ERROR_UNKNOWN;
        }
    }
    mAnchorDirs.clear();
    mAnchorFds.clear();
    mAnchorUuids.clear();
    mAnchorSizes.clear();

    LOGI("Anchor Relocating: Stopped ");
    return SXR_ERROR_NONE;
}

int isDirectory(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

SxrResult SvrAnchorsCore::StartRelocating(const char* fmapFolder)
{
    if (!mAnchorsHandle)
    {
        return SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
    }

    char anchorFolder[128];
    sprintf(anchorFolder, "%s/anchors/", fmapFolder);
    LOGI("StartRelocating foldername: %s", anchorFolder);

    // find all folders in fmapfolder and assume they are anchors
    DIR *dir = nullptr;
    struct dirent *ent = nullptr;
    if ((dir = opendir(anchorFolder)) != nullptr)
    {
        while ((ent = readdir(dir)) != nullptr)
        {
            //std::string path = anchorFolder + std::string(ent->d_name);
            std::string path(anchorFolder);
            path.append(std::string(ent->d_name, sizeof(ent->d_name)).c_str());
            if (!isDirectory(path.c_str()))
            {
                int fd = open(path.c_str(), O_RDONLY);
                if (fd < 0)
                {
                    LOGE("StartRelocating Error: failed to open '%s': %s", std::string(path, sizeof(path)).c_str(), strerror(errno));
                    continue;
                }
                uint32_t fileSize = lseek(fd, 0, SEEK_END);
                LOGI("StartRelocating: Anchor file: %s fd: %d size: %d", std::string(path, sizeof(path)).c_str(), fd, fileSize);
                mAnchorFds.push_back(fd);
                mAnchorDirs.push_back(path);
                XrAnchorUuidQTI anchorId;
                std::array<uint8_t, uuidValLength> anchorUuid = uuidStringToVal(ent->d_name);
                memcpy(anchorId.uuid, anchorUuid.data(), anchorUuid.size());
                mAnchorUuids.push_back(anchorId);
                mAnchorSizes.push_back(fileSize);
            }
        }
        closedir(dir);
    }
    else
    {
        LOGE("StartRelocating Error: Could not open folder %s", anchorFolder);
        return SXR_ERROR_UNKNOWN;
    }

    int ret = 0;
    for (size_t i = 0; i < mAnchorDirs.size(); i++)
    {
        LOGI("StartRelocating: Adding anchor: %s fd: %d size: %d", uuidValToString(mAnchorUuids.at(i).uuid).c_str(), mAnchorFds.at(i), mAnchorSizes.at(i));
        ret = QVRAnchors_AddToSearch(mAnchorsHandle, &mAnchorUuids.at(i), mAnchorFds.at(i), mAnchorSizes.at(i));
        if (ret != 0)
        {
            LOGE("StartRelocating Error: Could not add anchor %s to relocation list: %s", uuidValToString(mAnchorUuids.at(i).uuid).c_str(), QVRErrorToString(ret));
            continue;
        }
    }

    LOGI("Anchor Relocating: Started");
    return SXR_ERROR_NONE;
}

//void SvrAnchorsCore::GetTransformMatrixIfNeeded()
//{
//    if (mTransform.ready)
//    {
//        return;
//    }
//
//    mTransform.qvrToSxrMatrix = Svr::gAppContext->modeContext->qvrTransformMat;
//    mTransform.sxrToQvrMatrix = glm::affineInverse(Svr::gAppContext->modeContext->qvrTransformMat);
//
//    mTransform.ready = true;
//}


sxrAnchorPose SvrAnchorsCore::Convert(const sxrAnchorPose &anchor_pose, const sxrMatrix &conversion)
{
    // Transform anchor pose from sxr to qvr or vice-versa
    glm::fquat anchorRot = glm::fquat(anchor_pose.orientation.w, anchor_pose.orientation.x, anchor_pose.orientation.y, anchor_pose.orientation.z);
    glm::vec4 anchorPos = glm::vec4(anchor_pose.position.x, anchor_pose.position.y, anchor_pose.position.z, 1.0f);
    glm::mat4 anchorMat = glm::mat4(anchorRot);
    anchorMat[3] = anchorPos;

    glm::mat4 poseMat = glm::make_mat4(conversion.M) * anchorMat;

    glm::fquat poseOrientation = glm::quat_cast(poseMat);
    glm::vec3 posePosition = glm::vec3(poseMat[3][0], poseMat[3][1], poseMat[3][2]);

    sxrAnchorPose anchorPose;
    anchorPose.orientation = { poseOrientation.x, poseOrientation.y, poseOrientation.z, poseOrientation.w };
    anchorPose.position = { posePosition.x, posePosition.y, posePosition.z };
    anchorPose.poseQuality = anchor_pose.poseQuality;

    return anchorPose;
}

constexpr unsigned int uuidStringLength = 36;
constexpr unsigned int uuidValLength = 16;

/// utility function to convet uint8_t[16] uuid value to string representation
std::string SvrAnchorsCore::uuidValToString(const uint8_t* data)
{
    char strBuf[uuidStringLength + 1];
    snprintf(strBuf, sizeof(strBuf), "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
        data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    return std::string(strBuf);
}

/// utility function to convet char[36] uuid string representation to uint8_t[16] value
std::array<uint8_t, uuidValLength> SvrAnchorsCore::uuidStringToVal(const char* uuidString)
{
    std::array<uint8_t, uuidValLength> data;
    const auto scanned = sscanf(uuidString, "%2hhx%2hhx%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
        data.data(), data.data() + 1, data.data() + 2, data.data() + 3, data.data() + 4, data.data() + 5, data.data() + 6, data.data() + 7,
        data.data() + 8, data.data() + 9, data.data() + 10, data.data() + 11, data.data() + 12, data.data() + 13, data.data() + 14, data.data() + 15);
    assert(scanned == uuidValLength);
    return data;
}

