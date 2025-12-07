//=============================================================================
// FILE: svrApplication.cpp
//
//                  Copyright (c) 2015 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "svrApplication.h"
#include "svrUtil.h"

namespace Svr
{

bool SvrApplication::mImGuiInitialized = false;
int  SvrApplication::mImGuiShaderHandle = 0;
int  SvrApplication::mImGuiVertHandle = 0;
int  SvrApplication::mImGuiFragHandle = 0;
int  SvrApplication::mImGuiAttribLocTex = 0;
int  SvrApplication::mImGuiAttribLocProjMtx = 0;
int  SvrApplication::mImGuiAttribLocPos = 0;
int  SvrApplication::mImGuiAttribLocUv = 0;
int  SvrApplication::mImGuiAttribLocColor = 0;
unsigned int SvrApplication::mImGuiFontTextureHandle = 0;
unsigned int SvrApplication::mImGuiVboHandle = 0;
unsigned int SvrApplication::mImGuiVaoHandle = 0;
unsigned int SvrApplication::mImGuiElementsHandle = 0;

SvrApplication::SvrApplication()
{
    memset(&mAppContext, 0, sizeof(mAppContext));
    mAppContext.physicalWidth = 0;
    mAppContext.physicalHeight = 0;
    mAppContext.targetEyeWidth = 0;
    mAppContext.targetEyeHeight = 0;

    mAppContext.display = EGL_NO_DISPLAY;

    mAppContext.eyeRenderSurface = EGL_NO_SURFACE;
    mAppContext.eyeRenderContext = EGL_NO_CONTEXT;

    mAppContext.assetManager = NULL;
    mAppContext.nativeWindow = NULL;

    mAppContext.frameCount = 0;

	mAppContext.isProtectedContent = false;
    mAppContext.isMotionAwareFrames = false;
    mAppContext.isFoveationSubsampled = false;
    mAppContext.isColorSpaceSRGB = false;
    mAppContext.isInHeadless = false;
    mAppContext.isCameraLayerEnabled = false;
}

SvrApplication::~SvrApplication()
{
}

void SvrApplication::Initialize()
{
    if (!mImGuiInitialized)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = mAppContext.targetEyeWidth;
        io.DisplaySize.y = mAppContext.targetEyeHeight;
        io.DeltaTime = 1.0f / 60.0f;
        io.IniFilename = NULL;
        io.RenderDrawListsFn = SvrApplication::RenderImGuiDrawLists;

        CreateImGuiDeviceObjects();

        mImGuiInitialized = true;
    }

    mAppContext.eyeBufferIndex = 0;
    mAppContext.cpuPerfLevel = kPerfMedium;
    mAppContext.gpuPerfLevel = kPerfMedium;
    mAppContext.trackingMode = (kTrackingRotation | kTrackingPosition);

    AllocateEyeBuffers();
}

void SvrApplication::AllocateEyeBuffers()
{
    //Default to separate render targets per eye
    LOGI("SvrApplication - Allocating Separate Single Eye Buffers");
    for (int i = 0; i < SVR_NUM_EYE_BUFFERS; i++)
    {
        mAppContext.eyeBuffers[i].eyeTarget[kLeft].Initialize(mAppContext.targetEyeWidth, mAppContext.targetEyeHeight, 1, GL_RGBA8, true, mAppContext.isProtectedContent);
        mAppContext.eyeBuffers[i].eyeTarget[kRight].Initialize(mAppContext.targetEyeWidth, mAppContext.targetEyeHeight, 1, GL_RGBA8, true, mAppContext.isProtectedContent);
    }
}

void SvrApplication::Update()
{
    // Process any events that have come in since last time we were here
    ProcessEvents();

    mInput.Update();

    if (mImGuiInitialized)
    {
        ImGui::NewFrame();
    }
}

void SvrApplication::Shutdown()
{
    //Free Eye Buffers
    for (int i = 0; i < SVR_NUM_EYE_BUFFERS; i++)
    {
		Svr::SvrEyeBuffer &eyeBuffer = mAppContext.eyeBuffers[i];
        for(int k=0;k<SVR_NUM_EYES;k++)
        {
            eyeBuffer.eyeTarget[k].Destroy();
        }
    }
}

SvrInput& SvrApplication::GetInput()
{
    return mInput;
}

SvrApplicationContext& SvrApplication::GetApplicationContext()
{
    return mAppContext;
}

void SvrApplication::CreateImGuiDeviceObjects()
{
    const GLchar *vertex_shader =
        "#version 300 es\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 300 es\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    mImGuiShaderHandle = GL(glCreateProgram());
    mImGuiVertHandle = GL(glCreateShader(GL_VERTEX_SHADER));
    mImGuiFragHandle = GL(glCreateShader(GL_FRAGMENT_SHADER));
    GL(glShaderSource(mImGuiVertHandle, 1, &vertex_shader, 0));
    GL(glShaderSource(mImGuiFragHandle, 1, &fragment_shader, 0));
    GL(glCompileShader(mImGuiVertHandle));
    GL(glCompileShader(mImGuiFragHandle));
    GL(glAttachShader(mImGuiShaderHandle, mImGuiVertHandle));
    GL(glAttachShader(mImGuiShaderHandle, mImGuiFragHandle));
    GL(glLinkProgram(mImGuiShaderHandle));

    mImGuiAttribLocTex = GL(glGetUniformLocation(mImGuiShaderHandle, "Texture"));
    mImGuiAttribLocProjMtx = GL(glGetUniformLocation(mImGuiShaderHandle, "ProjMtx"));
    mImGuiAttribLocPos = GL(glGetAttribLocation(mImGuiShaderHandle, "Position"));
    mImGuiAttribLocUv = GL(glGetAttribLocation(mImGuiShaderHandle, "UV"));
    mImGuiAttribLocColor = GL(glGetAttribLocation(mImGuiShaderHandle, "Color"));

    GL(glGenBuffers(1, &mImGuiVboHandle));
    GL(glGenBuffers(1, &mImGuiElementsHandle));

    GL(glGenVertexArrays(1, &mImGuiVaoHandle));
    GL(glBindVertexArray(mImGuiVaoHandle));
    GL(glBindBuffer(GL_ARRAY_BUFFER, mImGuiVboHandle));
    GL(glEnableVertexAttribArray(mImGuiAttribLocPos));
    GL(glEnableVertexAttribArray(mImGuiAttribLocUv));
    GL(glEnableVertexAttribArray(mImGuiAttribLocColor));

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    GL(glVertexAttribPointer(mImGuiAttribLocPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos)));
    GL(glVertexAttribPointer(mImGuiAttribLocUv, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv)));
    GL(glVertexAttribPointer(mImGuiAttribLocColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)));
#undef OFFSETOF
    GL(glBindVertexArray(0));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));


    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    GL(glGenTextures(1, &mImGuiFontTextureHandle));
    GL(glBindTexture(GL_TEXTURE_2D, mImGuiFontTextureHandle));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)mImGuiFontTextureHandle;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();
}

void SvrApplication::RenderImGuiDrawLists(ImDrawData* draw_data)
{
    GLint last_program, last_texture;
    GL(glGetIntegerv(GL_CURRENT_PROGRAM, &last_program));
    GL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture));
    GL(glEnable(GL_BLEND));
    GL(glBlendEquation(GL_FUNC_ADD));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glDisable(GL_CULL_FACE));
    GL(glDisable(GL_DEPTH_TEST));
    GL(glEnable(GL_SCISSOR_TEST));
    GL(glActiveTexture(GL_TEXTURE0));

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f / width, 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / -height, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f },
    };
    GL(glUseProgram(mImGuiShaderHandle));
    GL(glUniform1i(mImGuiAttribLocTex, 0));
    GL(glUniformMatrix4fv(mImGuiAttribLocProjMtx, 1, GL_FALSE, &ortho_projection[0][0]));
    GL(glBindVertexArray(mImGuiVaoHandle));

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        GL(glBindBuffer(GL_ARRAY_BUFFER, mImGuiVboHandle));
        GL(glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW));

        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mImGuiElementsHandle));
        GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW));

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                GL(glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId));
                // GL(glScissor((int)pcmd->ClipRect.x, (int)(height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y)));
                GL(glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_offset));
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL(glBindVertexArray(0));
}

void SvrApplication::Event(sxrEvent *evt)
{
    if (sxrIsProximitySuspendEnabled() && evt != NULL && evt->eventType == kEventProximity)
    {
        bool proximitySuspend = evt->eventData.proximity.distance > 0.0f;
        SxrResult result;
        result = proximitySuspend ? sxrSetXrPause() : sxrSetXrResume();
        if (result != SXR_ERROR_NONE)
        {
            LOGE("Event Proximity Failed");
        }
    }
}

bool SvrApplication::GetHeadlessState()
{
    return mAppContext.isInHeadless;
}

void SvrApplication::ProcessEvents()
{
    const char *pEventTag = "(SVR Event) ";
    char eventMsg[512];

    SxrResult retVal = SXR_ERROR_NONE;
    float minValues[3];
    float maxValues[3];
    float visibilityRadius;

    sxrEvent evt;
    while (sxrPollEvent(&evt))
    {
        memset(eventMsg, 0, sizeof(eventMsg));
        switch (evt.eventType)
        {
        case kEventNone:
            sprintf(eventMsg, "None!");
            break;

        case kEventSdkServiceStarting:
            sprintf(eventMsg, "SDK Service Starting...");
            break;

        case kEventSdkServiceStarted:
            sprintf(eventMsg, "SDK Service Started");
            break;

        case kEventSdkServiceStopped:
            sprintf(eventMsg, "SDK Service Stopped");
            break;

        case kEventControllerConnecting:
            sprintf(eventMsg, "Controller Connecting...");
            break;

        case kEventControllerConnected:
            sprintf(eventMsg, "Controller Connected");
            break;

        case kEventControllerDisconnected:
            sprintf(eventMsg, "Controller Disconnected");
            break;

        case kEventThermal:
            switch (evt.eventData.thermal.zone)
            {
            case kCpu:
                sprintf(eventMsg, "Thermal: CPU %u", evt.eventData.thermal.level);
                break;
            case kGpu:
                sprintf(eventMsg, "Thermal : GPU %u", evt.eventData.thermal.level);
                break;
            case kSkin:
                sprintf(eventMsg, "Thermal : Skin %u", evt.eventData.thermal.level);
                break;
            default:
                break;
            }   // Thermal Zone
            break;

        case kEventVrModeStarted:
            sprintf(eventMsg, "VR Mode Started");
            break;

        case kEventVrModeStopping:
            sprintf(eventMsg, "VR Mode Stopping...");
            break;

        case kEventVrModeStopped:
            sprintf(eventMsg, "VR Mode Stopped");
            break;

        case kEventVrModeHeadless:
            // VRMODE_STOPPING -> VRMODE_HEADLESS
            if(evt.eventData.state.new_state == 5)
            {
                sprintf(eventMsg, "kEventVrModeHeadless");
                sxrSetNotificationFlag();
                mAppContext.isInHeadless = true;
            }
            // VRMODE_HEADLESS -> VRMODE_STOPPED
            if(evt.eventData.state.previous_state == 5)
            {
                sprintf(eventMsg, "Restart from Headless");
                mAppContext.isInHeadless = false;
                LOGI("DEBUG mAppContext.isInHeadless = %d", mAppContext.isInHeadless);
            }
            break;

        case kEventSensorError:
            sprintf(eventMsg, "Sensor Error! Data = [%d, %d]", evt.eventData.data[0], evt.eventData.data[1]);
            break;

        case kEventMagnometerUncalibrated:
            sprintf(eventMsg, "Magnometer Uncalibrated");
            break;

        case kEventBoundarySystemCollision:
            sprintf(eventMsg, "Boundary System Collision: ");
            retVal = sxrGetBoundaryParameters(minValues, maxValues, &visibilityRadius);
            if (retVal != SXR_ERROR_NONE)
            {
                LOGE("    Error reading Boundary parameters!");
            }
            else
            {
                LOGE("    Boundary: Min = (%0.2f, %0.2f, %0.2f); Max = (%0.2f, %0.2f, %0.2f); Radius = %0.2f", minValues[0],
                    minValues[1],
                    minValues[2],
                    maxValues[0],
                    maxValues[1],
                    maxValues[2],
                    visibilityRadius);
            }
            break;

        case kEvent6dofRelocation:
            sprintf(eventMsg, "6DOF Relocation in Progress");
            break;

        case kEvent6dofWarningFeatureCount:
            sprintf(eventMsg, "6DOF Warning => Feature Count");
            break;

        case kEvent6dofWarningLowLight:
            sprintf(eventMsg, "6DOF Warning => Low Light");
            break;

        case kEvent6dofWarningBrightLight:
            sprintf(eventMsg, "6DOF Warning => Bright Light");
            break;

        case kEvent6dofWarningCameraCalibration:
            sprintf(eventMsg, "6DOF Warning => Stereo Camera Calibration");
            break;

        case kEventProximity:
        {
            sprintf(eventMsg, "Proximity Sensor Distance = %f", evt.eventData.proximity.distance);
        }
        break;

        default:
            sprintf(eventMsg, "Unknown Event Type (%d)! Data = [%d, %d]", evt.eventType, evt.eventData.data[0], evt.eventData.data[1]);
            break;
        }   // Switch Event Type

        LOGI("%s [Time = %0.4f]: %s", pEventTag, evt.eventTimeStamp, eventMsg);

        Event(&evt);

        //if (evt.eventType == kEventProximity)
        //{
        //    bool proximitySuspend = evt.eventData.proximity.distance > 0.0f;
        //    SxrResult result;
        //    result = proximitySuspend ? sxrSetXrPause() : sxrSetXrResume();
        //    if (result != SXR_ERROR_NONE)
        //    {
        //        LOGE("EventProximity Failed");
        //    }
        //}
    }

}   // ProcessEvents

}
