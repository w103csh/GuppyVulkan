
#ifndef HELPERS_H
#define HELPERS_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <limits>
#include <sstream>
#include <type_traits>
#include <vector>

#include "Vertex.h"

namespace std {
// Hash function for Vertex class
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const &vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std

namespace vk {
inline VkResult assert_success(VkResult res) {
    if (res != VK_SUCCESS) {
        std::stringstream ss;
        ss << "VkResult " << res << " returned";
        throw std::runtime_error(ss.str());
    }
    return res;
}
}  // namespace vk

namespace helpers {

// Epsilon compare for floating point. Taken from here https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type almost_equal(T x, T y, int ulp) {
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x - y) <= std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
        // unless the result is subnormal
        || std::abs(x - y) < (std::numeric_limits<T>::min)();
}

static bool has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

static VkFormat find_supported_format(const VkPhysicalDevice &physical_dev, const std::vector<VkFormat> &candidates,
                               const VkImageTiling tiling, const VkFormatFeatureFlags features) {
    VkFormat format = {};
    for (VkFormat f : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_dev, f, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            format = f;
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            format = f;
            break;
        } else {
            throw std::runtime_error("failed to find supported format!");
        }
    }
    return format;
}

static VkFormat find_depth_format(const VkPhysicalDevice &physical_dev) {
    return find_supported_format(physical_dev, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                                 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

static bool memory_type_from_properties(const VkPhysicalDeviceMemoryProperties &mem_props, uint32_t typeBits, VkFlags requirements_mask,
                                 uint32_t *typeIndex) {
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((mem_props.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

static void execute_begin_command_buffer(const VkCommandBuffer &cmd) {
    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = NULL;

    vk::assert_success(vkBeginCommandBuffer(cmd, &cmd_buf_info));
}

static void execute_end_command_buffer(VkCommandBuffer &cmd) {
    vk::assert_success(vkEndCommandBuffer(cmd));
}

static VkDeviceSize create_buffer(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &mem_props, const VkDeviceSize &size,
                           const VkBufferUsageFlags &usage, const VkMemoryPropertyFlags &props, VkBuffer &buf,
                           VkDeviceMemory &buf_mem) {
    bool pass;

    // CREATE BUFFER

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = size;
    buf_info.usage = usage;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // TODO: what should these be ?
    // buf_info.queueFamilyIndexCount = 0;
    // buf_info.pQueueFamilyIndices = NULL;
    // buf_info.flags = 0;

    vk::assert_success(vkCreateBuffer(dev, &buf_info, NULL, &buf));

    // ALLOCATE DEVICE MEMORY

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(dev, buf, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    pass = memory_type_from_properties(mem_props, mem_reqs.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &alloc_info.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    /*
        It should be noted that in a real world application, you're not supposed to actually
        call vkAllocateMemory for every individual buffer. The maximum number of simultaneous
        memory allocations is limited by the maxMemoryAllocationCount physical device limit,
        which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080. The
        right way to allocate memory for a large number of objects at the same time is to create
        a custom allocator that splits up a single allocation among many different objects by
        using the offset parameters that we've seen in many functions.

        You can either implement such an allocator yourself, or use the VulkanMemoryAllocator
        library provided by the GPUOpen initiative. However, for this tutorial it's okay to
        use a separate allocation for every resource, because we won't come close to hitting
        any of these limits for now.

        The previous chapter already mentioned that you should allocate multiple resources like
        buffers from a single memory allocation, but in fact you should go a step further.
        Driver developers recommend that you also store multiple buffers, like the vertex and
        index buffer, into a single VkBuffer and use offsets in commands like
        vkCmdBindVertexBuffers. The advantage is that your data is more cache friendly in that
        case, because it's closer together. It is even possible to reuse the same chunk of
        memory for multiple resources if they are not used during the same render operations,
        provided that their data is refreshed, of course. This is known as aliasing and some
        Vulkan functions have explicit flags to specify that you want to do this.
    */

    vk::assert_success(vkAllocateMemory(dev, &alloc_info, NULL, &buf_mem));

    // BIND MEMORY

    vk::assert_success(vkBindBufferMemory(dev, buf, buf_mem, 0));

    return mem_reqs.size;
}

static void copy_buffer(VkCommandBuffer &cmd, const VkBuffer &src_buf, const VkBuffer &dst_buf, const VkDeviceSize &size) {
    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;  // Optional
    copy_region.dstOffset = 0;  // Optional
    copy_region.size = size;
    vkCmdCopyBuffer(cmd, src_buf, dst_buf, 1, &copy_region);
}

static void create_image(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &mem_props,
                  const std::vector<uint32_t> &queueFamilyIndices, const uint32_t width, uint32_t height, uint32_t mip_levels,
                  const VkSampleCountFlagBits &num_samples, const VkFormat &format, const VkImageTiling &tiling,
                  const VkImageUsageFlags &usage, const VkFlags &requirements_mask, VkImage &image, VkDeviceMemory &image_memory) {
    bool pass;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;  // param?
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = num_samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    image_info.pQueueFamilyIndices = queueFamilyIndices.data();

    /* Create image */
    vk::assert_success(vkCreateImage(dev, &image_info, NULL, &image));

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(dev, image, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    /* Use the memory properties to determine the type of memory required */
    pass = memory_type_from_properties(mem_props, mem_reqs.memoryTypeBits, requirements_mask, &alloc_info.memoryTypeIndex);
    assert(pass);

    /* Allocate memory */
    vk::assert_success(vkAllocateMemory(dev, &alloc_info, NULL, &image_memory));

    vk::assert_success(vkBindImageMemory(dev, image, image_memory, 0));
}

static void copy_buffer_to_image(VkCommandBuffer &cmd, const VkBuffer &src_buf, const VkImage &dst_img, const uint32_t &width,
                          const uint32_t &height) {
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, src_buf, dst_img,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // VK_IMAGE_USAGE_TRANSFER_DST_BIT
                           1, &region);
}

static void create_image_view(const VkDevice &device, const VkImage &image, const uint32_t &mip_levels, const VkFormat &format,
                       const VkImageAspectFlags &aspectFlags, const VkImageViewType &viewType, VkImageView &image_view) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;

    // swap chain is a 2D depth texture
    viewInfo.viewType = viewType;
    viewInfo.format = format;

    // defaults
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mip_levels;
    /*
        If you were working on a stereographic 3D application, then you would create a swap
        chain with multiple layers. You could then create multiple image views for each image
        representing the views for the left and right eyes by accessing different layers.
    */
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vk::assert_success(vkCreateImageView(device, &viewInfo, nullptr, &image_view));
}

static void transition_image_layout(VkCommandBuffer &cmd, VkImage &image, const uint32_t &mip_levels, const VkFormat &format,
                             const VkImageLayout &old_layout, const VkImageLayout &new_layout, VkPipelineStageFlags src_stages,
                             VkPipelineStageFlags dst_stages) {
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // ACCESS MASK

    bool layout_handled = true;

    switch (old_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        default:
            layout_handled = false;
            break;
    }

    switch (new_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        default:
            layout_handled = false;
            break;
    }

    // if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    //    barrier.srcAccessMask = 0;
    //    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    //    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    //} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    //    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    //    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    //    barrier.srcAccessMask = 0;
    //    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    //    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    //    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    //} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    //    barrier.srcAccessMask = 0;
    //    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    //    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //} else {
    //    throw std::invalid_argument("unsupported image layout transition!");
    //}

    if (!layout_handled) throw std::invalid_argument("unsupported image layout transition!");

    // ASPECT MASK

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkCmdPipelineBarrier(cmd, src_stages, dst_stages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

}  // namespace helpers

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

// struct VertexResource {
//    VkBuffer buffer;
//    VkDeviceMemory memory;
//};
//
// struct IndexResource {
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

#endif  // !HELPERS_H