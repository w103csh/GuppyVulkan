
#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <iostream>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace ext {

static bool printSuccess;
static bool active;
static bool extensionPresent;
static PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag;
static PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName;
static PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin;
static PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd;
static PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert;

inline void debugMessage(std::string type, const VkResult& result, const char* name) {
    if (printSuccess && result == VK_SUCCESS)
        std::cout << "setObjectName succeeded for: " << name << " | VkResult" << result << " returned" << std::endl;
    else
        std::cout << "setObjectName failed for: " << name << " | VkResult" << result << " returned" << std::endl;
}

// Get function pointers for the debug report extensions from the device
static void CreateDebugMarkerEXTs(VkDevice device, VkPhysicalDevice physicalDevice) {
    // Check if the debug marker extension is present (which is the case if run from a graphics debugger)
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    for (auto extension : extensions) {
        if (strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
            extensionPresent = true;
            break;
        }
    }

    if (extensionPresent) {
        // The debug marker extension is not part of the core, so function pointers need to be loaded manually
        vkDebugMarkerSetObjectTag =
            (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
        vkDebugMarkerSetObjectName =
            (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
        vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
        vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
        vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
        // Set flag if at least one function pointer is present
        active = (vkDebugMarkerSetObjectName != VK_NULL_HANDLE);
        // std::cout << "1: " << (vkDebugMarkerSetObjectTag != VK_NULL_HANDLE) << std::endl;
        // std::cout << "2: " << (vkDebugMarkerSetObjectName != VK_NULL_HANDLE) << std::endl;
        // std::cout << "3: " << (vkCmdDebugMarkerBegin != VK_NULL_HANDLE) << std::endl;
        // std::cout << "4: " << (vkCmdDebugMarkerEnd != VK_NULL_HANDLE) << std::endl;
        // std::cout << "5: " << (vkCmdDebugMarkerInsert != VK_NULL_HANDLE) << std::endl;
    } else {
        std::cout << "Warning: " << VK_EXT_DEBUG_MARKER_EXTENSION_NAME << " not present, debug markers are disabled.";
        std::cout << "Try running from inside a Vulkan graphics debugger (e.g. RenderDoc)" << std::endl;
    }
}

// static void DestroyDebugMarkerEXTs(VkDevice device, VkPhysicalDevice physicalDevice) {
//    if (active) {
//        if (vkDebugMarkerSetObjectTag != VK_NULL_HANDLE) {
//            auto func = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
//        }
//        if (vkDebugMarkerSetObjectName != VK_NULL_HANDLE) {
//            auto func = vkDebugMarkerSetObjectName =
//                (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
//        }
//        if (vkCmdDebugMarkerBegin != VK_NULL_HANDLE) {
//            auto func = vkCmdDebugMarkerBegin =
//                (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
//        }
//        if (vkCmdDebugMarkerEnd != VK_NULL_HANDLE) {
//            auto func = vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device,
//            "vkCmdDebugMarkerEndEXT");
//        }
//        if (vkCmdDebugMarkerInsert != VK_NULL_HANDLE) {
//            auto func = vkCmdDebugMarkerInsert =
//                (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
//        }
//    }
//}

static inline void DebugMarkerSetObjectName(const VkDevice& dev, const uint64_t& object,
                                            VkDebugReportObjectTypeEXT objectType, const char* name) {
    VkResult res = VK_INCOMPLETE;
    if (active) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.object = object;
        nameInfo.pObjectName = name;
        res = vkDebugMarkerSetObjectName(dev, &nameInfo);
        debugMessage("SetObjectName", res, name);
    }
}

static void DebugMarkerSetObjectTag(const VkDevice& dev, const uint64_t& object,
                                    const VkDebugReportObjectTypeEXT& objectType, const uint64_t& tagName,
                                    const size_t& tagSize, const void* tag) {
    VkResult res = VK_INCOMPLETE;
    if (active) {
        VkDebugMarkerObjectTagInfoEXT tagInfo = {};
        tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
        tagInfo.objectType = objectType;
        tagInfo.object = object;
        tagInfo.tagName = tagName;
        tagInfo.tagSize = tagSize;
        tagInfo.pTag = tag;
        vkDebugMarkerSetObjectTag(dev, &tagInfo);
        debugMessage("setObjectTag", res, "");
    }
}

static void DebugMarkerBegin(const VkCommandBuffer& cmd, const char* pMarkerName, glm::vec4 color) {
    // Check for valid function pointer (may not be present if not running in a debugging application)
    VkResult res = VK_INCOMPLETE;
    if (active) {
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pMarkerName = pMarkerName;
        vkCmdDebugMarkerBegin(cmd, &markerInfo);
        debugMessage("DebugMarkerBegin", res, pMarkerName);
    }
}

static void DebugMarkerInsert(const VkCommandBuffer& cmdbuffer, const char* markerName, glm::vec4 color) {
    // Check for valid function pointer (may not be present if not running in a debugging application)
    VkResult res = VK_INCOMPLETE;
    if (active) {
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pMarkerName = markerName;
        vkCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
        debugMessage("DebugMarkerInsert", res, markerName);
    }
}

static void DebugMarkerEnd(const VkCommandBuffer& cmdBuffer) {
    // Check for valid function (may not be present if not runnin in a debugging application)
    VkResult res = VK_INCOMPLETE;
    if (vkCmdDebugMarkerEnd) {
        vkCmdDebugMarkerEnd(cmdBuffer);
        debugMessage("DebugMarkerEnd", res, "");
    }
}

// Unfortunately, because this function is an extension function, it is not automatically loaded. We have to look up
// its address ourselves using vkGetInstanceProcAddr.
// (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers)
static VkResult CreateDebugReportCallbackEXT(const VkInstance instance,
                                             const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugReportCallbackEXT(const VkInstance instance, VkDebugReportCallbackEXT& callback,
                                          const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

// Unfortunately, because this function is an extension function, it is not automatically loaded. We have to look up
// its address ourselves using vkGetInstanceProcAddr.
// (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers)
static VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
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

};  // namespace ext

#endif  // EXTENSIONS_H