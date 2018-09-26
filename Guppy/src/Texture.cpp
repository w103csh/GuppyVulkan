
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.h"
#include "util_init.hpp"

#define STB_FORMAT VK_FORMAT_R8G8B8A8_UNORM

// TODO: info should hold this pointer so that it can be destroyed easily.
VkSemaphore transfer_semaphore = nullptr;
VkSubmitInfo transfer_submit_info, graphics_submit_info;

void Texture::create_texture(struct sample_info& info, std::string tex_path) {
    tex_data tex = {};
    create_image(info, tex_path, tex);
    create_image_view(info, tex);
    create_sampler(info, tex);
    create_desc_info(tex);
    info.textures.push_back(tex);
}

void Texture::create_image(struct sample_info& info, std::string texturePath, tex_data& tex) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkDeviceSize imageSize = width * height * 4;  // TODO: where does this 4 come from?

    tex.width = static_cast<uint32_t>(width);
    tex.height = static_cast<uint32_t>(height);
    tex.channels = static_cast<uint32_t>(channels);
    tex.mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex.width, tex.height)))) + 1;

    staging_buffer staging_buf;

    auto mem_reqs_size =
        create_buffer(info, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buf.buf, staging_buf.mem);

    void* pData;
    vkMapMemory(info.device, staging_buf.mem, 0, mem_reqs_size, 0, &pData);
    memcpy(pData, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(info.device, staging_buf.mem);

    stbi_image_free(pixels);

    create_image(info, static_cast<uint32_t>(tex.width), static_cast<uint32_t>(tex.height), tex.mip_levels, VK_SAMPLE_COUNT_1_BIT,
                 STB_FORMAT, VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.mem);

    transition_image_layout(info.cmds[info.transfer_queue_family_index], tex.image, tex.mip_levels, STB_FORMAT,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT);

    copy_buffer_to_image(info.cmds[info.graphics_queue_family_index], staging_buf.buf, tex.image, tex.width, tex.height);
    info.staging_buffers.push_back(staging_buf);

    // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    generate_mipmaps(info, tex.image, info.swapchain_format.format, tex.width, tex.height, tex.mip_levels);

}

void Texture::create_image_view(struct sample_info& info, tex_data& tex) {
    create_image_view(info.device, tex.image, tex.mip_levels, STB_FORMAT,
                      VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, tex.view);
}

void Texture::create_sampler(struct sample_info& info, tex_data& tex) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;  // TODO: OPTION (FEATURE BASED)
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;  // test this out for fun
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0;  // static_cast<float>(m_mipLevels / 2); // Optional
    samplerInfo.maxLod = static_cast<float>(tex.mip_levels);
    samplerInfo.mipLodBias = 0;  // Optional

    if (vkCreateSampler(info.device, &samplerInfo, nullptr, &tex.sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void Texture::create_desc_info(tex_data& tex) {
    tex.img_desc_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex.img_desc_info.imageView = tex.view;
    tex.img_desc_info.sampler = tex.sampler;
}

void Texture::generate_mipmaps(struct sample_info& info, const VkImage& image, const VkFormat& format, const int32_t& width,
                               const int32_t& height, const uint32_t& mip_levels) {
    auto cmd = info.cmds[info.graphics_queue_family_index];

    /*  Throw if the format we want doesn't support linear blitting!

        TODO: Clean up what happens if the GPU doesn't handle linear blitting

        There are two alternatives in this case. You could implement a function that searches common texture image
        formats for one that does support linear blitting, or you could implement the mipmap generation in software
        with a library like stb_image_resize. Each mip level can then be loaded into the image in the same way that
        you loaded the original image.
    */
    find_supported_format(info, {format}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    // This was the way before mip maps
    // transitionImageLayout(
    //    srcQueueFamilyIndexFinal,
    //    dstQueueFamilyIndexFinal,
    //    m_mipLevels,
    //    image,
    //    VK_FORMAT_R8G8B8A8_UNORM,
    //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //    m_transferCommandPool
    //);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mip_levels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &barrier);

        // This is a bit wonky methinks (non-sqaure is the case for this)
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);
}

void Texture::submit_queues(struct sample_info& info) {
    VkResult U_ASSERT_ONLY res;

    // If semaphore hasn't been created then create it.
    if (transfer_semaphore == nullptr) {
        // SEMAPHORE
        VkSemaphoreCreateInfo semaphore_info = {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        res = vkCreateSemaphore(info.device, &semaphore_info, nullptr, &transfer_semaphore);
        assert(res == VK_SUCCESS);

        // TRANSFER SUBMIT INFO
        transfer_submit_info = {};
        transfer_submit_info.pNext = nullptr;
        transfer_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // WAIT
        transfer_submit_info.waitSemaphoreCount = 0;
        transfer_submit_info.pWaitSemaphores = nullptr;
        transfer_submit_info.pWaitDstStageMask = 0;
        // COMMAND BUFFERS
        transfer_submit_info.commandBufferCount = 1;
        transfer_submit_info.pCommandBuffers = &info.cmds[info.transfer_queue_family_index];
        // SIGNAL
        transfer_submit_info.signalSemaphoreCount = 1;
        transfer_submit_info.pSignalSemaphores = &transfer_semaphore;

        // GRAPHICS SUBMIT INFO
        graphics_submit_info = {};
        graphics_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        graphics_submit_info.pNext = nullptr;
        // WAIT
        VkPipelineStageFlags wait_stages[] = { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
        graphics_submit_info.waitSemaphoreCount = 1;
        graphics_submit_info.pWaitSemaphores = &transfer_semaphore;
        graphics_submit_info.pWaitDstStageMask = wait_stages;
        // Command buffer
        graphics_submit_info.commandBufferCount = 1;
        graphics_submit_info.pCommandBuffers = &info.cmds[info.graphics_queue_family_index];
        // SIGNAL
        graphics_submit_info.signalSemaphoreCount = 0;
        graphics_submit_info.pSignalSemaphores = nullptr;
    }

    // graphics then transfer and see if semaphore works ...

    execute_end_command_buffer(info.cmds[info.graphics_queue_family_index]);
    execute_end_command_buffer(info.cmds[info.transfer_queue_family_index]);


    // FOR TESTING ...
    VkFence fence1, fence2;
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start fence in signaled state
    res = vkCreateFence(info.device, &fence_info, nullptr, &fence1);
    assert(res == VK_SUCCESS);
    res = vkCreateFence(info.device, &fence_info, nullptr, &fence2);
    assert(res == VK_SUCCESS);

    // SUBMIT ...
    res = vkQueueSubmit(info.queues[info.transfer_queue_family_index], 1, &transfer_submit_info, fence1);
    assert(res == VK_SUCCESS);
    res = vkQueueSubmit(info.queues[info.graphics_queue_family_index], 1, &graphics_submit_info, fence2);
    assert(res == VK_SUCCESS);


    // WAIT FOR FENCES
    res = vkWaitForFences(info.device, 1, &fence1, VK_TRUE, std::numeric_limits<uint64_t>::max() /* disables timeout */);
    assert(res == VK_SUCCESS);
    res = vkWaitForFences(info.device, 1, &fence2, VK_TRUE, std::numeric_limits<uint64_t>::max() /* disables timeout */);
    assert(res == VK_SUCCESS);

    execute_begin_command_buffer(info.cmds[info.graphics_queue_family_index]);
    execute_begin_command_buffer(info.cmds[info.transfer_queue_family_index]);
}
