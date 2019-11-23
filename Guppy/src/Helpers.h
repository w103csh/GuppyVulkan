
#ifndef HELPERS_H
#define HELPERS_H

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>
#include <utility>

#include "ConstantsAll.h"
#include "Extensions.h"  // This is here for convenience

namespace vk {
inline VkResult assert_success(VkResult res) {
    if (res != VK_SUCCESS) {
        std::stringstream ss;
        ss << "VkResult " << res << " returned";
        assert(false);
        exit(EXIT_FAILURE);
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

template <typename T>
constexpr bool checkInterval(const T t, const T interval, T &lastTick) {
    if (t - interval > lastTick) {
        lastTick = t;
        return true;
    }
    return false;
}

std::vector<macroInfo> getMacroReplaceInfo(const std::string_view &macroIdentifierPrefix, const std::string &text);

void macroReplace(const macroInfo &info, int itemCount, std::string &text);

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

// The point of this is to turn the glm::lookAt into an affine transform for
// the Object3d model.
static glm::mat4 viewToWorld(glm::vec3 position, glm::vec3 focalPoint, glm::vec3 up) {
    /*  This assumes the up vector is a cardinal x, y, or z vector. glm::lookAt constructs
        a matrix where the direction from position to focal point is assign to the 3rd row (or
        z row). Here we shift the result to match the up vector.
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
        m = glm::rotate(m, M_PI_FLT, CARDINAL_Y);
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
static std::string makeVecString(TVec v) {
    std::string s = "(";
    for (auto i = 0; i < v.length(); i++) {
        s += std::to_string(v[i]);
        if (i + 1 < v.length()) s += ", ";
    }
    s += ")";
    return s;
}

bool hasStencilComponent(VkFormat format);

VkFormat findSupportedFormat(const VkPhysicalDevice &phyDev, const std::vector<VkFormat> &candidates,
                             const VkImageTiling tiling, const VkFormatFeatureFlags features);

VkFormat findDepthFormat(const VkPhysicalDevice &phyDev);

bool getMemoryType(const VkPhysicalDeviceMemoryProperties &memProps, uint32_t typeBits, VkMemoryPropertyFlags reqMask,
                   uint32_t *typeIndex);

VkDeviceSize createBuffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                          const VkMemoryPropertyFlags &props, const VkPhysicalDeviceMemoryProperties &memProps,
                          VkBuffer &buff, VkDeviceMemory &mem);

void copyBuffer(const VkCommandBuffer &cmd, const VkBuffer &srcBuff, const VkBuffer &dstBuff, const VkDeviceSize &size);

void createImageMemory(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &memProps,
                       const VkMemoryPropertyFlags &memPropFlags, VkImage &image, VkDeviceMemory &memory);

void createImage(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &memProps,
                 const std::vector<uint32_t> &queueFamilyIndices, const VkSampleCountFlagBits &numSamples,
                 const VkFormat &format, const VkImageTiling &tiling, const VkImageUsageFlags &usage,
                 const VkMemoryPropertyFlags &reqMask, uint32_t width, uint32_t height, uint32_t mipLevels,
                 uint32_t arrayLayers, VkImage &image, VkDeviceMemory &memory);

void copyBufferToImage(const VkCommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount,
                       const VkBuffer &src_buf, const VkImage &dst_img);

void createImageView(const VkDevice &device, const VkImage &image, const VkFormat &format, const VkImageViewType &viewType,
                     const VkImageSubresourceRange &subresourceRange, VkImageView &view);

void transitionImageLayout(const VkCommandBuffer &cmd, const VkImage &image, const VkFormat &format,
                           const VkImageLayout &oldLayout, const VkImageLayout &newLayout, VkPipelineStageFlags srcStages,
                           VkPipelineStageFlags dstStages, uint32_t mipLevels, uint32_t arrayLayers);

void validatePassTypeStructures();

void cramers3(glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 c4);

static glm::vec3 triangleNormal(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    return glm::cross(a - b, a - c);
}

static glm::vec3 positionOnLine(const glm::vec3 &a, const glm::vec3 &b, float t) { return a + ((b - a) * t); }

void makeTriangleAdjacenyList(const std::vector<VB_INDEX_TYPE> &indices, std::vector<VB_INDEX_TYPE> &indiciesAdjacency);

static void destroyCommandBuffers(const VkDevice &dev, const VkCommandPool &pool, std::vector<VkCommandBuffer> &cmds) {
    if (!cmds.empty()) {
        vkFreeCommandBuffers(dev, pool, static_cast<uint32_t>(cmds.size()), cmds.data());
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

static void destroyImageResource(const VkDevice &dev, ImageResource &res) {
    if (res.view != VK_NULL_HANDLE) vkDestroyImageView(dev, res.view, nullptr);
    res.view = VK_NULL_HANDLE;
    if (res.image != VK_NULL_HANDLE) vkDestroyImage(dev, res.image, nullptr);
    res.image = VK_NULL_HANDLE;
    if (res.memory != VK_NULL_HANDLE) vkFreeMemory(dev, res.memory, nullptr);
    res.memory = VK_NULL_HANDLE;
}

constexpr bool compExtent2D(const VkExtent2D &a, const VkExtent2D &b) {
    return a.height == b.height && a.width == b.width;  //
}

constexpr bool compExtent3D(const VkExtent3D &a, const VkExtent3D &b) {
    return a.height == b.height && a.width == b.width && a.depth == b.depth;
}

static bool isNumber(const std::string_view &sv) {
    return std::find_if(sv.begin(), sv.end(), [](const auto &c) { return !std::isdigit(c); }) == sv.end();
}

void attachementImageBarrierWriteToSamplerRead(const VkImage &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                               const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void attachementImageBarrierWriteToStorageRead(const VkImage &image, BarrierResource &resource,
                                               const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                               const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void attachementImageBarrierStorageWriteToColorRead(const VkImage &image, BarrierResource &resource,
                                                    const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                    const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void attachementImageBarrierWriteToWrite(const VkImage &image, BarrierResource &resource,
                                         const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                         const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void storageImageBarrierWriteToRead(const VkImage &image, BarrierResource &resource,
                                    const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void bufferBarrierWriteToRead(const VkDescriptorBufferInfo &bufferInfo, BarrierResource &resource,
                              const uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                              const uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

void globalDebugBarrierWriteToRead(BarrierResource &resource);

void recordBarriers(const BarrierResource &resource, const VkCommandBuffer &cmd, const VkPipelineStageFlags srcStageMask,
                    const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags = 0);

void createBuffer(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &memProps, const bool debugMarkersEnabled,
                  const VkCommandBuffer &cmd, const VkBufferUsageFlagBits usage, const VkDeviceSize size,
                  const std::string &&name, BufferResource &stgRes, BufferResource &buffRes, const void *data,
                  const bool mappable = false);

}  // namespace helpers

#endif  // !HELPERS_H
