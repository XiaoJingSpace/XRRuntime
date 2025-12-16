cmake_minimum_required(VERSION 3.22.1)

# Project name (required by CMake)
project(XRRuntime)

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

# Check for OpenXR headers
set(OPENXR_INCLUDE_DIR "")
set(OPENXR_HEADER_FILE "")

# Get project root directory (CMakeLists.txt is in src/main/cpp, so go up 3 levels)
# CMAKE_CURRENT_SOURCE_DIR = D:/WorkSpace/XRRuntimeStudy/src/main/cpp
# Level 1: D:/WorkSpace/XRRuntimeStudy/src/main
# Level 2: D:/WorkSpace/XRRuntimeStudy/src
# Level 3: D:/WorkSpace/XRRuntimeStudy (project root)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" DIRECTORY)
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" DIRECTORY)

# Try to find OpenXR headers in multiple possible locations
set(OPENXR_PATH1 "${PROJECT_ROOT}/include/openxr/openxr.h")
set(OPENXR_PATH2 "${PROJECT_ROOT}/libs/openxr/include/openxr/openxr.h")

if(EXISTS "${OPENXR_PATH1}")
    set(OPENXR_INCLUDE_DIR "${PROJECT_ROOT}/include")
    set(OPENXR_HEADER_FILE "${OPENXR_PATH1}")
    message(STATUS "Checking OpenXR path 1: ${OPENXR_PATH1} - FOUND")
elseif(EXISTS "${OPENXR_PATH2}")
    set(OPENXR_INCLUDE_DIR "${PROJECT_ROOT}/libs/openxr/include")
    set(OPENXR_HEADER_FILE "${OPENXR_PATH2}")
    message(STATUS "Checking OpenXR path 2: ${OPENXR_PATH2} - FOUND")
else()
    message(STATUS "Checking OpenXR path 1: ${OPENXR_PATH1} - NOT FOUND")
    message(STATUS "Checking OpenXR path 2: ${OPENXR_PATH2} - NOT FOUND")
    message(STATUS "Project root: ${PROJECT_ROOT}")
endif()

# Verify OpenXR headers are found
if(NOT OPENXR_HEADER_FILE)
    message(FATAL_ERROR "
================================================================================
OpenXR SDK headers not found!

Please set up OpenXR SDK Source headers. Options:

Option 1: Copy headers to include/openxr/
  - Clone: git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
  - Copy: cp -r OpenXR-SDK-Source/include/openxr include/

Option 2: Copy headers to libs/openxr/include/openxr/
  - Clone: git clone https://github.com/KhronosGroup/OpenXR-SDK-Source.git
  - Copy: cp -r OpenXR-SDK-Source/include/openxr libs/openxr/include/

See docs/OPENXR_SDK_SETUP.md for detailed instructions.
================================================================================
")
endif()

message(STATUS "Found OpenXR headers at: ${OPENXR_INCLUDE_DIR}")

# Check for Qualcomm SDK headers (optional, but warn if missing)
set(QVR_INCLUDE_DIR "${PROJECT_ROOT}/SnapdragonXR-SDK-source.rel.4.0.5/3rdparty/qvr/inc")
if(NOT EXISTS "${QVR_INCLUDE_DIR}")
    message(WARNING "QVR headers not found at: ${QVR_INCLUDE_DIR}")
    message(WARNING "Some features may not work without QVR SDK")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENXR_INCLUDE_DIR}
    ${PROJECT_ROOT}/include
    ${PROJECT_ROOT}/libs/openxr/include
    ${PROJECT_ROOT}/libs/qualcomm/include
    ${QVR_INCLUDE_DIR}
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

