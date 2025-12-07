//******************************************************************************************************************************
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//******************************************************************************************************************************

struct ME_MotionVector {
    float xMagnitude;
    float yMagnitude;
};

/*******************************************************************************************************************************
*   ME_Init
*
*   @brief
*       Create and initilize motion estimation resources
*   @return
*       true on success, false on failure
*
*******************************************************************************************************************************/
#ifdef __cplusplus
extern "C"
#endif
bool ME_Init(unsigned int  inputWidthArg,     // [IN]  Width of images being provided as input for motion calculation
             unsigned int  inputHeightArg,    // [IN]  Height of images that will be provided as input for motion calculation
             bool          downscaleInputArg, // [IN]  Controls if the input is downscaled before motion estimation is performed
             unsigned int *outputWidthArg,    // [OUT] Width of the vector array that will be produced
             unsigned int *outputHeightArg);  // [OUT] Height of the vector array that will be produced

/*******************************************************************************************************************************
*   ME_GenerateVectors
*
*   @brief
*       Generate motion vectors from frame A to B.
*   @return
*       True on success, false on error.
*
*******************************************************************************************************************************/
bool ME_GenerateVectors(
        GLuint    inputTextureA,         // [IN]  Input frame A
        GLuint    inputTextureB,         // [IN]  Input frame B
        GLfloat   reprojectionMatrix[9], // [IN]  Transformation that will be applied before motion estimation
        GLboolean matrixTarget,          // [IN]  Which texture the matrix is applied to. False is A, true is B.
        GLuint    outputTexture);        // [OUT] RGBA16F texture into which the motion vectors will be stored. (X in R, Y in G)

/*******************************************************************************************************************************
*   ME_GenerateVectors
*
*   @brief
*       Generate motion vectors from frame A to B, without format conversion or reprojection. Input textures are GL_R8 format.
*   @return
*       True on success, false on error.
*
*******************************************************************************************************************************/
#ifdef __cplusplus
extern "C"
#endif
bool ME_GenerateVectors(
        GLuint    inputTextureA,         // [IN]  Input frame A  (GL_R8)
        GLuint    inputTextureB,         // [IN]  Input frame B  (GL_R8)
        GLuint    outputTexture);        // [OUT] RGBA16F texture into which the motion vectors will be stored. (X in R, Y in G)


/*******************************************************************************************************************************
*   ME_GenerateVectors_SeparateLevels
*
*   @brief
*       Generate motion vectors from separate pre-downscaled images.
*   @return
*       True on success, false on error.
*
*******************************************************************************************************************************/
bool ME_GenerateVectors_SeparateLevels(
                 void*           inputDataA,        // [IN]  First input buffer to consume
                 void*           inputDataASmall,   // [IN]  First input buffer to consume
                 void*           inputDataASmaller, // [IN]  First input buffer to consume
                 void*           inputDataB,        // [IN]  Second input buffer to consume
                 void*           inputDataBSmall,   // [IN]  Second input buffer to consume
                 void*           inputDataBSmaller, // [IN]  Second input buffer to consume
                 ME_MotionVector *outputMemory);    // [OUT] An allocation of (outputWidth * outputHeight) MotionVectors where the results will be stored

/*******************************************************************************************************************************
*   ME_Destroy
*
*   @brief
*       Perform clean up of resources
*   @return
*       none
*
*******************************************************************************************************************************/
#ifdef __cplusplus
extern "C"
#endif
void ME_Destroy();
