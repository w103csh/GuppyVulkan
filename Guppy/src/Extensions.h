
#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <vulkan/vulkan.h>

/*
 * structure for comparing vulkan char arrays
 */
struct less_str {
    bool operator()(char const* a, char const* b) const { return std::strcmp(a, b) < 0; }
};

// Unfortunately, because this function is an extension function, it is not automatically loaded. We have to look up
// its address ourselves using vkGetInstanceProcAddr. (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers)
static VkResult CreateDebugReportCallbackEXT(const VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestoryDebugReportCallbackEXT(const VkInstance instance, VkDebugReportCallbackEXT& callback,
                                          const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

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

static void DestroyDebugUtilsMessengerEXT(const VkInstance instance, VkDebugUtilsMessengerEXT& messenger,
                                          const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, messenger, pAllocator);
    }
}

#endif // EXTENSIONS_H