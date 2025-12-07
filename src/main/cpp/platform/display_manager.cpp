#include "display_manager.h"
#include "qualcomm/xr2_platform.h"
#include "utils/logger.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <cstring>

bool CreateSwapchainImages(uint32_t width, uint32_t height, int64_t format, 
                           uint32_t imageCount, void* images) {
    LOGI("Creating swapchain images: %ux%u, format: %lld, count: %u", 
         width, height, format, imageCount);
    
    // Create OpenGL ES textures for swapchain images
    // This is a simplified implementation
    GLuint* textures = static_cast<GLuint*>(images);
    
    glGenTextures(imageCount, textures);
    
    for (uint32_t i = 0; i < imageCount; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    LOGI("Swapchain images created successfully");
    return true;
}

void DestroySwapchainImages(void* images, uint32_t imageCount) {
    if (!images || imageCount == 0) {
        return;
    }
    
    GLuint* textures = static_cast<GLuint*>(images);
    glDeleteTextures(imageCount, textures);
    
    LOGI("Swapchain images destroyed");
}

bool GetSupportedSwapchainFormats(std::vector<int64_t>& formats) {
    // OpenGL ES formats
    formats.push_back(GL_RGBA8);
    formats.push_back(GL_RGB8);
    formats.push_back(GL_RGBA16F);
    formats.push_back(GL_RGB16F);
    
    return true;
}

bool GetXR2ViewConfigurationViews(XrViewConfigurationView* views, uint32_t count) {
    if (!views || count < 2) {
        return false;
    }
    
    // Get display properties from XR2 platform
    uint32_t recommendedWidth, recommendedHeight;
    uint32_t maxWidth, maxHeight;
    
    if (!GetXR2DisplayProperties(&recommendedWidth, &recommendedHeight, 
                                  &maxWidth, &maxHeight)) {
        return false;
    }
    
    // Set view properties for both eyes (binocular rendering)
    // Each eye gets its own independent render target
    for (uint32_t i = 0; i < count && i < 2; ++i) {
        views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        views[i].recommendedImageRectWidth = recommendedWidth;
        views[i].recommendedImageRectHeight = recommendedHeight;
        views[i].maxImageRectWidth = maxWidth;
        views[i].maxImageRectHeight = maxHeight;
        views[i].recommendedSwapchainSampleCount = 1;
        views[i].maxSwapchainSampleCount = 4;
    }
    
    // If more than 2 views requested, duplicate the configuration
    for (uint32_t i = 2; i < count; ++i) {
        views[i] = views[i % 2];
    }
    
    return true;
}

