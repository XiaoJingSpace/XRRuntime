//-----------------------------------------------------------------------------
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

#include <jni.h>
#include "RingBuffer.h"
#include <android/log.h>
#include "svrApi.h"

extern "C" JNIEXPORT jint Java_com_qualcomm_snapdragonvrservice_ControllerConnection_AllocateSharedMemory(JNIEnv *, jobject, int size)
//-----------------------------------------------------------------------------
{
    RingBuffer<svrControllerState>* ringBuffer = new RingBuffer<svrControllerState>();
    ringBuffer->allocate(size);
    return (jint)ringBuffer;
}

extern "C" JNIEXPORT jint Java_com_qualcomm_snapdragonvrservice_ControllerConnection_GetFileDescriptor(JNIEnv *, jobject, jint ringBufferPtr)
//-----------------------------------------------------------------------------
{
    RingBuffer<svrControllerState>* ringBuffer = (RingBuffer<svrControllerState>*)(ringBufferPtr);
    jint fd = 0;
    if( ringBuffer != 0 )
    {
        fd = ringBuffer->getFd();
    }
    return fd;
}

extern "C" JNIEXPORT jint Java_com_qualcomm_snapdragonvrservice_ControllerConnection_GetFileDescriptorSize(JNIEnv *, jobject, jint ringBufferPtr)
//-----------------------------------------------------------------------------
{
    RingBuffer<svrControllerState>* ringBuffer = (RingBuffer<svrControllerState>*)(ringBufferPtr);
    jint size = 0;
    if( ringBuffer != 0 )
    {
        size = ringBuffer->getBufferSize();
    }
    return size;
}

extern "C" JNIEXPORT void Java_com_qualcomm_snapdragonvrservice_ControllerConnection_FreeSharedMemory(JNIEnv *, jobject, jint ringBufferPtr)
//-----------------------------------------------------------------------------
{
    RingBuffer<svrControllerState>* ringBuffer = (RingBuffer<svrControllerState>*)(ringBufferPtr);
    //__android_log_print(ANDROID_LOG_ERROR, "XXX", "Free:: ringBufferPtr = 0x%08x", ringBufferPtr);
    if( ringBuffer != 0 )
    {
        delete ringBuffer;
    }
}

extern "C" JNIEXPORT void Java_com_qualcomm_snapdragonvrservice_ControllerConnection_GetControllerState(JNIEnv * env, jobject,jint ringBufferPtr)
{
    jclass controllerStateClass = NULL;
    jmethodID methodUpdateState = NULL;
    controllerStateClass = env->FindClass("com/qualcomm/snapdragonvrservice/ControllerState");
    methodUpdateState = env->GetStaticMethodID(controllerStateClass,"updateState","(FFFFFFFFFFFFFIIIFFFFFFFFI)V");

    RingBuffer<svrControllerState>* ringBuffer = (RingBuffer<svrControllerState>*)(ringBufferPtr);
    if (ringBuffer != 0)
    {
        svrControllerState* currentData = ringBuffer->get();
        if( currentData != 0x00 )
        {
            env->CallStaticVoidMethod(controllerStateClass,methodUpdateState,
                                     currentData->rotation.x, currentData->rotation.y, currentData->rotation.z, currentData->rotation.w,
                                     currentData->position.x, currentData->position.y, currentData->position.z,
                                     currentData->accelerometer.x,currentData->accelerometer.y, currentData->accelerometer.z,
                                     currentData->gyroscope.x,currentData->gyroscope.y, currentData->gyroscope.z,
                                     currentData->buttonState, currentData->isTouching, currentData->timestamp,
                                     currentData->analog2D[0].x, currentData->analog2D[0].y,
                                     currentData->analog2D[1].x, currentData->analog2D[1].y,
                                     currentData->analog1D[0], currentData->analog1D[1],
                                     currentData->analog1D[2], currentData->analog1D[3],
                                     currentData->connectionState);
        } else {
            __android_log_print(ANDROID_LOG_ERROR, "ControllerDebug","current data is null ");
        }
    }

}

