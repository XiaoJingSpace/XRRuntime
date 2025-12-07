//=============================================================================
// FILE: svrApiCore.h
//
//                  Copyright (c) 2016 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================
#ifndef _SVR_API_CORE_H_
#define _SVR_API_CORE_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>

#include <android/looper.h>
#include <android/native_window.h>
#include <android/sensor.h>

// BEGIN SxrPresentation region
#include <android/native_window_jni.h>
// END SxrPresentation region

#include <pthread.h>
#include <jni.h>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/transform.hpp"

#include "sxrApi.h"
#include "svrGpuTimer.h"
#include "private/svrApiEvent.h"
#include "QVRServiceClient.h"

#include "svrRenderExt.h"

#include "SvrServiceClient.h"
#include "ControllerManager.h"

#include "QVRCameraClient.h"


#define NUM_SWAP_FRAMES 4

namespace Svr
{
    struct svrFrameParamsInternal
    {
        sxrFrameParams      frameParams;
        GLsync              frameSync;
        GLsync              motionVectorSync;
        EGLSyncKHR          externalSync;
        uint64_t            frameSubmitTimeStamp;
        uint64_t            warpFrameLeftTimeStamp;
        uint64_t            warpFrameRightTimeStamp;
        uint64_t            minVSyncCount;
    };

    struct SvrModeContext
    {
        EGLDisplay      display;
        ANativeWindow*  nativeWindow;
        EGLint          colorspace;

        uint64_t        vsyncCount;
        uint64_t        vsyncTimeNano;
        pthread_mutex_t vsyncMutex;

        //Warp Thread/Context data
        EGLSurface      eyeRenderWarpSurface;
        EGLSurface      eyeRenderOrigSurface;
        EGLint          eyeRenderOrigConfigId;
        EGLContext      eyeRenderContext;

        EGLContext      warpRenderContext;
        EGLSurface      warpRenderSurface;
        int             warpRenderSurfaceWidth;
        int             warpRenderSurfaceHeight;

        pthread_cond_t  warpThreadContextCv;
        pthread_mutex_t warpThreadContextMutex;
        bool            warpContextCreated;

        pthread_t       warpThread;
        bool            warpThreadExit;

        pthread_t       vsyncThread;
        bool            vsyncThreadExit;

        pthread_cond_t  warpBufferConsumedCv;
        pthread_mutex_t warpBufferConsumedMutex;

        svrFrameParamsInternal frameParams[NUM_SWAP_FRAMES];
        unsigned int           submitFrameCount;
        unsigned int           warpFrameCount;
        uint64_t               prevSubmitVsyncCount;

		pid_t	        renderThreadId;
		pid_t	        warpThreadId;

        // Recenter transforms
        glm::fquat          recenterRot;
        glm::vec3           recenterPos;

        // Qvr service transform
        glm::mat4           qvrTransformMat;
        glm::mat4           qvrInverseMat;

		// Protected content
		bool                isProtectedContent;

		// Event Manager
        svrEventManager*    eventManager;

		// Performance Counters
        unsigned int        fpsFrameCounter = 0;
        unsigned int        fpsPrevTimeMs = 0;

        // Pose error tracking
        sxrHeadPoseState    prevPoseState;

#ifdef ENABLE_XR_CASTING
        uint64_t                SxrPresentationCount;
        ANativeWindow*          SxrPresentationWindowANativeWindow;
        EGLDisplay              SxrPresentationWindowDisplay;
        EGLContext              SxrPresentationWindowContext;
        EGLSurface              SxrPresentationWindowSurface;
        int32_t                 SxrPresentationWindowSurfaceWidth;
        int32_t                 SxrPresentationWindowSurfaceHeight;
        GLuint                  SxrPresentationFBO;
        GLint                   SxrPresentationCachedReadFBO;
        pthread_t               SxrPresentationThread;
        pid_t	                SxrPresentationThreadId;
        pthread_mutex_t         SxrPresentationReadyMutex;
        pthread_mutex_t         SxrPresentationFrameMutex;
        pthread_mutex_t         SxrPresentationSurfaceMutex;
        pthread_cond_t          SxrPresentationInitializedCv;
        pthread_cond_t          SxrPresentationFrameAvailableCV;
        pthread_cond_t          SxrPresentationSurfaceChangedCV;
        pthread_cond_t          SxrPresentationSurfaceDestroyedCV;
        volatile bool           SxrPresentationThreadExit;
        volatile bool           SxrPresentationSurfaceChanged;
        volatile bool           SxrPresentationSurfaceDestroyed;
        volatile bool           SxrPresentationThreadReady;
        svrFrameParamsInternal* SxrPresentationPendingFrameParams;
        uint64_t                SxrPresentationPendingFrameTimeStamp;
        svrFrameParamsInternal* SxrPresentationLastFrameParams;
        uint64_t                SxrPresentationLastFrameWarpedTimeStamp;
#endif // ENABLE_XR_CASTING
    };

    struct SvrAppContext
    {
        ALooper*            looper;

        JavaVM*	            javaVm;
        JNIEnv*	            javaEnv;
        jobject		        javaActivityObject;

        jclass              javaSvrApiClass;
        jmethodID           javaSvrApiStartVsyncMethodId;
        jmethodID           javaSvrApiStopVsyncMethodId;

        qvrservice_client_helper_t* qvrHelper;

        SvrServiceClient*           svrServiceClient;
        ControllerManager*          controllerManager;

        SvrModeContext*     modeContext;

        sxrDeviceInfo       deviceInfo;
        unsigned int        currentTrackingMode;

        bool                inVrMode;

        // In headless state, we will receive notifications
        bool                keepNotification;


#ifdef ENABLE_XR_CASTING
        jmethodID javaSxrEnablePresentation;
        bool isSxrPresentationInitialized;
#endif // ENABLE_XR_CASTING
    };

    extern SvrAppContext* gAppContext;
}

#endif //_SVR_API_CORE_H_
