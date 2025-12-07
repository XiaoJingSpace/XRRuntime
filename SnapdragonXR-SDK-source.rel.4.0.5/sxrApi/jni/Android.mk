#=============================================================================
#                  Copyright (c) 2020 QUALCOMM Technologies Inc.
#                              All Rights Reserved.
#
#=============================================================================

LOCAL_PATH := $(call my-dir)

# Use this to enable/disable Motion To Photon Test
ENABLE_MOTION_PHOTON_TEST := true

# Use this to enable/disable Telemetry profiling support
ENABLE_TELEMETRY := false

# Use this to enable/disable ATrace profiling support
ENABLE_ATRACE := true

# Use this to enable/disable Motion Vectors
ENABLE_MOTION_VECTORS := true

# Use this to enable/disable XR Casting support
ENABLE_XR_CASTING := true

# Use this to enable/disable Camera Pass Through
ENABLE_CAMERA := true

# Use this to enable/disable Custom Mesh feature
ENABLE_CUSTOM_MESH := true

# Use this to enable/disable remote rendering(warp on hmd mode) support
ENABLE_REMOTE_XR_RENDERING := true

ifeq ($(ENABLE_TELEMETRY),true)
include $(CLEAR_VARS)
LOCAL_MODULE    := libTelemetry
LOCAL_SRC_FILES := ../../3rdparty/telemetry/lib/rad_tm_android_arm.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifeq ($(ENABLE_MOTION_VECTORS),true)
include $(CLEAR_VARS)
LOCAL_MODULE    := libStaticMotionVector
LOCAL_SRC_FILES := ../../3rdparty/motionengine/lib/$(TARGET_ARCH_ABI)/libMotionEngine.a
include $(PREBUILT_STATIC_LIBRARY)
endif

SDK_ROOT_PATH := $(LOCAL_PATH)/../..

SVR_FRAMEWORK_PATH		:= $(SDK_ROOT_PATH)/framework
GLM_ROOT_PATH			:= $(SDK_ROOT_PATH)/3rdparty/glm-0.9.7.0
TINYOBJ_ROOT_PATH		:= $(SDK_ROOT_PATH)/3rdparty/tinyobj
CJSON_ROOT_PATH			:= $(SDK_ROOT_PATH)/3rdparty/cJSON
SVR_API_PATH			:= $(SDK_ROOT_PATH)/sxrApi
SVR_API_PUBLIC_PATH		:= $(SDK_ROOT_PATH)/sxrApi/public
SVR_API_PRIVATE_PATH	:= $(SDK_ROOT_PATH)/sxrApi/private
SVR_API_SVRCONTROLLER_PATH := $(SVR_API_PRIVATE_PATH)/ControllerManager/inc/
SVR_API_SVRCONTROLLER_UTIL_PATH := $(SVR_API_PRIVATE_PATH)/SvrControllerUtil/jni/inc/
SVR_API_SVRSERVICECLIENT_PATH := $(SVR_API_PRIVATE_PATH)/SvrServiceClient/inc/
TELEMETRY_ROOT_PATH		:= $(SDK_ROOT_PATH)/3rdparty/telemetry
QCOM_ADSP_ROOT_PATH		:= $(SDK_ROOT_PATH)/3rdparty/qcom_adsp
QCOM_QVR_SERVICE_PATH	:= $(SDK_ROOT_PATH)/3rdparty/qvr
QCOM_AEE_ROOT_PATH		:= $(SDK_ROOT_PATH)/3rdparty/AEE
QCOM_CVC_ROOT_PATH		:= $(SDK_ROOT_PATH)/3rdparty/CVC
QCOM_DFS_ROOT_PATH		:= $(SDK_ROOT_PATH)/3rdparty/dfs
SVR_API_3DR_PATH        := $(SVR_API_PRIVATE_PATH)/Svr3dr/inc
SVR_API_ANCHORS_PATH    := $(SVR_API_PRIVATE_PATH)/SvrAnchors/inc

ifeq ($(ENABLE_MOTION_VECTORS),true)
MOTION_ENGINE_PATH	    := $(SDK_ROOT_PATH)/3rdparty/motionengine
endif

ifeq ($(ENABLE_CUSTOM_MESH),true)
SVR_API_SVRCUSTOM_MESH_PATH := $(SVR_API_PRIVATE_PATH)/SvrCustomMesh/inc/
endif

ifeq ($(ENABLE_CAMERA),true)
SVR_API_SVRCAMERA_PATH := $(SVR_API_PRIVATE_PATH)/SvrCamera/inc/

include $(CLEAR_VARS)
LOCAL_MODULE    := libStereoRectifyWrapper
LOCAL_SRC_FILES := $(QCOM_CVC_ROOT_PATH)/VIO/lib/$(TARGET_ARCH_ABI)/libStereoRectifyWrapper.so
LOCAL_EXPORT_C_INCLUDES := $(QCOM_DFS_ROOT_PATH)/inc $(QCOM_AEE_ROOT_PATH)/inc $(QCOM_CVC_ROOT_PATH)/VIO/inc
include $(PREBUILT_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)

ifeq ($(ENABLE_MOTION_PHOTON_TEST),true)
	LOCAL_CFLAGS += -DMOTION_TO_PHOTON_TEST
endif

ifeq ($(ENABLE_TELEMETRY),true)
	LOCAL_CFLAGS += -DSVR_PROFILING_ENABLED
	LOCAL_CFLAGS += -DSVR_PROFILE_TELEMETRY
endif

ifeq ($(ENABLE_ATRACE),true)
	LOCAL_CFLAGS += -DSVR_PROFILING_ENABLED
	LOCAL_CFLAGS += -DSVR_PROFILE_ATRACE
endif

ifeq ($(ENABLE_MOTION_VECTORS),true)
	LOCAL_CFLAGS += -DENABLE_MOTION_VECTORS
endif

ifeq ($(ENABLE_XR_CASTING),true)
	LOCAL_CFLAGS += -DENABLE_XR_CASTING
endif

ifeq ($(ENABLE_CAMERA),true)
	LOCAL_CFLAGS += -DENABLE_CAMERA
endif

ifeq ($(ENABLE_CUSTOM_MESH),true)
	LOCAL_CFLAGS += -DENABLE_CUSTOM_MESH
endif

ifeq ($(ENABLE_REMOTE_XR_RENDERING),true)
	LOCAL_CFLAGS += -DENABLE_REMOTE_XR_RENDERING
endif

LOCAL_MODULE := sxrapi
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(GLM_ROOT_PATH) $(TINYOBJ_ROOT_PATH) $(CJSON_ROOT_PATH) $(SVR_FRAMEWORK_PATH) $(SVR_API_PUBLIC_PATH) $(SVR_API_PRIVATE_PATH) $(TELEMETRY_ROOT_PATH)/include $(QCOM_ADSP_ROOT_PATH)/include  $(QCOM_AEE_ROOT_PATH)/inc $(QCOM_QVR_SERVICE_PATH)/inc $(SVR_API_SVRCONTROLLER_UTIL_PATH) $(SVR_API_SVRCONTROLLER_PATH) $(SVR_API_SVRSERVICECLIENT_PATH) $(SVR_API_PATH) $(SVR_API_3DR_PATH) $(SVR_API_ANCHORS_PATH)

ifeq ($(ENABLE_MOTION_VECTORS),true)
LOCAL_C_INCLUDES += $(MOTION_ENGINE_PATH)/inc
endif

ifeq ($(ENABLE_CAMERA),true)
LOCAL_C_INCLUDES += $(SVR_API_SVRCAMERA_PATH)
endif

ifeq ($(ENABLE_CUSTOM_MESH),true)
LOCAL_C_INCLUDES += $(SVR_API_SVRCUSTOM_MESH_PATH)
endif

#Make sure sxrApiVersion is compiled every time
FILE_LIST := $(wildcard $(LOCAL_PATH)/../private/*.cpp)

FILE_LIST += ../../framework/svrConfig.cpp ../../framework/svrContainers.cpp
FILE_LIST += ../../framework/svrGeometry.cpp ../../framework/svrShader.cpp ../../framework/svrUtil.cpp ../../framework/svrRenderTarget.cpp
FILE_LIST += ../../framework/svrKtxLoader.cpp ../../framework/svrProfile.cpp
FILE_LIST += ../../framework/svrCpuTimer.cpp ../../framework/svrGpuTimer.cpp
FILE_LIST += ../../framework/svrRenderExt.cpp

FILE_LIST += ../../3rdparty/tinyobj/tiny_obj_loader.cc
FILE_LIST += ../../3rdparty/cJSON/cJSON.c

FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/SvrControllerUtil/jni/src/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/ControllerManager/src/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/SvrServiceClient/src/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/Svr3dr/src/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/SvrAnchors/src/*.cpp)

ifeq ($(ENABLE_CAMERA),true)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/SvrCamera/src/*.cpp)
endif

ifeq ($(ENABLE_CUSTOM_MESH),true)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../private/SvrCustomMesh/src/*.cpp)
endif

ifeq ($(ENABLE_REMOTE_XR_RENDERING),true)
LOCAL_C_INCLUDES += $(SVR_API_PRIVATE_PATH)/RvrServiceClient/inc/
FILE_LIST += $(LOCAL_PATH)/../private/RvrServiceClient/src/SxrServiceClientManager.cpp
endif

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

#$(warning $(LOCAL_SRC_FILES))
.PHONY: $(LOCAL_PATH)/../private/sxrApiVersion.cpp

LOCAL_CPPFLAGS += -Wall -fno-strict-aliasing -Wno-unused-variable -Wno-unused-function

LOCAL_STATIC_LIBRARIES := cpufeatures

ifeq ($(ENABLE_CAMERA),true)
LOCAL_SHARED_LIBRARIES += libStereoRectifyWrapper
LOCAL_ARM_NEON := true
endif

ifeq ($(ENABLE_MOTION_VECTORS),true)
LOCAL_STATIC_LIBRARIES += libStaticMotionVector
endif

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3

APP_ALLOW_MISSING_DEPS=true

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
