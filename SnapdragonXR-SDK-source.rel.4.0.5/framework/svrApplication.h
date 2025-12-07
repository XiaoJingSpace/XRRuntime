//=============================================================================
// FILE: svrApplication.h
//
//                  Copyright (c) 2015 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
#pragma once

#include <android/sensor.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <imgui.h>
#include <pthread.h>

#include "sxrApi.h"

#include "svrInput.h"
#include "svrRenderTarget.h"

#define SVR_NUM_EYE_BUFFERS     4
#define SVR_NUM_EYES			2

namespace Svr
{
    enum SvrEyeId
    {
        kLeft = 0,
        kRight = 1
    };

    struct SvrEyeBuffer
    {
        SvrRenderTarget eyeTarget[SVR_NUM_EYES];
		SvrRenderTarget singleSampledBufferForTimeWarp[SVR_NUM_EYES];
    };

    struct SvrApplicationContext
    {
        int             physicalWidth;
        int             physicalHeight;
        int             targetEyeWidth;
        int             targetEyeHeight;
        float           targetEyeFovXDeg;
        float           targetEyeFovYDeg;

        SvrEyeBuffer    eyeBuffers[SVR_NUM_EYE_BUFFERS];
        int             eyeBufferIndex;

        EGLDisplay      display;
        EGLSurface      eyeRenderSurface;
        EGLContext      eyeRenderContext;

        ANativeActivity*    activity;
        AAssetManager*  assetManager;
        ANativeWindow*  nativeWindow;

        sxrPerfLevel    cpuPerfLevel;
        sxrPerfLevel    gpuPerfLevel;
        unsigned int    trackingMode;

        bool            isProtectedContent;
        bool            isMotionAwareFrames;
        bool            isFoveationSubsampled;
        bool            isColorSpaceSRGB;
        bool            isCameraLayerEnabled;

        const char*     internalPath;
        const char*     externalPath;

        int            frameCount;

        // flag for headless
        bool           isInHeadless;

    };

    class SvrApplication
    {
    public:
        SvrApplication();
        virtual ~SvrApplication();
        virtual void LoadConfiguration() = 0;
        virtual void Initialize();
        virtual void Shutdown();

        virtual void Update();
        virtual void Render() = 0;
        virtual void Event(sxrEvent *evt);

        virtual void AllocateEyeBuffers();
        virtual void ProcessEvents();
        virtual bool GetHeadlessState();

        SvrInput& GetInput();
        SvrApplicationContext& GetApplicationContext();

    protected:
        //ImGui Members
        static bool mImGuiInitialized;
        static int  mImGuiShaderHandle;
        static int  mImGuiVertHandle;
        static int  mImGuiFragHandle;
        static int  mImGuiAttribLocTex;
        static int  mImGuiAttribLocProjMtx;
        static int  mImGuiAttribLocPos;
        static int  mImGuiAttribLocUv;
        static int  mImGuiAttribLocColor;
        static unsigned int mImGuiFontTextureHandle;
        static unsigned int mImGuiVboHandle;
        static unsigned int mImGuiVaoHandle;
        static unsigned int mImGuiElementsHandle;
        static void CreateImGuiDeviceObjects();
        static void RenderImGuiDrawLists(ImDrawData* draw_data);

    protected:
        SvrApplicationContext   mAppContext;
        SvrInput                mInput;
    };

    extern SvrApplication* CreateApplication();
}
