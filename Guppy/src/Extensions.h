/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <iostream>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Shell.h"

namespace ext {

static bool debugMakerActive = false;

// DebugMarker
static PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;
static PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
static PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = VK_NULL_HANDLE;
static PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = VK_NULL_HANDLE;
static PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;
// DebugReport
static PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = VK_NULL_HANDLE;
static PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = VK_NULL_HANDLE;
// DebugUtils
static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = VK_NULL_HANDLE;
static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = VK_NULL_HANDLE;
// TransformFeedback
static PFN_vkCmdBindTransformFeedbackBuffersEXT vkCmdBindTransformFeedbackBuffers = VK_NULL_HANDLE;

inline void DebugMessage(std::string type, const VkResult& result, const char* name) {
    static bool printSuccess = false;

    if (printSuccess && result == VK_SUCCESS)
        std::cout << "setObjectName succeeded for: " << name << " | VkResult" << result << " returned" << std::endl;
    else
        std::cout << "setObjectName failed for: " << name << " | VkResult" << result << " returned" << std::endl;
}

static void CreateDeviceEXTs(const Shell::Context& ctx) {
    // TODO: If retrieval of the function pointers fail the flags should be updated.
    if (ctx.debugMarkersEnabled) {
        vkDebugMarkerSetObjectTag =
            (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(ctx.dev, "vkDebugMarkerSetObjectTagEXT");
        assert(vkDebugMarkerSetObjectTag != VK_NULL_HANDLE);
        vkDebugMarkerSetObjectName =
            (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(ctx.dev, "vkDebugMarkerSetObjectNameEXT");
        assert(vkDebugMarkerSetObjectName != VK_NULL_HANDLE);
        vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(ctx.dev, "vkCmdDebugMarkerBeginEXT");
        assert(vkCmdDebugMarkerBegin != VK_NULL_HANDLE);
        vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(ctx.dev, "vkCmdDebugMarkerEndEXT");
        assert(vkCmdDebugMarkerEnd != VK_NULL_HANDLE);
        vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(ctx.dev, "vkCmdDebugMarkerInsertEXT");
        assert(vkCmdDebugMarkerInsert != VK_NULL_HANDLE);

        debugMakerActive = true;

        // TODO: log VK_ERROR_EXTENSION_NOT_PRESENT message.
    }
    if (ctx.transformFeedbackEnabled) {
        assert(false);  // You probably don't want to use transform feedback see below.

        /**
         * I started trying to implement a simple a vertex update on the gpu using transform feedback while following along
         * with the shader book I'm reading. As I was doing this it was becoming increasingly clear that this was going to be
         * difficult due to the lack of working examples I could find online. That is when I stumbled upon this blog post by
         * a Khronos employee: http://jason-blog.jlekstrand.net/2018/10/transform-feedback-is-terrible-so-why.html. After
         * reading this I will take his adivce and implement the update on the gpu using compute shaders, but I am going to
         * leave this code here because I liked the changes I made to the extensions code in this file, and the
         * Shell/Settings code elsewhere. Also, it should be a good reminder to not just following things blindly.
         */

        vkCmdBindTransformFeedbackBuffers =
            (PFN_vkCmdBindTransformFeedbackBuffersEXT)vkGetDeviceProcAddr(ctx.dev, "vkCmdBindTransformFeedbackBuffersEXT");
        assert(vkCmdBindTransformFeedbackBuffers != VK_NULL_HANDLE);

        // TODO: log VK_ERROR_EXTENSION_NOT_PRESENT message.
    }
}

static void CreateInstanceEXTs(const VkInstance& instance, bool validate) {
    if (validate) {
        // Utils messenger
        vkCreateDebugUtilsMessengerEXT =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        assert(vkCreateDebugUtilsMessengerEXT != VK_NULL_HANDLE);
        vkDestroyDebugUtilsMessengerEXT =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        assert(vkDestroyDebugUtilsMessengerEXT != VK_NULL_HANDLE);
    }

    // TODO: log VK_ERROR_EXTENSION_NOT_PRESENT message.
}

static inline void DebugMarkerSetObjectName(const VkDevice& dev, const uint64_t& object,
                                            VkDebugReportObjectTypeEXT objectType, const char* name) {
    VkResult res = VK_INCOMPLETE;
    if (debugMakerActive) {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.object = object;
        nameInfo.pObjectName = name;
        res = vkDebugMarkerSetObjectName(dev, &nameInfo);
        DebugMessage("SetObjectName", res, name);
    }
}

static void DebugMarkerSetObjectTag(const VkDevice& dev, const uint64_t& object,
                                    const VkDebugReportObjectTypeEXT& objectType, const uint64_t& tagName,
                                    const size_t& tagSize, const void* tag) {
    VkResult res = VK_INCOMPLETE;
    if (debugMakerActive) {
        VkDebugMarkerObjectTagInfoEXT tagInfo = {};
        tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
        tagInfo.objectType = objectType;
        tagInfo.object = object;
        tagInfo.tagName = tagName;
        tagInfo.tagSize = tagSize;
        tagInfo.pTag = tag;
        vkDebugMarkerSetObjectTag(dev, &tagInfo);
        DebugMessage("setObjectTag", res, "");
    }
}

static void DebugMarkerBegin(const VkCommandBuffer& cmd, const char* pMarkerName, glm::vec4 color) {
    // Check for valid function pointer (may not be present if not running in a debugging application)
    VkResult res = VK_INCOMPLETE;
    if (debugMakerActive) {
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pMarkerName = pMarkerName;
        vkCmdDebugMarkerBegin(cmd, &markerInfo);
        DebugMessage("DebugMarkerBegin", res, pMarkerName);
    }
}

static void DebugMarkerInsert(const VkCommandBuffer& cmdbuffer, const char* markerName, glm::vec4 color) {
    // Check for valid function pointer (may not be present if not running in a debugging application)
    VkResult res = VK_INCOMPLETE;
    if (debugMakerActive) {
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pMarkerName = markerName;
        vkCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
        DebugMessage("DebugMarkerInsert", res, markerName);
    }
}

static void DebugMarkerEnd(const VkCommandBuffer& cmdBuffer) {
    // Check for valid function (may not be present if not runnin in a debugging application)
    VkResult res = VK_INCOMPLETE;
    if (vkCmdDebugMarkerEnd) {
        vkCmdDebugMarkerEnd(cmdBuffer);
        DebugMessage("DebugMarkerEnd", res, "");
    }
}

};  // namespace ext

#endif  // EXTENSIONS_H