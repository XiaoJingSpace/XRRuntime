//-----------------------------------------------------------------------------
//  Copyright (c) 2017 Qualcomm Technologies, Inc.
//  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

#include <jni.h>
#include "RingBuffer.h"
#include <pthread.h>
#include <android/log.h>
#include "sxrApi.h"
#include <string.h>
#define JNI_FUNC(x) Java_com_qti_acg_apps_controllers_finch_ControllerContext_##x
/**************************************************************************//**
* qvrservice_head_tracking_data_t
* -----------------------------------------------------------------------------
* This structure contains a quaternion (x,y,z,w) representing rotational pose
* and position vector representing translational pose in the Android Portrait
* coordinate system. ts is the timestamp of
* the corresponding sensor value originating from the sensor stack.
* The cofficients may be used for computing a forward-predicted pose.
* tracking_state contains the state of 6DOF tracking.
*   bit 0     : RELOCATION_IN_PROGRESS
*   bits 1-15 : reserved
* tracking_warning_flags contains the 6DOF tracking warnings
*   bit 0     : LOW_FEATURE_COUNT_ERROR
*   bit 1     : LOW_LIGHT_ERROR
*   bit 2     : BRIGHT_LIGHT_ERROR
*   bit 3     : STEREO_CAMERA_CALIBRATION_ERROR
*   bits 4-15 : reserved
* flags_3dof_mag_used bit will be set only when TRACKING_MODE_ROTATIONAL_MAG
* mode is used. flags_3dof_mag_calibrated will be set only when mag data
* is calibrated. If the flag is not set, user needs to be notified to move
* the device for calibrating mag sensor (i.e. figure 8 movement)
* pose_quality, sensor_quality and camera_quality fields are filled only
* for TRACKING_MODE_POSITIONAL mode. They are scaled from 0.0 (bad) to 1.0
* (good).
******************************************************************************/
typedef struct {
    float rotation[4];
    float translation[3];
    uint32_t reserved;
    uint64_t ts;
    uint64_t reserved1;
    float prediction_coff_s[3];
    uint32_t reserved2;
    float prediction_coff_b[3];
    uint32_t reserved3;
    float prediction_coff_bdt[3];
    uint32_t reserved4;
    float prediction_coff_bdt2[3];
    uint16_t tracking_state;
    uint16_t tracking_warning_flags;
    uint32_t flags_3dof_mag_used : 1;
    uint32_t flags_3dof_mag_calibrated : 1;
    float pose_quality;
    float sensor_quality;
    float camera_quality;
    float prediction_coff_ts[3];
    uint32_t reserved6;
    float prediction_coff_tb[3];
    uint32_t reserved7;
    uint8_t reserved8[32];
} qvrservice_head_tracking_data_t;


enum ButtonCode
{
    kNone = 0,
    kApp = 1,   // App键
    kClick = 2, // Click键
    kHome = 4,  // home键
};
class VRSControllerState
{
    public:

    float gyrox = 0.0f;
    float gyroy = 0.0f;
    float gyroz = 0.0f;
    float accelx = 0.0f;
    float accely = 0.0f;
    float accelz = 0.0f;

    bool isTouching = false;
    float touchPosX = 0.0f;
    float touchPosY = 0.0f;

    bool touchDown = false;
    bool touchUp = false;
    bool recentered = false;

    bool clickButtonState = false;
    bool clickButtonDown = false;
    bool clickButtonUp = false;

    bool appButtonState = false;
    bool appButtonDown = false;
    bool appButtonUp = false;

    bool homeButtonDown = false;
    bool homeButtonState = false;
    bool homeButtonUp = false;

    int button = 0;
};
class NativeContext {
    public:
        sxrControllerState state;
        RingBuffer<sxrControllerState>* sharedMemory;
    public:
        NativeContext()
        {
            sharedMemory = 0;
			state.rotation.x = 0;
			state.rotation.y = 0;
			state.rotation.z = 0;
			state.rotation.w = 1;
			
			state.position.x = 0;
			state.position.y = 0;
			state.position.z = 0;
			
			state.accelerometer.x = 0;
			state.accelerometer.y = 0;
			state.accelerometer.z = 0;
			
			state.gyroscope.x = 0;
			state.gyroscope.y = 0;
			state.gyroscope.z = 0;
			
		
			state.connectionState = (sxrControllerConnectionState)0;
			state.buttonState = 0;
			state.isTouching = 0;
			state.timestamp = 0;
			for(int i=0;i<4;i++)
			{
				state.analog2D[i].x = 0;
				state.analog2D[i].y = 0;
			}
			
			for(int i=0;i<8;i++)
			{
				state.analog1D[i] = 0;
			}
        }

        ~NativeContext()
        {
            delete sharedMemory;
            sharedMemory = 0;
        }
};
VRSControllerState state, pre_state;

class PoseNativeContext {
    public:
        void* sharedMemory = NULL;
        volatile int32_t *index;
        uint32_t index_offset = 0;
        uint32_t ring_offset = 64;
        uint32_t num_elements = 80;
        qvrservice_head_tracking_data_t* pose_ring;
};
//-----------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL JNI_FUNC(updateNativeConnectionState)(JNIEnv *jniEnv, jobject, jint ptr, jint state)
//-----------------------------------------------------------------------------
{
	NativeContext* entry = (NativeContext*)(ptr);
	entry->state.connectionState = (sxrControllerConnectionState)state;
	
    if( entry->sharedMemory != 0 )
    {
        entry->sharedMemory->set(&entry->state);
    }
}

//-----------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL JNI_FUNC(updateNativeState)(JNIEnv *jniEnv, jobject, jint ptr, jint state, jint btns,
                                                                                                    jfloat pos0, jfloat pos1, jfloat pos2,
                                                                                                    jfloat rot0, jfloat rot1, jfloat rot2, jfloat rot3,
                                                                                                    jfloat gyro0, jfloat gyro1, jfloat gyro2,
                                                                                                    jfloat acc0, jfloat acc1, jfloat acc2,
                                                                                                    jint timestamp, jint touchpads, jfloat x, jfloat y,float trigger)
//-----------------------------------------------------------------------------
{
    NativeContext* entry = (NativeContext*)(ptr);

    entry->state.rotation.x = rot0;
    entry->state.rotation.y = rot1;
    entry->state.rotation.z = -rot2;
    entry->state.rotation.w = -rot3;

    entry->state.position.x = pos0;
    entry->state.position.y = pos1;
    entry->state.position.z = pos2;
	
	entry->state.accelerometer.x = acc0;
	entry->state.accelerometer.y = acc1;
	entry->state.accelerometer.z = acc2;
	
	entry->state.gyroscope.x = gyro0;
	entry->state.gyroscope.y = gyro1;
	entry->state.gyroscope.z = gyro2;
	

    entry->state.connectionState = (sxrControllerConnectionState)state;
    entry->state.buttonState = btns;
    entry->state.isTouching = touchpads;
    entry->state.timestamp = timestamp;
    entry->state.analog2D[0].x = x;
    entry->state.analog2D[0].y = y;
    entry->state.analog1D[0]   = trigger;

    if( entry->sharedMemory != 0 )
    {
        entry->sharedMemory->set(&entry->state);
    }

    //__android_log_print(ANDROID_LOG_ERROR, "XXXXX", "state updated - %f %f %f", p0, p1, p2);
}

void updateNativeState1 (JNIEnv *jniEnv, jobject, jint ptr, jint state, jint btns,
                                                                                                    jfloat pos0, jfloat pos1, jfloat pos2,
                                                                                                    jfloat rot0, jfloat rot1, jfloat rot2, jfloat rot3,
                                                                                                    jfloat gyro0, jfloat gyro1, jfloat gyro2,
                                                                                                    jfloat acc0, jfloat acc1, jfloat acc2,
                                                                                                    jint timestamp, jint touchpads, jfloat x, jfloat y,float trigger)
//-----------------------------------------------------------------------------
{
    NativeContext* entry = (NativeContext*)(ptr);

    entry->state.rotation.x = rot0;
    entry->state.rotation.y = rot1;
    entry->state.rotation.z = -rot2;
    entry->state.rotation.w = -rot3;

    entry->state.position.x = pos0;
    entry->state.position.y = pos1;
    entry->state.position.z = pos2;

	entry->state.accelerometer.x = acc0;
	entry->state.accelerometer.y = acc1;
	entry->state.accelerometer.z = acc2;

	entry->state.gyroscope.x = gyro0;
	entry->state.gyroscope.y = gyro1;
	entry->state.gyroscope.z = gyro2;


    entry->state.connectionState = (sxrControllerConnectionState)state;
    entry->state.buttonState = btns;
    entry->state.isTouching = touchpads;
    entry->state.timestamp = timestamp;
    entry->state.analog2D[0].x = x;
    entry->state.analog2D[0].y = y;


    if( entry->sharedMemory != 0 )
    {
        entry->sharedMemory->set(&entry->state);
    }

    //__android_log_print(ANDROID_LOG_ERROR, "XXXXX", "state updated - %f %f %f", p0, p1, p2);
}

//-----------------------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL JNI_FUNC(createNativeContext)(JNIEnv *jniEnv, jobject, jint fd, jint size)
//-----------------------------------------------------------------------------
{
    NativeContext* nativeContext = new NativeContext();
    nativeContext->sharedMemory = RingBuffer<sxrControllerState>::fromFd(fd, size, false);
    return (jint)(nativeContext);
}

//-----------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL JNI_FUNC(freeNativeContext)(JNIEnv *jniEnv, jobject, jint ptr)
//-----------------------------------------------------------------------------
{
    NativeContext* nativeContext = (NativeContext*)(ptr);
    if( nativeContext != 0 )
    {
        delete nativeContext;
    }
}
extern "C" JNIEXPORT jint JNICALL JNI_FUNC(createNativePoseContext)(JNIEnv *jniEnv, jobject, jint fd, jint size)
{
    PoseNativeContext* nativeContext = new PoseNativeContext();
    nativeContext->sharedMemory = SharedMemory::map(fd, size, true);
    nativeContext->index = (volatile int32_t*) ((intptr_t*) nativeContext->sharedMemory + nativeContext->index_offset);
    nativeContext->pose_ring = (qvrservice_head_tracking_data_t*) ((intptr_t*) nativeContext->sharedMemory +  + nativeContext->ring_offset);
    return (jint)(nativeContext);
}
extern "C" JNIEXPORT void JNICALL JNI_FUNC(getPoseData)(JNIEnv *jniEnv, jobject, jint ptr, jfloatArray pos, jfloatArray rot, jlongArray timestamp)
{
    PoseNativeContext* nativeContext = (PoseNativeContext*)(ptr);
    int32_t idx = *(nativeContext->index);
    if (idx>=0 && idx < static_cast<int>(nativeContext->num_elements)) {
        qvrservice_head_tracking_data_t *t = (qvrservice_head_tracking_data_t*) &(nativeContext->pose_ring[idx]);
        //__android_log_print(ANDROID_LOG_ERROR, "XXXXX", "quat =  (%f %f %f %f) position=( %f %f %f)\n", t->rotation[0], t->rotation[1], t->rotation[2], t->rotation[3], t->translation[0], t->translation[1],t->translation[2]);
        jfloat *position = (jniEnv)->GetFloatArrayElements(pos, 0);
        position[0] = t->translation[0];
        position[1] = t->translation[1];
        position[2] = t->translation[2];
        (jniEnv)->ReleaseFloatArrayElements( pos, position, 0);
        jfloat *rotation = (jniEnv)->GetFloatArrayElements( rot, 0);
        rotation[0] = t->rotation[0];
        rotation[1] = t->rotation[1];
        rotation[2] = t->rotation[2];
        rotation[3] = t->rotation[3];
        (jniEnv)->ReleaseFloatArrayElements( rot, rotation, 0);
        jlong *timestamp1 = (jniEnv)->GetLongArrayElements( timestamp, 0);
        timestamp1[0] = t->ts;
        (jniEnv)->ReleaseLongArrayElements( timestamp, timestamp1, 0);
    }
}