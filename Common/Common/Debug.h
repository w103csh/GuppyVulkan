/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <array>
#include <string>
#include <vulkan/vulkan.hpp>

namespace Debug {

// clang-format off
constexpr vk::DebugReportObjectTypeEXT getDebugReportObjectTypeEXT(const vk::ObjectType type) {
    switch (type) {
        case vk::ObjectType::eInstance                      : return vk::DebugReportObjectTypeEXT::eInstance;
        case vk::ObjectType::ePhysicalDevice                : return vk::DebugReportObjectTypeEXT::ePhysicalDevice;
        case vk::ObjectType::eDevice                        : return vk::DebugReportObjectTypeEXT::eDevice;
        case vk::ObjectType::eQueue                         : return vk::DebugReportObjectTypeEXT::eQueue;
        case vk::ObjectType::eSemaphore                     : return vk::DebugReportObjectTypeEXT::eSemaphore;
        case vk::ObjectType::eCommandBuffer                 : return vk::DebugReportObjectTypeEXT::eCommandBuffer;
        case vk::ObjectType::eFence                         : return vk::DebugReportObjectTypeEXT::eFence;
        case vk::ObjectType::eDeviceMemory                  : return vk::DebugReportObjectTypeEXT::eDeviceMemory;
        case vk::ObjectType::eBuffer                        : return vk::DebugReportObjectTypeEXT::eBuffer;
        case vk::ObjectType::eImage                         : return vk::DebugReportObjectTypeEXT::eImage;
        case vk::ObjectType::eEvent                         : return vk::DebugReportObjectTypeEXT::eEvent;
        case vk::ObjectType::eQueryPool                     : return vk::DebugReportObjectTypeEXT::eQueryPool;
        case vk::ObjectType::eBufferView                    : return vk::DebugReportObjectTypeEXT::eBufferView;
        case vk::ObjectType::eImageView                     : return vk::DebugReportObjectTypeEXT::eImageView;
        case vk::ObjectType::eShaderModule                  : return vk::DebugReportObjectTypeEXT::eShaderModule;
        case vk::ObjectType::ePipelineCache                 : return vk::DebugReportObjectTypeEXT::ePipelineCache;
        case vk::ObjectType::ePipelineLayout                : return vk::DebugReportObjectTypeEXT::ePipelineLayout;
        case vk::ObjectType::eRenderPass                    : return vk::DebugReportObjectTypeEXT::eRenderPass;
        case vk::ObjectType::ePipeline                      : return vk::DebugReportObjectTypeEXT::ePipeline;
        case vk::ObjectType::eDescriptorSetLayout           : return vk::DebugReportObjectTypeEXT::eDescriptorSetLayout;
        case vk::ObjectType::eSampler                       : return vk::DebugReportObjectTypeEXT::eSampler;
        case vk::ObjectType::eDescriptorPool                : return vk::DebugReportObjectTypeEXT::eDescriptorPool;
        case vk::ObjectType::eDescriptorSet                 : return vk::DebugReportObjectTypeEXT::eDescriptorSet;
        case vk::ObjectType::eFramebuffer                   : return vk::DebugReportObjectTypeEXT::eFramebuffer;
        case vk::ObjectType::eCommandPool                   : return vk::DebugReportObjectTypeEXT::eCommandPool;
        case vk::ObjectType::eSurfaceKHR                    : return vk::DebugReportObjectTypeEXT::eSurfaceKHR;
        case vk::ObjectType::eSwapchainKHR                  : return vk::DebugReportObjectTypeEXT::eSwapchainKHR;
        case vk::ObjectType::eDebugReportCallbackEXT        : return vk::DebugReportObjectTypeEXT::eDebugReportCallbackEXT;
        case vk::ObjectType::eDisplayKHR                    : return vk::DebugReportObjectTypeEXT::eDisplayKHR;
        case vk::ObjectType::eDisplayModeKHR                : return vk::DebugReportObjectTypeEXT::eDisplayModeKHR;
        case vk::ObjectType::eObjectTableNVX                : return vk::DebugReportObjectTypeEXT::eObjectTableNVX;
        case vk::ObjectType::eIndirectCommandsLayoutNVX     : return vk::DebugReportObjectTypeEXT::eIndirectCommandsLayoutNVX;
        case vk::ObjectType::eValidationCacheEXT            : return vk::DebugReportObjectTypeEXT::eValidationCacheEXT;
        case vk::ObjectType::eSamplerYcbcrConversion        : return vk::DebugReportObjectTypeEXT::eSamplerYcbcrConversion;
        case vk::ObjectType::eDescriptorUpdateTemplate      : return vk::DebugReportObjectTypeEXT::eDescriptorUpdateTemplate;
        case vk::ObjectType::eAccelerationStructureNV       : return vk::DebugReportObjectTypeEXT::eAccelerationStructureNV;
        // case vk::ObjectType::eDebugReport                   : return vk::DebugReportObjectTypeEXT::eDebugReport;
        // case vk::ObjectType::eValidationCache               : return vk::DebugReportObjectTypeEXT::eValidationCache;
        // case vk::ObjectType::eDescriptorUpdateTemplateKHR   : return vk::DebugReportObjectTypeEXT::eDescriptorUpdateTemplateKHR;
        // case vk::ObjectType::eSamplerYcbcrConversionKHR     : return vk::DebugReportObjectTypeEXT::eSamplerYcbcrConversionKHR;
        default: return vk::DebugReportObjectTypeEXT::eUnknown;
    }
}
// clang-format on

inline std::string makeName(const vk::ObjectType type, const char* name) {
    return std::string(name) + " " + vk::to_string(type);
}

class Base {
   public:
    void init(vk::Device& dev, bool enable) {
        dev_ = dev;
        enable_ = enable;
    }

    template <typename T>
    void setMarkerName(const T& object, const char* name) const {
        if (!enable_) return;
        dev_.debugMarkerSetObjectNameEXT({getDebugReportObjectTypeEXT(object.objectType),
                                          reinterpret_cast<const uint64_t&>(object),
                                          makeName(object.objectType, name).c_str()});
    }

    template <typename TObject, typename TTag>
    void setMarkerTag(const TObject& object, const char* name, const TTag& tag) const {
        if (!enable_) return;
        dev_.debugMarkerSetObjectTagEXT({getDebugReportObjectTypeEXT(object.objectType),
                                         reinterpret_cast<const uint64_t&>(object),
                                         makeName(object.objectType, name).c_str(), sizeof(tag), &tag});
    }

   private:
    vk::Device dev_;
    bool enable_;
};

}  // namespace Debug

#endif  // !DEBUG_H