//-----------------------------------------------------------------------------
//  Copyright (c) 2017 Qualcomm Technologies, Inc.
//  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

#include <jni.h>
#include "RingBuffer.h"
#include "private/svrApiCore.h"
#include <android/log.h>

#define JNI_FUNC(x) Java_com_qualcomm_svrapi_controllers_ControllerContext_##x

//TODO: create a java wrapper class for RingBuffer
//-----------------------------------------------------------------------------
extern "C" JNIEXPORT jlong JNICALL JNI_FUNC(AllocateSharedMemory)(JNIEnv *, jobject, int size)
//-----------------------------------------------------------------------------
{
    RingBuffer<sxrControllerState>* ringBuffer = new RingBuffer<sxrControllerState>();
    //__android_log_print(ANDROID_LOG_ERROR, "XXX", "Allocate:: ringBufferPtr = 0x%08x", ringBuffer);
    ringBuffer->allocate(size);
    return (jlong)ringBuffer;
}

//-----------------------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL JNI_FUNC(GetFileDescriptor)(JNIEnv *, jobject, jlong ringBufferPtr)
//-----------------------------------------------------------------------------
{
    RingBuffer<sxrControllerState>* ringBuffer = (RingBuffer<sxrControllerState>*)(ringBufferPtr);
    jint fd = 0;
    if( ringBuffer != 0 )
    {
        fd = ringBuffer->getFd();
    }
    return fd;
}

//-----------------------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL JNI_FUNC(GetFileDescriptorSize)(JNIEnv *, jobject, jlong ringBufferPtr)
//-----------------------------------------------------------------------------
{
    RingBuffer<sxrControllerState>* ringBuffer = (RingBuffer<sxrControllerState>*)(ringBufferPtr);
    jint size = 0;
    if( ringBuffer != 0 )
    {
        size = ringBuffer->getBufferSize();
    }
    return size;
}

//-----------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL JNI_FUNC(FreeSharedMemory)(JNIEnv *, jobject, jlong ringBufferPtr)
//-----------------------------------------------------------------------------
{
    RingBuffer<sxrControllerState>* ringBuffer = (RingBuffer<sxrControllerState>*)(ringBufferPtr);
    //__android_log_print(ANDROID_LOG_ERROR, "XXX", "Free:: ringBufferPtr = 0x%08x", ringBufferPtr);
    if( ringBuffer != 0 )
    {
        delete ringBuffer;
    }
}