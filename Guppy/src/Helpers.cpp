
#include "CmdBufHandler.h"
#include "Helpers.h"

namespace helpers {

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

VkFormat findSupportedFormat(const VkPhysicalDevice &phyDev, const std::vector<VkFormat> &candidates, const VkImageTiling tiling,
                             const VkFormatFeatureFlags features) {
    VkFormat format = VK_FORMAT_UNDEFINED;
    for (VkFormat f : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(phyDev, f, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            format = f;
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            format = f;
            break;
        }
    }
    return format;
}

VkFormat findDepthFormat(const VkPhysicalDevice &phyDev) {
    return findSupportedFormat(phyDev, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool getMemoryType(uint32_t typeBits, VkFlags reqMask, uint32_t *typeIndex) {
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < CmdBufHandler::mem_props().memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((CmdBufHandler::mem_props().memoryTypes[i].propertyFlags & reqMask) == reqMask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

VkDeviceSize createBuffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                          const VkMemoryPropertyFlags &props, VkBuffer &buff, VkDeviceMemory &mem) {
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = size;
    buffInfo.usage = usage;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // TODO: what should these be ?
    // buffInfo.queueFamilyIndexCount = 0;
    // buffInfo.pQueueFamilyIndices = nullptr;
    // buffInfo.flags = 0;

    vk::assert_success(vkCreateBuffer(dev, &buffInfo, nullptr, &buff));

    // ALLOCATE DEVICE MEMORY

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(dev, buff, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    auto pass = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &allocInfo.memoryTypeIndex);
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
    vk::assert_success(vkAllocateMemory(dev, &allocInfo, nullptr, &mem));

    // BIND MEMORY
    vk::assert_success(vkBindBufferMemory(dev, buff, mem, 0));

    return memReqs.size;
}

void copyBuffer(const VkCommandBuffer &cmd, const VkBuffer &srcBuff, const VkBuffer &dstBuff, const VkDeviceSize &size) {
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, srcBuff, dstBuff, 1, &copyRegion);
}

void createImage(const VkDevice &dev, const std::vector<uint32_t> &queueFamilyIndices, const VkSampleCountFlagBits &numSamples,
                 const VkFormat &format, const VkImageTiling &tiling, const VkImageUsageFlags &usage, const VkFlags &reqMask,
                 uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkImage &image,
                 VkDeviceMemory &memory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;  // param?
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    /* Create image */
    vk::assert_success(vkCreateImage(dev, &imageInfo, nullptr, &image));

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(dev, image, &mem_reqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = mem_reqs.size;
    /* Use the memory properties to determine the type of memory required */
    auto pass = getMemoryType(mem_reqs.memoryTypeBits, reqMask, &allocInfo.memoryTypeIndex);
    assert(pass);

    /* Allocate memory */
    vk::assert_success(vkAllocateMemory(dev, &allocInfo, nullptr, &memory));

    vk::assert_success(vkBindImageMemory(dev, image, memory, 0));
}

void copyBufferToImage(const VkCommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount, const VkBuffer &srcBuff, const VkImage &dstImg) {
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, srcBuff, dstImg,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // VK_IMAGE_USAGE_TRANSFER_DST_BIT
                           1, &region);
}

void createImageView(const VkDevice &device, const VkImage &image, const uint32_t &mipLevels, const VkFormat &format,
                     const VkImageAspectFlags &aspectFlags, const VkImageViewType &viewType, uint32_t layerCount, VkImageView &view) {
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
    viewInfo.subresourceRange.levelCount = mipLevels;

    /*  If you were working on a stereographic 3D application, then you would create a swap
        chain with multiple layers. You could then create multiple image views for each image
        representing the views for the left and right eyes by accessing different layers.
    */
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;

    vk::assert_success(vkCreateImageView(device, &viewInfo, nullptr, &view));
}

void transitionImageLayout(const VkCommandBuffer &cmd, const VkImage &image, const VkFormat &format, const VkImageLayout &oldLayout,
                           const VkImageLayout &newLayout, VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages,
                           uint32_t mipLevels, uint32_t arrayLayers) {
    VkImageMemoryBarrier barrier;
    for (uint32_t i = 0; i < arrayLayers; i++) {
        barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = i;
        barrier.subresourceRange.layerCount = 1;

        // ACCESS MASK

        bool layoutHandled = true;

        switch (oldLayout) {
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
                layoutHandled = false;
                break;
        }

        switch (newLayout) {
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
                barrier.dstAccessMask =
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;
            default:
                layoutHandled = false;
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

        if (!layoutHandled) throw std::invalid_argument("unsupported image layout transition!");

        // ASPECT MASK

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        vkCmdPipelineBarrier(cmd, srcStages, dstStages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}

}  // namespace helpers
