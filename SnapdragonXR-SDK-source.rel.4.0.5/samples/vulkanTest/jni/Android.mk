#=============================================================================
#                  Copyright (c) 2016 QUALCOMM Technologies Inc.
#                              All Rights Reserved.
#
#=============================================================================
LOCAL_PATH := $(call my-dir)
MY_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)
LOCAL_MODULE := libsxrapi

LOCAL_SRC_FILES := $(MY_PATH)/../.sxrLibs/$(APP_OPTIM)/jni/$(TARGET_ARCH_ABI)/libsxrapi.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

SDK_ROOT_PATH := $(LOCAL_PATH)/../../..

SXR_SXRAPI_PATH = : $(SDK_ROOT_PATH)/sxrApi
SXR_FRAMEWORK_PATH = : $(SDK_ROOT_PATH)/framework
VULKAN_PATH := $(SDK_ROOT_PATH)/3rdparty
GLM_ROOT_PATH := $(SDK_ROOT_PATH)/3rdparty/glm-0.9.7.0
# TINYOBJ_ROOT_PATH := $(SDK_ROOT_PATH)/3rdparty/tinyobj
SXR_API_ROOT_PATH := $(SDK_ROOT_PATH)/sxrApi/public
VULKAN_FRAMEWORK_PATH = : $(LOCAL_PATH)/framework

LOCAL_MODULE    := vulkantest
LOCAL_ARM_MODE	:= arm

LOCAL_C_INCLUDES  := $(VULKAN_PATH) $(GLM_ROOT_PATH) $(SXR_SXRAPI_PATH) $(SXR_API_ROOT_PATH) $(VULKAN_FRAMEWORK_PATH) $(SXR_FRAMEWORK_PATH)

LOCAL_SRC_FILES += ./framework/BufferObject.cpp
LOCAL_SRC_FILES += ./framework/MemoryAllocator.cpp
LOCAL_SRC_FILES += ./framework/MeshObject.cpp
LOCAL_SRC_FILES += ./framework/TextureObject.cpp
LOCAL_SRC_FILES += ./framework/VkSampleFramework.cpp
LOCAL_SRC_FILES += sample.cpp

LOCAL_LDLIBS    := -llog -landroid -lvulkan
LOCAL_SHARED_LIBRARIES := libsxrapi

LOCAL_WHOLE_STATIC_LIBRARIES += android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
