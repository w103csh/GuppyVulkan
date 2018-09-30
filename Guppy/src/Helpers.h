
#ifndef HELPERS_H
#define HELPERS_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <sstream>
#include <vector>

#include "Vertex.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

namespace std
{
    // Hash function for Vertex class
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                     (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}


namespace vk {
    inline VkResult assert_success(VkResult res) {
        if (res != VK_SUCCESS) {
            std::stringstream ss;
            ss << "VkResult " << res << " returned";
            throw std::runtime_error(ss.str());
        }
        return res;
    }
}

struct ImageResource {
    VkFormat format;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

struct ShaderResources {
    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;
};

struct UniformBufferResources {
    VkDescriptorBufferInfo info;
    VkBuffer buffer;
    VkDeviceMemory memory;
};

struct BufferResource {
    VkBuffer buffer;
    VkDeviceMemory memory;
};

//struct VertexResource {
//    VkBuffer buffer;
//    VkDeviceMemory memory;
//};
//
//struct IndexResource {
//    VkBuffer buffer;
//    VkDeviceMemory memory;
//};

struct CommandData {
    VkPhysicalDeviceMemoryProperties mem_props;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
    uint32_t transfer_queue_family;
    std::vector<VkQueue> queues;
    std::vector<VkCommandPool> cmd_pools;
    std::vector<VkCommandBuffer> cmds;
};

#endif // !HELPERS_H