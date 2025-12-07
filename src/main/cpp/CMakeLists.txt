cmake_minimum_required(VERSION 3.22.1)

# Set source files
set(OPENXR_SOURCES
    openxr/openxr_api.cpp
    openxr/instance.cpp
    openxr/session.cpp
    openxr/space.cpp
    openxr/swapchain.cpp
    openxr/input.cpp
    openxr/event.cpp
    openxr/frame.cpp
)

set(PLATFORM_SOURCES
    platform/android_platform.cpp
    platform/display_manager.cpp
    platform/input_manager.cpp
    platform/frame_sync.cpp
)

set(QUALCOMM_SOURCES
    qualcomm/xr2_platform.cpp
    qualcomm/qvr_api_wrapper.cpp
    qualcomm/spaces_sdk_wrapper.cpp
)

set(UTILS_SOURCES
    utils/logger.cpp
    utils/error_handler.cpp
    utils/memory_manager.cpp
)

set(JNI_SOURCES
    jni/jni_bridge.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/openxr/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/qualcomm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc
)

# Create library
add_library(xrruntime SHARED
    main.cpp
    ${OPENXR_SOURCES}
    ${PLATFORM_SOURCES}
    ${QUALCOMM_SOURCES}
    ${UTILS_SOURCES}
    ${JNI_SOURCES}
)

# Link libraries
target_link_libraries(xrruntime
    android
    log
    EGL
    GLESv3
    # OpenXR Loader will be linked dynamically
    # Qualcomm libraries will be linked dynamically
)

# Set output directory
set_target_properties(xrruntime PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)

