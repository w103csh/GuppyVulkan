
#include "Helpers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <regex>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <unordered_map>

#include "ConstantsAll.h"
#include "ComputeHandler.h"  // Remove this if a ComputeConstants.h is made.
#include "Face.h"
#include "Mesh.h"

namespace {
const std::string MACRO_REPLACE_PREFIX = "MACRO_REPLACE_PREFIX";
const std::string MACRO_REGEX_TEMPLATE = "(#define)\\s+(" + MACRO_REPLACE_PREFIX + "(.+))\\s+([\\-]?\\d+)";
}  // namespace

namespace helpers {

std::vector<macroInfo> getMacroReplaceInfo(const std::string_view &macroIdentifierPrefix, const std::string &text) {
    // { macro identifier, line, value }
    std::vector<macroInfo> replaceStrs;

    auto regexStr = MACRO_REGEX_TEMPLATE;
    regexStr = helpers::replaceFirstOccurrence(MACRO_REPLACE_PREFIX, macroIdentifierPrefix, regexStr);
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
    helpers::replaceFirstOccurrence(std::get<1>(info), replaceStr, text);
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

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

VkFormat findSupportedFormat(const VkPhysicalDevice &phyDev, const std::vector<VkFormat> &candidates,
                             const VkImageTiling tiling, const VkFormatFeatureFlags features) {
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

bool getMemoryType(const VkPhysicalDeviceMemoryProperties &memProps, uint32_t typeBits, VkMemoryPropertyFlags reqMask,
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

VkDeviceSize createBuffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                          const VkMemoryPropertyFlags &props, const VkPhysicalDeviceMemoryProperties &memProps,
                          VkBuffer &buff, VkDeviceMemory &mem) {
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
    auto pass = getMemoryType(memProps, memReqs.memoryTypeBits, props, &allocInfo.memoryTypeIndex);
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

void createImageMemory(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &memProps,
                       const VkMemoryPropertyFlags &memPropFlags, VkImage &image, VkDeviceMemory &memory) {
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(dev, image, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;

    // Use the memory properties to determine the type of memory required
    auto pass = getMemoryType(memProps, memReqs.memoryTypeBits, memPropFlags, &allocInfo.memoryTypeIndex);
    assert(pass);

    // Allocate memory
    vk::assert_success(vkAllocateMemory(dev, &allocInfo, nullptr, &memory));
    // Bind memory
    vk::assert_success(vkBindImageMemory(dev, image, memory, 0));
}

void createImage(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &memProps,
                 const std::vector<uint32_t> &queueFamilyIndices, const VkSampleCountFlagBits &numSamples,
                 const VkFormat &format, const VkImageTiling &tiling, const VkImageUsageFlags &usage,
                 const VkMemoryPropertyFlags &reqMask, uint32_t width, uint32_t height, uint32_t mipLevels,
                 uint32_t arrayLayers, VkImage &image, VkDeviceMemory &memory) {
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

    vk::assert_success(vkCreateImage(dev, &imageInfo, nullptr, &image));
    helpers::createImageMemory(dev, memProps, reqMask, image, memory);
}

void copyBufferToImage(const VkCommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount,
                       const VkBuffer &srcBuff, const VkImage &dstImg) {
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

void createImageView(const VkDevice &device, const VkImage &image, const VkFormat &format, const VkImageViewType &viewType,
                     const VkImageSubresourceRange &subresourceRange, VkImageView &view) {
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

    /*  If you were working on a stereographic 3D application, then you would create a swap
        chain with multiple layers. You could then create multiple image views for each image
        representing the views for the left and right eyes by accessing different layers.
    */
    viewInfo.subresourceRange = subresourceRange;

    vk::assert_success(vkCreateImageView(device, &viewInfo, nullptr, &view));
}

void transitionImageLayout(const VkCommandBuffer &cmd, const VkImage &image, const VkFormat &format,
                           const VkImageLayout &oldLayout, const VkImageLayout &newLayout, VkPipelineStageFlags srcStages,
                           VkPipelineStageFlags dstStages, uint32_t mipLevels, uint32_t layerCount) {
    VkImageMemoryBarrier barrier;
    // for (uint32_t i = 0; i < arrayLayers; i++) {

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
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

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
        case VK_IMAGE_LAYOUT_GENERAL:
            barrier.dstAccessMask = 0;
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
    //} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout ==
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
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
    //}
}

void validatePassTypeStructures() {
    auto it1 = std::find_first_of(RenderPass::ALL.begin(), RenderPass::ALL.end(), Compute::ALL.begin(), Compute::ALL.end());
    assert(it1 == RenderPass::ALL.end());
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
constexpr bool any2(const VB_INDEX_TYPE i0, const VB_INDEX_TYPE i1, const VB_INDEX_TYPE i2, const VB_INDEX_TYPE i,
                    VB_INDEX_TYPE &r0, VB_INDEX_TYPE &r1) {
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
constexpr bool any1(const VB_INDEX_TYPE i0, const VB_INDEX_TYPE i1, const VB_INDEX_TYPE i, VB_INDEX_TYPE &r) {
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
constexpr bool sharesTwoIndices(const VB_INDEX_TYPE i00, const VB_INDEX_TYPE i10, const VB_INDEX_TYPE i20,
                                const VB_INDEX_TYPE i01, const VB_INDEX_TYPE i11, VB_INDEX_TYPE &r) {
    VB_INDEX_TYPE r0 = 0, r1 = 0;
    if (any2(i00, i10, i20, i01, r0, r1))
        if (any1(r0, r1, i11, r)) return true;
    if (any2(i00, i10, i20, i11, r0, r1))
        if (any1(r0, r1, i01, r)) return true;
    return false;
}
}  // namespace

void makeTriangleAdjacenyList(const std::vector<VB_INDEX_TYPE> &indices, std::vector<VB_INDEX_TYPE> &indicesAdjaceny) {
    indicesAdjaceny.resize(indices.size() * 3);
    // I used int64_t in the second loops below.
    assert(indicesAdjaceny.size() <= INT64_MAX);

    VB_INDEX_TYPE t0, t1, t2, a0, a1, a2, r;
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

void attachementImageBarrierWriteToSamplerRead(const VkImage &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr});
    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
}

void attachementImageBarrierWriteToStorageRead(const VkImage &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr});
    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_GENERAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
}

void attachementImageBarrierStorageWriteToColorRead(const VkImage &image, BarrierResource &resource,
                                                    const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr});
    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
}

void attachementImageBarrierWriteToWrite(const VkImage &image, BarrierResource &resource, const uint32_t srcQueueFamilyIndex,
                                         const uint32_t dstQueueFamilyIndex) {
    // Ex: Barrier between scene/post-processing write to framebuffer, and UI write to the same framebuffer.
    // I believe this exact thing is handled in a subpass depency in RenderPass::ImGui.
    resource.imgBarriers.push_back({VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr});
    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
}

void storageImageBarrierWriteToRead(const VkImage &image, BarrierResource &resource, const uint32_t srcQueueFamilyIndex,
                                    const uint32_t dstQueueFamilyIndex) {
    resource.imgBarriers.push_back({VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr});
    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_GENERAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.imgBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.imgBarriers.back().image = image;
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
}

void bufferBarrierWriteToRead(const VkDescriptorBufferInfo &bufferInfo, BarrierResource &resource,
                              const uint32_t srcQueueFamilyIndex, const uint32_t dstQueueFamilyIndex) {
    resource.buffBarriers.push_back({VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr});
    resource.buffBarriers.back().srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.buffBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    resource.buffBarriers.back().srcQueueFamilyIndex = srcQueueFamilyIndex;
    resource.buffBarriers.back().dstQueueFamilyIndex = dstQueueFamilyIndex;
    resource.buffBarriers.back().buffer = bufferInfo.buffer;
    resource.buffBarriers.back().offset = bufferInfo.offset;
    resource.buffBarriers.back().size = bufferInfo.range;
}

void globalDebugBarrierWriteToRead(BarrierResource &resource) {
    resource.glblBarriers.push_back({VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr});
    // All src
    resource.glblBarriers.back().srcAccessMask =
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
    // All dst
    resource.glblBarriers.back().dstAccessMask =
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
}

void recordBarriers(const BarrierResource &resource, const VkCommandBuffer &cmd, const VkPipelineStageFlags srcStageMask,
                    const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags) {
    vkCmdPipelineBarrier(                                     //
        cmd,                                                  //
        srcStageMask,                                         //
        dstStageMask,                                         //
        dependencyFlags,                                      //
        static_cast<uint32_t>(resource.glblBarriers.size()),  //
        resource.glblBarriers.data(),                         //
        static_cast<uint32_t>(resource.buffBarriers.size()),  //
        resource.buffBarriers.data(),                         //
        static_cast<uint32_t>(resource.imgBarriers.size()),   //
        resource.imgBarriers.data()                           //
    );
}

}  // namespace helpers
