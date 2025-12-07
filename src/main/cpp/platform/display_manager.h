#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <openxr/openxr.h>
#include <vector>

// Swapchain image creation
bool CreateSwapchainImages(uint32_t width, uint32_t height, int64_t format, 
                           uint32_t imageCount, void* images);

void DestroySwapchainImages(void* images, uint32_t imageCount);

// Supported formats
bool GetSupportedSwapchainFormats(std::vector<int64_t>& formats);

// View configuration
bool GetXR2ViewConfigurationViews(XrViewConfigurationView* views, uint32_t count);

#endif // DISPLAY_MANAGER_H

