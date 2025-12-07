#ifndef SPACES_SDK_WRAPPER_H
#define SPACES_SDK_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

// Snapdragon Spaces SDK wrapper functions

// SDK lifecycle
bool InitializeSpacesSDK();
void ShutdownSpacesSDK();

// Hand tracking
bool GetSpacesHandPose(uint32_t handIndex, float* position, float* rotation);

// Scene understanding
bool GetSpacesSceneMesh(uint32_t* vertexCount, float** vertices, 
                       uint32_t* indexCount, uint32_t** indices);

#endif // SPACES_SDK_WRAPPER_H

