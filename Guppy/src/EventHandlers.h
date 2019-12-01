/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <sstream>
#include <vulkan/vulkan.h>

#include "Shell.h"

namespace events {

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                          VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                          const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                          void *pUserData) {
    Shell *shell = reinterpret_cast<Shell *>(pUserData);

    std::stringstream ss;
    ss << pCallbackData->pMessageIdName << " (";

    Shell::LogPriority priority;
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            priority = Shell::LogPriority::LOG_DEBUG;
            ss << "VERBOSE/";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            priority = Shell::LogPriority::LOG_INFO;
            ss << "INFO/";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            priority = Shell::LogPriority::LOG_WARN;
            ss << "WARN/";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            priority = Shell::LogPriority::LOG_ERR;
            ss << "ERROR/";
            break;
        default:
            assert(false && "Not sure if messageSeverity is a bitmask or not");
            return VK_TRUE;
    }
    switch (messageTypes) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: {
            ss << "GENERAL";
        } break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: {
            ss << "VALIDATION";
        } break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: {
            ss << "PERFORMANCE";
        } break;
        default:
            assert(false && "Not sure if messageTypes is a bitmask or not");
            return VK_TRUE;
    }

    ss << "): msgNum: " << pCallbackData->messageIdNumber << " - " << pCallbackData->pMessage;

    ss << std::endl
       << "\tQueue labels: " << pCallbackData->queueLabelCount
       << " - Command buffer labels: " << pCallbackData->cmdBufLabelCount << " - Objects: " << pCallbackData->objectCount
       << std::endl;

    shell->log(priority, ss.str().c_str());

    return VK_FALSE;
}

}  // namespace events

#endif  // !EVENT_HANDLERS_H