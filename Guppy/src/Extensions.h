#pragma once

#include <vulkan/vulkan.h>

// Unfortunately, because this function is an extension function, it is not automatically loaded. We have to look up
// its address ourselves using vkGetInstanceProcAddr. (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers)
static VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
