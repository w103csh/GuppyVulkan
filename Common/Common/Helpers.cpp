/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Helpers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <regex>
#include <unordered_map>

namespace {
const std::string MACRO_REPLACE_PREFIX = "MACRO_REPLACE_PREFIX";
const std::string MACRO_REGEX_TEMPLATE = "(#define)\\s+(" + MACRO_REPLACE_PREFIX + "(.+))\\s+([\\-]?\\d+)";
}  // namespace

namespace helpers {

std::vector<macroInfo> getMacroReplaceInfo(const std::string_view &macroIdentifierPrefix, const std::string &text) {
    // { macro identifier, line, value }
    std::vector<macroInfo> replaceStrs;

    auto regexStr = MACRO_REGEX_TEMPLATE;
    regexStr = replaceFirstOccurrence(MACRO_REPLACE_PREFIX, macroIdentifierPrefix, regexStr);
    std::regex macroRegex(regexStr);

    std::smatch results;
    auto it = text.cbegin();
    while (std::regex_search(it, text.end(), results, macroRegex)) {
        std::stringstream ss;
        ss << results[1] << " " << results[2] << " ";
        replaceStrs.push_back({results[2], results[0], ss.str(), std::stoi(results[4])});
        it = results[0].second;
    }

    return replaceStrs;
}

void macroReplace(const macroInfo &info, int itemCount, std::string &text) {
    std::string replaceStr = std::get<2>(info) + std::to_string(itemCount);
    replaceFirstOccurrence(std::get<1>(info), replaceStr, text);
}

glm::mat4 moveAndRotateTo(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up) {
    auto const z(glm::normalize(eye - center));
    auto const x(glm::normalize(cross(z, up)));
    auto const y(glm::cross(x, z));

    glm::mat4 Result(1.0f);
    Result[0][0] = x.x;
    Result[0][1] = x.y;
    Result[0][2] = x.z;
    Result[1][0] = y.x;
    Result[1][1] = y.y;
    Result[1][2] = y.z;
    Result[2][0] = z.x;
    Result[2][1] = z.y;
    Result[2][2] = z.z;
    Result[3][0] = eye.x;
    Result[3][1] = eye.y;
    Result[3][2] = eye.z;
    return Result;
}

// Normalized screen space transform (texture coord space) s,t,r : [0, 1]
glm::mat4 getBias() {
    auto bias = glm::translate(glm::mat4{1.0f}, glm::vec3{0.5f, 0.5f, 0.0f});
    bias = glm::scale(bias, glm::vec3{0.5f, 0.5f, 1.0f});
    return bias;
}

glm::mat3 makeArbitraryBasis(const glm::vec3 &dir) {
    glm::mat3 basis;
    glm::vec3 u, n;
    n = glm::cross(glm::vec3(1, 0, 0), dir);
    if (glm::length(n) < 0.00001f) n = glm::cross(glm::vec3(0, 1, 0), dir);
    u = glm::cross(dir, n);
    basis[0] = glm::normalize(u);
    basis[1] = glm::normalize(dir);
    basis[2] = glm::normalize(n);
    return basis;
}

bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD16UnormS8Uint || format == vk::Format::eD24UnormS8Uint ||
           format == vk::Format::eD32SfloatS8Uint;
}

vk::Format findSupportedFormat(const vk::PhysicalDevice &phyDev, const std::vector<vk::Format> &candidates,
                               const vk::ImageTiling tiling, const vk::FormatFeatureFlags features) {
    vk::Format format = vk::Format::eUndefined;
    for (vk::Format f : candidates) {
        vk::FormatProperties props = phyDev.getFormatProperties(f);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            format = f;
            break;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            format = f;
            break;
        }
    }
    return format;
}

vk::Format findDepthFormat(const vk::PhysicalDevice &phyDev) {
    return findSupportedFormat(phyDev, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                               vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool getMemoryType(const vk::PhysicalDeviceMemoryProperties &memProps, uint32_t typeBits, vk::MemoryPropertyFlags reqMask,
                   uint32_t *typeIndex) {
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memProps.memoryTypes[i].propertyFlags & reqMask) == reqMask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

vk::DeviceSize createBuffer(const vk::Device &dev, const vk::DeviceSize &size, const vk::BufferUsageFlags &usage,
                            const vk::MemoryPropertyFlags &props, const vk::PhysicalDeviceMemoryProperties &memProps,
                            vk::Buffer &buff, vk::DeviceMemory &mem, const vk::AllocationCallbacks *pAllocator) {
    vk::BufferCreateInfo buffInfo = {};
    buffInfo.size = size;
    buffInfo.usage = usage;
    buffInfo.sharingMode = vk::SharingMode::eExclusive;
    // TODO: what should these be ?
    // buffInfo.queueFamilyIndexCount = 0;
    // buffInfo.pQueueFamilyIndices = nullptr;
    // buffInfo.flags = 0;

    buff = dev.createBuffer(buffInfo, pAllocator);

    // ALLOCATE DEVICE MEMORY

    vk::MemoryRequirements memReqs = dev.getBufferMemoryRequirements(buff);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memReqs.size;
    auto pass = getMemoryType(memProps, memReqs.memoryTypeBits, props, &allocInfo.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    /*
        It should be noted that in a real world application, you're not supposed to actually
        call allocateMemory for every individual buffer. The maximum number of simultaneous
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
        index buffer, into a single vk::Buffer and use offsets in commands like
        bindVertexBuffers. The advantage is that your data is more cache friendly in that
        case, because it's closer together. It is even possible to reuse the same chunk of
        memory for multiple resources if they are not used during the same render operations,
        provided that their data is refreshed, of course. This is known as aliasing and some
        Vulkan functions have explicit flags to specify that you want to do this.
    */
    mem = dev.allocateMemory(allocInfo, pAllocator);

    // BIND MEMORY
    dev.bindBufferMemory(buff, mem, 0);

    return memReqs.size;
}

void copyBuffer(const vk::CommandBuffer &cmd, const vk::Buffer &srcBuff, const vk::Buffer &dstBuff,
                const vk::DeviceSize &size) {
    vk::BufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    cmd.copyBuffer(srcBuff, dstBuff, {copyRegion});
}

void createImageMemory(const vk::Device &dev, const vk::PhysicalDeviceMemoryProperties &memProps,
                       const vk::MemoryPropertyFlags &memPropFlags, vk::Image &image, vk::DeviceMemory &memory,
                       const vk::AllocationCallbacks *pAllocator) {
    vk::MemoryRequirements memReqs = dev.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memReqs.size;

    // Use the memory properties to determine the type of memory required
    auto pass = getMemoryType(memProps, memReqs.memoryTypeBits, memPropFlags, &allocInfo.memoryTypeIndex);
    assert(pass);

    // Allocate memory
    memory = dev.allocateMemory(allocInfo, pAllocator);
    // Bind memory
    dev.bindImageMemory(image, memory, 0);
}

void createImage(const vk::Device &dev, const vk::PhysicalDeviceMemoryProperties &memProps,
                 const std::vector<uint32_t> &queueFamilyIndices, const vk::SampleCountFlagBits &numSamples,
                 const vk::Format &format, const vk::ImageTiling &tiling, const vk::ImageUsageFlags &usage,
                 const vk::MemoryPropertyFlags &reqMask, uint32_t width, uint32_t height, uint32_t mipLevels,
                 uint32_t arrayLayers, vk::Image &image, vk::DeviceMemory &memory,
                 const vk::AllocationCallbacks *pAllocator) {
    vk::ImageCreateInfo imageInfo = {};
    imageInfo.imageType = vk::ImageType::e2D;  // param?
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    image = dev.createImage(imageInfo, pAllocator);
    createImageMemory(dev, memProps, reqMask, image, memory, pAllocator);
}

void copyBufferToImage(const vk::CommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount,
                       const vk::Buffer &srcBuff, const vk::Image &dstImg) {
    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    cmd.copyBufferToImage(srcBuff, dstImg, vk::ImageLayout::eTransferDstOptimal, {region});
}

void createImageView(const vk::Device &device, const vk::Image &image, const vk::Format &format,
                     const vk::ImageViewType &viewType, const vk::ImageSubresourceRange &subresourceRange,
                     vk::ImageView &view, const vk::AllocationCallbacks *pAllocator) {
    vk::ImageViewCreateInfo viewInfo = {};
    viewInfo.image = image;

    // swap chain is a 2D depth texture
    viewInfo.viewType = viewType;
    viewInfo.format = format;

    // defaults
    viewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.a = vk::ComponentSwizzle::eIdentity;

    /*  If you were working on a stereographic 3D application, then you would create a swap
        chain with multiple layers. You could then create multiple image views for each image
        representing the views for the left and right eyes by accessing different layers.
    */
    viewInfo.subresourceRange = subresourceRange;

    view = device.createImageView(viewInfo, pAllocator);
}

void transitionImageLayout(const vk::CommandBuffer &cmd, const vk::Image &image, const vk::Format &format,
                           const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout,
                           vk::PipelineStageFlags srcStages, vk::PipelineStageFlags dstStages, uint32_t mipLevels,
                           uint32_t layerCount) {
    // for (uint32_t i = 0; i < arrayLayers; i++) {

    vk::ImageMemoryBarrier barrier = {};
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = {};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    // ACCESS MASK

    bool layoutHandled = true;

    switch (oldLayout) {
        case vk::ImageLayout::eUndefined:
            barrier.srcAccessMask = {};
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::ePreinitialized:
            barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
            break;
        default:
            layoutHandled = false;
            break;
    }

    switch (newLayout) {
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.dstAccessMask =
                vk::AccessFlagBits::eColorAttachmentWrite;  // | vk::AccessFlagBits::eColorAttachmentRead;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.dstAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentWrite;  // | vk::AccessFlagBits::eDepthStencilAttachmentRead;
            break;
        case vk::ImageLayout::eGeneral:
            barrier.dstAccessMask = {};
            break;
        default:
            layoutHandled = false;
            break;
    }

    // if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
    //    barrier.srcAccessMask = 0;
    //    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    //    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    //    destinationStage = vk::PipelineStageFlagBits::eTransfer;
    //} else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout ==
    // vk::ImageLayout::eShaderReadOnlyOptimal) {
    //    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    //    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    //    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    //    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    //} else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthAttachmentOptimal)
    //{
    //    barrier.srcAccessMask = 0;
    //    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
    //    vk::AccessFlagBits::eDepthStencilAttachmentWrite; sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    //    destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    //} else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eColorAttachmentOptimal) {
    //    barrier.srcAccessMask = 0;
    //    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    //    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    //    destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    //} else {
    //    throw std::invalid_argument("unsupported image layout transition!");
    //}

    if (!layoutHandled) throw std::invalid_argument("unsupported image layout transition!");

    // ASPECT MASK

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    cmd.pipelineBarrier(srcStages, dstStages, {}, {}, {}, {barrier});
    //}
}

void cramers3(glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 c4) {
    /*  Setup example (comes from barycentric intersection):

        |a   d   g| |beta | = |j|
        |b   e   h| |gamma| = |k|
        |c   f   i| |t    | = |l|

        M =      a(ei-hf) + b(gf-di) + c(dh-eg)
        t =     (f(ak-jb) + e(jc-al) + d(bl-kc)) / M
        gamma = (i(ak-jb) + h(jc-al) + g(bl-kc)) / M
        beta =  (j(ei-hf) + k(gf-di) + l(dh-eg)) / M

        This should be faster than using glm::determinant...
        potentially. T
    */
    // glm::mat3 A(c1, c2, c3);
    // auto M_ = glm::determinant(A);
    // glm::mat3 BETA(c4, c2, c3);
    // auto beta_ = glm::determinant(BETA) / M_;
    // glm::mat3 GAMMA(c1, c4, c3);
    // auto gamma_ = glm::determinant(GAMMA) / M_;
    // glm::mat3 T(c1, c2, c4);
    // auto t_ = glm::determinant(T) / M_;
}

// triangle adjacency helpers
namespace {
constexpr bool any2(const IndexBufferType i0, const IndexBufferType i1, const IndexBufferType i2, const IndexBufferType i,
                    IndexBufferType &r0, IndexBufferType &r1) {
    if (i0 == i) {
        r0 = i1;
        r1 = i2;
        return true;
    }
    if (i1 == i) {
        r0 = i0;
        r1 = i2;
        return true;
    }
    if (i2 == i) {
        r0 = i0;
        r1 = i1;
        return true;
    }
    return false;
}
constexpr bool any1(const IndexBufferType i0, const IndexBufferType i1, const IndexBufferType i, IndexBufferType &r) {
    if (i0 == i) {
        r = i1;
        return true;
    }
    if (i1 == i) {
        r = i0;
        return true;
    }
    return false;
}
constexpr bool sharesTwoIndices(const IndexBufferType i00, const IndexBufferType i10, const IndexBufferType i20,
                                const IndexBufferType i01, const IndexBufferType i11, IndexBufferType &r) {
    IndexBufferType r0 = 0, r1 = 0;
    if (any2(i00, i10, i20, i01, r0, r1))
        if (any1(r0, r1, i11, r)) return true;
    if (any2(i00, i10, i20, i11, r0, r1))
        if (any1(r0, r1, i01, r)) return true;
    return false;
}
}  // namespace

void makeTriangleAdjacenyList(const std::vector<IndexBufferType> &indices, std::vector<IndexBufferType> &indicesAdjaceny) {
    indicesAdjaceny.resize(indices.size() * 3);
    // I used int64_t in the second loops below.
    assert(indicesAdjaceny.size() <= INT64_MAX);

    IndexBufferType t0, t1, t2, a0, a1, a2, r;
    bool b0, b1, b2;
    auto iSize = static_cast<int64_t>(indices.size());

    for (int64_t i = 0; i < static_cast<int64_t>(iSize); i += 3) {
        // Found flags
        b0 = b1 = b2 = false;
        // Triangle indices
        t0 = indices[i];
        t1 = indices[i + 1];
        t2 = indices[i + 2];
        // Set the triangle indices
        indicesAdjaceny[(i * 2)] = t0;
        indicesAdjaceny[(i * 2) + 2] = t1;
        indicesAdjaceny[(i * 2) + 4] = t2;

        for (int64_t j = i - 3, k = i + 5; j >= 0 || k < iSize; j -= 3, k += 3) {
            if (j >= 0) {
                // Adjacency indices
                a0 = indices[j];
                a1 = indices[j + 1];
                a2 = indices[j + 2];
                // First edge
                if (!b0 && sharesTwoIndices(a0, a1, a2, t0, t1, r)) {
                    indicesAdjaceny[(i * 2) + 1] = r;
                    b0 = true;
                    if (b0 && b1 && b2) break;
                }
                // Second edge
                if (!b1 && sharesTwoIndices(a0, a1, a2, t1, t2, r)) {
                    indicesAdjaceny[(i * 2) + 3] = r;
                    b1 = true;
                    if (b0 && b1 && b2) break;
                }
                // Thired edge
                if (!b2 && sharesTwoIndices(a0, a1, a2, t2, t0, r)) {
                    indicesAdjaceny[(i * 2) + 5] = r;
                    b2 = true;
                    if (b0 && b1 && b2) break;
                }
            }
            if (k < iSize) {
                // Adjacency indices
                a0 = indices[k - 2];
                a1 = indices[k - 1];
                a2 = indices[k];
                // First edge
                if (!b0 && sharesTwoIndices(a0, a1, a2, t0, t1, r)) {
                    indicesAdjaceny[(i * 2) + 1] = r;
                    b0 = true;
                    if (b0 && b1 && b2) break;
                }
                // Second edge
                if (!b1 && sharesTwoIndices(a0, a1, a2, t1, t2, r)) {
                    indicesAdjaceny[(i * 2) + 3] = r;
                    b1 = true;
                    if (b0 && b1 && b2) break;
                }
                // Thired edge
                if (!b2 && sharesTwoIndices(a0, a1, a2, t2, t0, r)) {
                    indicesAdjaceny[(i * 2) + 5] = r;
                    b2 = true;
                    if (b0 && b1 && b2) break;
                }
            }
        }
    }
}

void decomposeScale(const glm::mat4 &m, glm::vec3 &scale) {
    glm::quat orientation{};
    glm::vec3 translation{};
    glm::vec3 skew{};
    glm::vec4 perspective{};
    glm::decompose(m, scale, orientation, translation, skew, perspective);
}

void attachementImageBarrierWriteToSamplerRead(const vk::Image &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderRead;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
}

void attachementImageBarrierWriteToStorageRead(const vk::Image &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderRead;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eGeneral;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
}

void attachementImageBarrierStorageWriteToColorRead(const vk::Image &image, BarrierResource &resource,
                                                    const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderRead;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eGeneral;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
}

void attachementImageBarrierWriteToWrite(const vk::Image &image, BarrierResource &resource,
                                         const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    // Ex: Barrier between scene/post-processing write to framebuffer, and UI write to the same framebuffer.
    // I believe this exact thing is handled in a subpass depency in RenderPass::ImGui.
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
}

void storageImageBarrierWriteToRead(const vk::Image &image, BarrierResource &resource, const uint32_t srcQueueFamilyIndex,
                                    const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderRead;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eGeneral;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eGeneral;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
}

void bufferBarrierWriteToRead(const vk::DescriptorBufferInfo &bufferInfo, BarrierResource &resource,
                              const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.buffBarriers.push_back({});
    resource.buffBarriers.back().srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.buffBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderRead;
    resource.buffBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.buffBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.buffBarriers.back().buffer = bufferInfo.buffer;
    resource.buffBarriers.back().offset = bufferInfo.offset;
    resource.buffBarriers.back().size = bufferInfo.range;
}

void globalDebugBarrierWriteToRead(BarrierResource &resource) {
    resource.glblBarriers.push_back({});
    // All src
    resource.glblBarriers.back().srcAccessMask =
        vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
        vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead |
        vk::AccessFlagBits::eInputAttachmentRead | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite |
        vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostRead |
        vk::AccessFlagBits::eHostWrite;
    // All dst
    resource.glblBarriers.back().dstAccessMask =
        vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
        vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead |
        vk::AccessFlagBits::eInputAttachmentRead | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite |
        vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostRead |
        vk::AccessFlagBits::eHostWrite;
}

void recordBarriers(const BarrierResource &resource, const vk::CommandBuffer &cmd, const vk::PipelineStageFlags srcStageMask,
                    const vk::PipelineStageFlags dstStageMask, const vk::DependencyFlags dependencyFlags) {
    cmd.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, resource.glblBarriers, resource.buffBarriers,
                        resource.imgBarriers);
}

}  // namespace helpers
