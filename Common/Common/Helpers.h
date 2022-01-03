/*
 * Modifications copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * -------------------------------
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HELPERS_H
#define HELPERS_H

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX /* Don't let Windows define min() or max() */
#endif
#endif

#include <algorithm>
#include <cctype>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <stdio.h>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <utility>

#include "Types.h"

namespace helpers {

static void checkVkResult(VkResult err) {
    if (err == 0) return;
    printf("VkResult %d\n", err);
    if (err < 0) exit(EXIT_FAILURE);
}
static void checkVkResult(vk::Result err) { checkVkResult(static_cast<VkResult>(err)); }

// Epsilon compare for floating point. Taken from here https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type almost_equal(T x, T y, int ulp) {
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x - y) <= std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
           // unless the result is subnormal
           || std::abs(x - y) < (std::numeric_limits<T>::min)();
}

template <typename T>
T minAlign(T value, T align) {
    T alignedValue = value;
    if ((value % align)) {
        alignedValue = (value + align - 1) & ~(align - 1);
    }
    return alignedValue;
}

template <typename T>
constexpr bool checkInterval(const T t, const T interval, T &lastTick) {
    if (t - interval > lastTick) {
        lastTick = t;
        return true;
    }
    return false;
}

static std::string replaceFirstOccurrence(const std::string &toReplace, const std::string_view &replaceWith,
                                          std::string &s) {
    std::size_t pos = s.find(toReplace);
    if (pos == std::string::npos) return s;
    return s.replace(pos, toReplace.length(), replaceWith);
}

template <typename T1, typename T2>
static std::string textReplace(std::string &text, std::string s1, std::string s2, T1 r1, T2 r2) {
    std::stringstream rss, nss;
    rss << s1 << r1 << s2;
    nss << s1 << r2 << s2;
    size_t f = text.find(rss.str());
    if (f != std::string::npos) text.replace(f, rss.str().length(), nss.str());
}

// { macro identifier, line to replace, line to append to, value }
typedef std::tuple<std::string, std::string, std::string, int> macroInfo;

std::vector<macroInfo> getMacroReplaceInfo(const std::string_view &macroIdentifierPrefix, const std::string &text);

void macroReplace(const macroInfo &info, int itemCount, std::string &text);

void textReplaceFromMap(const std::map<std::string, std::string> &map, std::string &text);

template <typename T>
std::vector<T> &&slice(const std::vector<T> &v, int m, int n) {
    auto first = v.cbegin();
    auto last = v.cbegin() + n + 1;
    return std::vector<T>(first, last);
}

static inline std::string getFilePath(std::string s) {
    char sep = '/';
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(0, i + 1));
    }
    return {};
}

static std::string getFileName(std::string s) {
    char sep = '/';
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(i + 1, s.length() - i));
    }
    return {};
}

static std::string getFileExt(std::string s) {
    s = getFileName(s);
    char sep = '.';
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(i + 1, s.length() - i));
    }
    return {};
}

static MODEL_FILE_TYPE getModelFileType(std::string s) {
    s = getFileExt(s);
    if (s == "obj" || s == "OBJ") {
        return MODEL_FILE_TYPE::OBJ;
    }
    return MODEL_FILE_TYPE::UNKNOWN;
}

// This obviously doesn't work right
template <typename T>
constexpr int nearestOdd(T value, bool roundDown = true) {
    static_assert(std::is_floating_point<T>::value, "value must be floating point type");
    int result = std::lround(value);
    if (result % 2 == 1)
        return result;
    else if (roundDown)
        return result - 1;
    else
        return result + 1;
}

template <typename T>
constexpr bool isPowerOfTwo(const T value) {
    static_assert(std::is_integral<T>::value, "value must be an integral type");
    return (value != 0) && ((value & (value - 1)) == 0);
}

constexpr glm::vec3 convertToZupLH(glm::vec3 v) { return {v.x, v.z, v.y}; }

glm::mat4 moveAndRotateTo(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up);

// The point of this is to turn the glm::lookAt into an affine transform for
// the Object3d model.
static glm::mat4 viewToWorld(glm::vec3 position, glm::vec3 focalPoint, glm::vec3 up) {
    /**
     * This assumes the up vector is a cardinal x, y, or z vector. glm::lookAt constructs a matrix where the direction from
     * position to focal point is assigned to the 3rd row (or z row). Here we shift the result to match the up vector.
     */
    auto m = glm::inverse(glm::lookAt(position, focalPoint, up));

    // glm::vec3 const f(glm::normalize(focalPoint - position));
    // glm::vec3 const s(glm::normalize(cross(f, up)));
    // glm::vec3 const u(glm::cross(s, f));

    // glm::mat4 m(1.0f);
    // m[0][0] = s.x;
    // m[1][0] = s.y;
    // m[2][0] = s.z;
    // m[0][1] = u.x;
    // m[1][1] = u.y;
    // m[2][1] = u.z;
    // m[0][2] = -f.x;
    // m[1][2] = -f.y;
    // m[2][2] = -f.z;
    // m[3][0] = -dot(position, s);
    // m[3][1] = -dot(position, u);
    // m[3][2] = dot(position, f);

    if (up.x == 1.0f) {
        assert(false && "TODO: rotate");
    } else if (up.y == 1.0f) {
        // glm::lookAt defaults to looking in -z by default so rotate it to positive...
        m = glm::rotate(m, glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        // m = glm::rotate(m, M_PI_FLT, glm::vec3(glm::row(m, 1)));
    } else if (up.z == 1.0f) {
        assert(false && "TODO: rotate");
    } else {
        throw std::runtime_error("Up vector not accounted for.");
    }

    return m;
}

static glm::mat4 affine(glm::vec3 scale = glm::vec3(1.0f), glm::vec3 translate = glm::vec3(0.0f), float angle = 0.0f,
                        glm::vec3 rotationAxis = glm::vec3(1.0f), glm::mat4 model = glm::mat4(1.0f)) {
    return glm::translate(glm::mat4(1.0f), translate) * glm::rotate(glm::mat4(1.0f), angle, rotationAxis) *
           glm::scale(model, scale);
}

// TODO: constant?
glm::mat4 getBias();

glm::mat3 makeArbitraryBasis(const glm::vec3 &dir);

template <typename TVec>
static std::string toString(TVec v) {
    std::string s = "(";
    for (auto i = 0; i < v.length(); i++) {
        s += std::to_string(v[i]);
        if (i + 1 < v.length()) s += ", ";
    }
    s += ")";
    return s;
}

#define SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURE_ENABLED false

constexpr bool hasStencilComponent(const vk::Format format) {
    return format == vk::Format::eD16UnormS8Uint || format == vk::Format::eD24UnormS8Uint ||
           format == vk::Format::eD32SfloatS8Uint;
}

constexpr vk::ImageLayout getDepthStencilAttachmentLayout(const vk::Format format) {
#if SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURE_ENABLED
    return hasStencilComponent(format) ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                       : vk::ImageLayout::eDepthAttachmentOptimal;
#else
    return vk::ImageLayout::eDepthStencilAttachmentOptimal;
#endif
}

constexpr vk::ImageLayout getDepthStencilReadOnlyLayout(const vk::Format format) {
#if SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURE_ENABLED
    return hasStencilComponent(format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                       : vk::ImageLayout::eDepthReadOnlyOptimal;
#else
    return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
#endif
}

constexpr vk::ImageAspectFlags getDepthStencilAspectMask(const vk::Format format) {
    vk::ImageAspectFlags aspecMask = vk::ImageAspectFlagBits::eDepth;
    if (hasStencilComponent(format)) {
        aspecMask |= vk::ImageAspectFlagBits::eStencil;
    }
    return aspecMask;
}

vk::ImageTiling getDepthStencilImageTiling(const vk::PhysicalDevice &physicalDevice, const vk::Format format);

vk::Format findSupportedFormat(const vk::PhysicalDevice &phyDev, const std::vector<vk::Format> &candidates,
                               const vk::ImageTiling tiling, const vk::FormatFeatureFlags features);

vk::Format findDepthFormat(const vk::PhysicalDevice &phyDev);

bool getMemoryType(const vk::PhysicalDeviceMemoryProperties &memProps, uint32_t typeBits, vk::MemoryPropertyFlags reqMask,
                   uint32_t *typeIndex);

vk::DeviceSize createBuffer(const vk::Device &dev, const vk::DeviceSize &size, const vk::BufferUsageFlags &usage,
                            const vk::MemoryPropertyFlags &props, const vk::PhysicalDeviceMemoryProperties &memProps,
                            vk::Buffer &buff, vk::DeviceMemory &mem, const vk::AllocationCallbacks *pAllocator);

void copyBuffer(const vk::CommandBuffer &cmd, const vk::Buffer &srcBuff, const vk::Buffer &dstBuff,
                const vk::DeviceSize &size);

void createImageMemory(const vk::Device &dev, const vk::PhysicalDeviceMemoryProperties &memProps,
                       const vk::MemoryPropertyFlags &memPropFlags, vk::Image &image, vk::DeviceMemory &memory,
                       const vk::AllocationCallbacks *pAllocator);

void createImage(const vk::Device &dev, const vk::PhysicalDeviceMemoryProperties &memProps,
                 const std::vector<uint32_t> &queueFamilyIndices, const vk::SampleCountFlagBits &numSamples,
                 const vk::Format &format, const vk::ImageTiling &tiling, const vk::ImageUsageFlags &usage,
                 const vk::MemoryPropertyFlags &reqMask, uint32_t width, uint32_t height, uint32_t mipLevels,
                 uint32_t arrayLayers, vk::Image &image, vk::DeviceMemory &memory,
                 const vk::AllocationCallbacks *pAllocator);

void copyBufferToImage(const vk::CommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount,
                       const vk::Buffer &src_buf, const vk::Image &dst_img);

void createImageView(const vk::Device &device, const vk::Image &image, const vk::Format &format,
                     const vk::ImageViewType &viewType, const vk::ImageSubresourceRange &subresourceRange,
                     vk::ImageView &view, const vk::AllocationCallbacks *pAllocator);

void transitionImageLayout(const vk::CommandBuffer &cmd, const vk::Image &image, const vk::Format &format,
                           const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout,
                           vk::PipelineStageFlags srcStages, vk::PipelineStageFlags dstStages, uint32_t mipLevels,
                           uint32_t arrayLayers);

void cramers3(glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 c4);

static glm::vec3 triangleNormal(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    return glm::cross(a - b, a - c);
}

static glm::vec3 positionOnLine(const glm::vec3 &a, const glm::vec3 &b, float t) { return a + ((b - a) * t); }

static glm::vec2 pointOnCircle(const glm::vec2 origin, float radius, float angle) {
    assert(false);  // never tested
    return {origin.x + radius * cos(angle), origin.y + radius * sin(angle)};
}

void makeTriangleAdjacenyList(const std::vector<IndexBufferType> &indices, std::vector<IndexBufferType> &indiciesAdjacency);

static void destroyCommandBuffers(const vk::Device &dev, const vk::CommandPool &pool, std::vector<vk::CommandBuffer> &cmds) {
    if (cmds.size()) {
        dev.freeCommandBuffers(pool, cmds);
        cmds.clear();
    }
}

void decomposeScale(const glm::mat4 &m, glm::vec3 &scale);

template <typename T1, typename T2>
static FlagBits incrementByteFlag(FlagBits flags, T1 firstBit, T2 allBits) {
    auto byteFlag = flags & allBits;
    flags = flags & ~allBits;
    byteFlag = byteFlag == 0 ? firstBit : byteFlag << 1;
    if (!(byteFlag & allBits)) byteFlag = 0;  // went through whole byte so reset
    flags |= byteFlag;
    return flags;
}

static void destroyImageResource(const vk::Device &dev, ImageResource &res, vk::AllocationCallbacks *pAllocator) {
    if (res.view) dev.destroyImageView(res.view, pAllocator);
    if (res.image) dev.destroyImage(res.image, pAllocator);
    if (res.memory) dev.freeMemory(res.memory, pAllocator);
}

constexpr bool compExtent2D(const vk::Extent2D &a, const vk::Extent2D &b) {
    return a.height == b.height && a.width == b.width;  //
}

constexpr bool compExtent3D(const vk::Extent3D &a, const vk::Extent3D &b) {
    return a.height == b.height && a.width == b.width && a.depth == b.depth;
}

static bool isNumber(const std::string_view &sv) {
    return std::find_if(sv.begin(), sv.end(), [](const auto &c) { return !std::isdigit(c); }) == sv.end();
}

void attachementImageBarrierWriteToSamplerRead(const vk::Image &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                               const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void attachementImageBarrierWriteToStorageRead(const vk::Image &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                               const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void attachementImageBarrierStorageWriteToColorRead(const vk::Image &image, BarrierResource &resource,
                                                    const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                    const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void attachementImageBarrierWriteToWrite(const vk::Image &image, BarrierResource &resource,
                                         const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                         const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void storageImageBarrierWriteToRead(const vk::Image &image, BarrierResource &resource,
                                    const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void bufferBarrierWriteToRead(const vk::DescriptorBufferInfo &bufferInfo, BarrierResource &resource,
                              const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                              const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void globalDebugBarrierWriteToRead(BarrierResource &resource);

static void globalDebugBarrier(const vk::CommandBuffer &cmd) {
    constexpr vk::AccessFlags all =
        vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
        vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead |
        vk::AccessFlagBits::eInputAttachmentRead | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite |
        vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostRead |
        vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
    vk::MemoryBarrier barrier = {};
    barrier.srcAccessMask = all;
    barrier.dstAccessMask = all;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {barrier}, {},
                        {});
}

void recordBarriers(const BarrierResource &resource, const vk::CommandBuffer &cmd, const vk::PipelineStageFlags srcStageMask,
                    const vk::PipelineStageFlags dstStageMask, const vk::DependencyFlags dependencyFlags = {});

inline void normalizePlane(plane &plane) { plane /= glm::length(glm::vec3(plane.x, plane.y, plane.z)); }

}  // namespace helpers

#endif  // !HELPERS_H
