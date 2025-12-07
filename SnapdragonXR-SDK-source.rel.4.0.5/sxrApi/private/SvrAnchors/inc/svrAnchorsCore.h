/**********************************************************
 * FILE: svrAnchorsCore.h
 *
 * Copyright (c) 2020 QUALCOMM Technologies Inc.
 **********************************************************/
#ifndef _SVR_ANCHORS_CORE_H_
#define _SVR_ANCHORS_CORE_H_

#include <array>
#include <vector>
#include <string>
#include <glm/mat4x4.hpp>

#include <beta/QVRAnchors.h>
#include <QVRServiceClient.h>

#include "sxrApiAnchors.h"

/*!
 * SvrAnchorsCore handles the QVRAnchors function calls, processing input/output data to appropriate
 * coordinate space. Please be noted that all the input and output of all functions are in
 * right-handed coordinate space.
 */
class SvrAnchorsCore {
public:
    /*!
     * Constructor.
     */
    SvrAnchorsCore();

    /*!
     * Destructor.
     */
    virtual ~SvrAnchorsCore();

    /*!
     * Initialize Anchors module.
     *
     * \param[in] helper qvrservice_client_helper_t handle.
     * \return
     *    - #SXR_ERROR_NONE upon success.
     *    - #SXR_ERROR_VRMODE_NOT_INITIALIZED if helper is null.
     *    - #SXR_ERROR_UNSUPPORTED if Anchors is not supported.
     */
    SxrResult Initialize(qvrservice_client_helper_t *helper);

    /*!
     * Shutdown Anchors module.
     *
     * \param[in] helper qvrservice_client_helper_t handle.
     * \return
     *    - #SXR_ERROR_NONE upon success.
     *    - #SXR_ERROR_VRMODE_NOT_INITIALIZED if helper is null.
     *    - #SXR_ERROR_NOT_INITIALIZED if Anchors is not initialized.
     */
    SxrResult Shutdown(qvrservice_client_helper_t *helper);

    /*!
    * Create an anchor pose. The caller may call #Destroy to remove the anchor.
    *
    * \param[in] anchor_pose.
    * \param[out] anchor id.
    * \return
    *    - #SXR_ERROR_NONE upon success.
    *    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
    *    - #SXR_ERROR_INVALID_OPERATION if error occurs.
    */
    SxrResult Create(sxrAnchorPose *anchor_pose, sxrAnchorUuid *anchorId);

    /*!
    * Destroy anchor pose previously created via #Create.
    *
    * \param[in] anchor id.
    * \return
    *    - #SXR_ERROR_NONE upon success.
    *    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
    *    - #SXR_ERROR_INVALID_OPERATION the anchor was not destroyed.
    */
    SxrResult Destroy(sxrAnchorUuid& anchorId);

    /*!
    * Get anchors data. The caller must call #ReleaseData if the return code was
    * #SXR_SUCCESS to unlock the anchors data.
    *
    * \param[in, out] numAnchors Number of anchors.
    * \param[in, out] anchors Pointer that points to the anchors data.Caller doesn't take the
    *                         ownership of the data.
    * \return
    *    - #SXR_ERROR_NONE upon success.
    *    - #SXR_3DR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
    *    - #SXR_3DR_ERROR_INVALID_OPERATION if error occurs.
    */
    SxrResult GetData(uint32_t *numAnchors, sxrAnchorInfo **anchors);

    /*!
     * Release anchors data which previously locked via #GetData.
     *
     * \return
     *    - #SXR_ERROR_NONE upon success.
     *    - #SXR_ERROR_NOT_INITIALIZED if 3dr is not initialized.
     *    - #SXR_ERROR_INVALID_OPERATION the mesh was not locked.
     */
    SxrResult ReleaseData();

    sxrAnchorPose GetPose(sxrAnchorPose, sxrMatrix);

    std::string ToString(sxrAnchorUuid& anchorId);

    /*!
    * Saves the anchor into /sdcard/anchors folder with uuid as the filename.
    *
    * \param[in] anchor id.
    * \param[in] file path.
    * \return
    *    - #SXR_ERROR_NONE upon success.
    *    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
    *    - #SXR_ERROR_INVALID_OPERATION if error occurs.
    */
    SxrResult SaveAnchor(sxrAnchorUuid& anchorId, const char* fmapFolder);

    /*!
    * Loads all files in /sdcard/anchors into the relocator.
    * Assumption that each file in there is an anchor and name is the anchor uuid
    *
    * \param[in] file path.
    * \return
    *    - #SXR_ERROR_NONE upon success.
    *    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
    *    - #SXR_ERROR_INVALID_OPERATION if error occurs.
    */
    SxrResult StartRelocating(const char* fmapFolder);

    /*!
    * Unloads all the anchors loaded in the startrelocating call
    *
    * \return
    *    - #SXR_ERROR_NONE upon success.
    *    - #SXR_ERROR_NOT_INITIALIZED if service is not initialized.
    *    - #SXR_ERROR_INVALID_OPERATION if error occurs.
    */
    SxrResult StopRelocating();

private:
    //struct Transform {
    //    bool ready;
    //    glm::mat4 qvrToSxrMatrix;
    //    glm::mat4 sxrToQvrMatrix;

    //    Transform() : ready(false), qvrToSxrMatrix(1.0f), sxrToQvrMatrix(1.0f)
    //    {
    //    }
    //};

    //void GetTransformMatrixIfNeeded();

    sxrAnchorPose Convert(const sxrAnchorPose &src, const sxrMatrix &conversionMatrix);

    qvrservice_class_t *mAnchorsHandle;

    XrAnchorDataQTI *mAnchors;

    std::vector<XrAnchorUuidQTI> mAnchorUuids;
    std::vector<std::string> mAnchorDirs;
    std::vector<int> mAnchorFds;
    std::vector<uint32_t> mAnchorSizes;

    //Transform mTransform;

    static constexpr unsigned int uuidStringLength = 36;
    static constexpr unsigned int uuidValLength = 16;

    /// utility function to convet uint8_t[16] uuid value to string representation
    static std::string uuidValToString(const uint8_t* data);

    /// utility function to convet char[36] uuid string representation to uint8_t[16] value
    static std::array<uint8_t, uuidValLength> uuidStringToVal(const char* uuidString);

};

#endif // _SVR_ANCHORS_CORE_H_
