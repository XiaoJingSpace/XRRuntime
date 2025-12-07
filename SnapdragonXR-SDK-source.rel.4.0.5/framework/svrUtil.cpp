//=============================================================================
// FILE: svrUtil.cpp
//
//                  Copyright (c) 2015 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//============================================================================

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/transform.hpp"

#include "svrUtil.h"

namespace Svr
{

    void SvrCheckGlError(const char* file, int line)
    {
        for (GLint error = glGetError(); error; error = glGetError())
        {
            char *pError;
            switch (error)
            {
            case GL_NO_ERROR:                       pError = (char *)"GL_NO_ERROR";                         break;
            case GL_INVALID_ENUM:                   pError = (char *)"GL_INVALID_ENUM";                     break;
            case GL_INVALID_VALUE:                  pError = (char *)"GL_INVALID_VALUE";                    break;
            case GL_INVALID_OPERATION:              pError = (char *)"GL_INVALID_OPERATION";                break;
            case GL_OUT_OF_MEMORY:                  pError = (char *)"GL_OUT_OF_MEMORY";                    break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:  pError = (char *)"GL_INVALID_FRAMEBUFFER_OPERATION";    break;

            default:
                LOGE("glError (0x%x) %s:%d\n", error, file, line);
                return;
            }

            LOGE("glError (%s) %s:%d\n", pError, file, line);
            return;
        }
        return;
    }

    void SvrCheckEglError(const char* file, int line)
    {
        for (int i = 0; i < 10; i++)
        {
            const EGLint error = eglGetError();
            if (error == EGL_SUCCESS)
            {
                break;
            }

            char *pError;
            switch (error)
            {
            case EGL_SUCCESS:				pError = (char *)"EGL_SUCCESS"; break;
            case EGL_NOT_INITIALIZED:		pError = (char *)"EGL_NOT_INITIALIZED"; break;
            case EGL_BAD_ACCESS:			pError = (char *)"EGL_BAD_ACCESS"; break;
            case EGL_BAD_ALLOC:				pError = (char *)"EGL_BAD_ALLOC"; break;
            case EGL_BAD_ATTRIBUTE:			pError = (char *)"EGL_BAD_ATTRIBUTE"; break;
            case EGL_BAD_CONTEXT:			pError = (char *)"EGL_BAD_CONTEXT"; break;
            case EGL_BAD_CONFIG:			pError = (char *)"EGL_BAD_CONFIG"; break;
            case EGL_BAD_CURRENT_SURFACE:	pError = (char *)"EGL_BAD_CURRENT_SURFACE"; break;
            case EGL_BAD_DISPLAY:			pError = (char *)"EGL_BAD_DISPLAY"; break;
            case EGL_BAD_SURFACE:			pError = (char *)"EGL_BAD_SURFACE"; break;
            case EGL_BAD_MATCH:				pError = (char *)"EGL_BAD_MATCH"; break;
            case EGL_BAD_PARAMETER:			pError = (char *)"EGL_BAD_PARAMETER"; break;
            case EGL_BAD_NATIVE_PIXMAP:		pError = (char *)"EGL_BAD_NATIVE_PIXMAP"; break;
            case EGL_BAD_NATIVE_WINDOW:		pError = (char *)"EGL_BAD_NATIVE_WINDOW"; break;
            case EGL_CONTEXT_LOST:			pError = (char *)"EGL_CONTEXT_LOST"; break;
            default:
                LOGE("eglError (0x%x) %s:%d\n", error, file, line);
                return;
            }
            LOGE("eglError (%s) %s:%d\n", pError, file, line);
            return;
        }
        return;
    }

    void SvrGetEyeViewMatrices(const sxrHeadPoseState& poseState, bool bUseHeadModel,
								glm::vec3 eyeTranslationLeft, glm::vec3 eyeTranslationRight, glm::fquat eyeRotationLeft, glm::fquat eyeRotationRight, float headHeight, float headDepth,
								glm::mat4& outLeftEyeMatrix, glm::mat4& outRightEyeMatrix)
    {
		//////////////////////////////////////////////////////////////
		// There's the possibility that we have custon rotation/translation per eye
		// check to see if we need to use defaults or there are valid custon values

		// The app has the option of adjusting each viewmatrix per eye. Not just an offset like for IPD but also a rotatio about a point.
		// The default is the default DEFAULT_IPD and no rotation. 

		// See if there's a custom translation/eye
		const glm::vec3 zero(0.0f, 0.0f, 0.0f);
		if (eyeTranslationLeft == zero && eyeTranslationRight == zero)
		{
			// there's no specified IPD, use the default
			eyeTranslationLeft.x  = DEFAULT_IPD * -0.5f;
			eyeTranslationRight.x = DEFAULT_IPD * +0.5f;
		}

		// is there a custom rotation/eye?
		// debugging-------------
		const glm::fquat qzero(0.0f, 0.0f, 0.0f, 0.0f); // invalid quat
		if (eyeRotationLeft == qzero && eyeRotationRight == qzero)
		{
			LOGE("error - SvrGetEyeViewMatrices has a zero quaternion, you shoukld pass in a valid quat");
			// make them identity
			const glm::fquat identity(1.0f, 0.0f, 0.0f, 0.0f); // glm quat is (w)(xyz)
			eyeRotationLeft = eyeRotationRight = identity;
		}
		// Done with checking for custom values
		// the eye rotations are either the passed in values or identity
		// the eye translations are either the passed in values or they use the default IPD
		////////////////////////////////////////////////////////////////////

        glm::fquat poseQuat = glm::fquat(poseState.pose.rotation.w, poseState.pose.rotation.x, poseState.pose.rotation.y, poseState.pose.rotation.z);
        glm::mat4 poseMat = glm::mat4_cast(poseQuat);

        glm::vec3 headOffset(0.0f, 0.0f, 0.0f);

        if (bUseHeadModel && 
            ((poseState.poseStatus | kTrackingPosition) == 0 ))
        {
            //Only use a head model if the uses has chosen to enable it and positional tracking is not enabled
            headOffset = glm::vec3(0.0f, headHeight, -headDepth);
        }
        else
        {
			// Todo: What are these statements for? They b oth do the same thing
            if ((poseState.poseStatus | kTrackingPosition) != 0)
            {
                //Positional tracking is enabled so use that positional data from the pose 
                headOffset.x = poseState.pose.position.x;
                headOffset.y = poseState.pose.position.y;
                headOffset.z = poseState.pose.position.z;
            }
            else
            {
                //No head model, no positional tracking data
                // But the user may have set the desired position in the pose state
                headOffset.x = poseState.pose.position.x;
                headOffset.y = poseState.pose.position.y;
                headOffset.z = poseState.pose.position.z;
            }
        }

        poseMat = glm::translate(poseMat, headOffset);

		// Now using custom rotations (or identity)
		glm::mat4 leftEyeOffsetMat = glm::translate(glm::mat4_cast(eyeRotationLeft), -eyeTranslationLeft);
        glm::mat4 rightEyeOffsetMat = glm::translate(glm::mat4_cast(eyeRotationRight), -eyeTranslationRight);

        outLeftEyeMatrix = leftEyeOffsetMat * poseMat;
        outRightEyeMatrix = rightEyeOffsetMat * poseMat;
    }

	glm::vec3 SvrPosePositionCameraSpaceToWorldSpace(const glm::vec3& posePosition)
	{
		return -posePosition;
	}
	glm::quat SvrPoseRotationCameraSpaceToWorldSpace(const glm::quat& poseRotation)
	{
		return conjugate(poseRotation);
	}	
}//End namespace Svr
