#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <iostream>
#include <vulkan/vulkan.h>

// Debug callback for validation layer
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData) {
    // example: see https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
    }

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

#endif  // !EVENT_HANDLERS_H