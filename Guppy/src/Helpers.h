
#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <set>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>
#include <unordered_map>

#include "Constants.h"
#include "Extensions.h"  // This is here for convenience

typedef uint32_t FlagBits;
// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
typedef uint32_t VB_INDEX_TYPE;
typedef uint8_t SCENE_INDEX_TYPE;

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

enum class STATUS {
    //
    PENDING = 0,
    READY,
    PENDING_BUFFERS,
    PENDING_TEXTURE,
    REDRAW,
    UPDATE_BUFFERS,
};

enum class SHADER_TYPE {
    // DEFAULT
    COLOR_VERT = 0,
    TEX_VERT,
    COLOR_FRAG,
    LINE_FRAG,
    TEX_FRAG,
    UTIL_FRAG,
    // PBR
    PBR_COLOR_FRAG,
};

enum class PIPELINE_TYPE {
    //
    DONT_CARE = -1,
    TRI_LIST_COLOR = 0,
    LINE = 1,
    TRI_LIST_TEX = 2,
};

enum class PUSH_CONSTANT_TYPE {
    //
    DEFAULT = 0,
};

enum class DESCRIPTOR_TYPE {
    //
    /*  Currently this reflects unique descriptor bindings
        for the shaders. As of now the bindings are hardcoded
        and each descriptor type only has one each. (And line
        up with the enum values...).
        TODO: This could  easily become more dynamic once I wrap
        my head around how it should work with an allocator.
    */
    DEFAULT_UNIFORM = 0,
    DEFAULT_SAMPLER = 1,
    DEFAULT_DYNAMIC_UNIFORM = 2
};

enum class INPUT_TYPE {
    //
    UP,
    DOWN,
    DBLCLK,
    UNKNOWN
};

enum class MESH_TYPE {
    //
    COLOR = 0,
    LINE,
    TEXTURE
};

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

template <typename T>
std::vector<T> &&slice(const std::vector<T> &v, int m, int n) {
    auto first = v.cbegin(), auto last = v.cbegin() + n + 1;
    return std::vector<T> vec(first, last);
}

static std::string getFilePath(std::string s) {
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(0, i + 1));
    }
    return {};
}

static std::string getFileName(std::string s) {
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif
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
        // TODO: rotate
    } else if (up.y == 1.0f) {
        // glm::lookAt defaults to looking in -z by default so rotate it to positive...
        m = glm::rotate(m, M_PI_FLT, CARDINAL_Y);
        // m = glm::rotate(m, M_PI_FLT, glm::vec3(glm::row(m, 1)));
    } else if (up.z == 1.0f) {
        // TODO: rotate
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

static std::string makeVec3String(std::string prefix, glm::vec3 v) {
    std::stringstream ss;
    ss << prefix << "(" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
    return ss.str();
}

bool hasStencilComponent(VkFormat format);

VkFormat findSupportedFormat(const VkPhysicalDevice &phyDev, const std::vector<VkFormat> &candidates,
                             const VkImageTiling tiling, const VkFormatFeatureFlags features);

VkFormat findDepthFormat(const VkPhysicalDevice &phyDev);

bool getMemoryType(uint32_t typeBits, VkFlags reqMask, uint32_t *typeIndex);

VkDeviceSize createBuffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                          const VkMemoryPropertyFlags &props, VkBuffer &buff, VkDeviceMemory &mem);

void copyBuffer(const VkCommandBuffer &cmd, const VkBuffer &srcBuff, const VkBuffer &dstBuff, const VkDeviceSize &size);

void createImage(const VkDevice &dev, const std::vector<uint32_t> &queueFamilyIndices,
                 const VkSampleCountFlagBits &numSamples, const VkFormat &format, const VkImageTiling &tiling,
                 const VkImageUsageFlags &usage, const VkFlags &reqMask, uint32_t width, uint32_t height, uint32_t mipLevels,
                 uint32_t arrayLayers, VkImage &image, VkDeviceMemory &memory);

void copyBufferToImage(const VkCommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount,
                       const VkBuffer &src_buf, const VkImage &dst_img);

void createImageView(const VkDevice &device, const VkImage &image, const uint32_t &mipLevels, const VkFormat &format,
                     const VkImageAspectFlags &aspectFlags, const VkImageViewType &viewType, uint32_t layerCount,
                     VkImageView &view);

void transitionImageLayout(const VkCommandBuffer &cmd, const VkImage &image, const VkFormat &format,
                           const VkImageLayout &oldLayout, const VkImageLayout &newLayout, VkPipelineStageFlags srcStages,
                           VkPipelineStageFlags dstStages, uint32_t mipLevels, uint32_t arrayLayers);

void cramers3(glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 c4);

static glm::vec3 triangleNormal(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    return glm::cross(a - b, a - c);
}

static glm::vec3 positionOnLine(const glm::vec3 &a, const glm::vec3 &b, float t) { return a + ((b - a) * t); }

//// This is super simple... add to it if necessary
// template <typename TKey, typename TValue>
// struct simple_container_key_map {
//    void set(TKey key, TValue value) {
//        for (auto &keyValue_ : container_)
//            if (std::equal(key.begin(), key.end(), keyValue_.first.begin())) return;
//        container_.push_back({key, value});
//    }
//
//    std::pair<TKey, TValue> get(const TKey &key) {
//        for (auto &keyValue_ : container_)
//            if (std::equal(key.begin(), key.end(), keyValue_.first.begin())) return keyValue;
//        return {};
//    }
//
//   private:
//    std::vector<std::pair<TKey, TValue>> container_;
//};

}  // namespace helpers

struct ImageResource {
    VkFormat format;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};

struct BufferResource {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize memoryRequirementsSize;
};

struct FrameDataHologram {
    // signaled when this struct is ready for reuse
    VkFence fence = VK_NULL_HANDLE;

    VkCommandBuffer primary_cmd = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> worker_cmds;

    VkBuffer buf = VK_NULL_HANDLE;
    uint8_t *base = nullptr;
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
};

// TODO: check other places like LoadingResourceHandler to see if this
// could be used.
struct SubmitResource {
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageMasks;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> signalSemaphores;
};

template <typename T>
struct SharedStruct : std::enable_shared_from_this<T> {
    const std::shared_ptr<const T> getPtr() const { return shared_from_this(); }
};

struct Ray {
    glm::vec3 e;              // start
    glm::vec3 d;              // end
    glm::vec3 direction;      // non-normalized direction vector from "e" to "d"
    glm::vec3 directionUnit;  // normalized "direction"
};

struct DescriptorBufferResources {
    uint32_t count;
    VkDeviceSize size;
    VkDescriptorBufferInfo info;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

// These are basically a unit. Maybe move this into a full on data structure.
struct DescriptorMapItem {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    struct Resource {
        STATUS status = STATUS::PENDING;
        std::vector<VkDescriptorSet> sets;
    };
    std::vector<Resource> resources;
};
struct hash_descriptor_resource_map {
    size_t operator()(const std::set<DESCRIPTOR_TYPE> &s) const {
        size_t value = 0;
        auto x = 1;
        for (const auto &i : s)
            if (value)
                value ^= ((std::hash<DESCRIPTOR_TYPE>()(i)) << 1) >> 1;
            else
                value = (std::hash<DESCRIPTOR_TYPE>()(i));
        return value;
    }
};
typedef std::unordered_map<const std::set<DESCRIPTOR_TYPE>, DescriptorMapItem, hash_descriptor_resource_map>
    descriptor_resource_map;

namespace helpers {

static void destroyImageResource(const VkDevice &dev, ImageResource &res) {
    if (res.view != VK_NULL_HANDLE) vkDestroyImageView(dev, res.view, nullptr);
    res.view = VK_NULL_HANDLE;
    if (res.image != VK_NULL_HANDLE) vkDestroyImage(dev, res.image, nullptr);
    res.image = VK_NULL_HANDLE;
    if (res.memory != VK_NULL_HANDLE) vkFreeMemory(dev, res.memory, nullptr);
    res.memory = VK_NULL_HANDLE;
}

static void destroyCommandBuffers(const VkDevice &dev, const VkCommandPool &pool, std::vector<VkCommandBuffer> &cmds) {
    if (!cmds.empty()) {
        vkFreeCommandBuffers(dev, pool, static_cast<uint32_t>(cmds.size()), cmds.data());
        cmds.clear();
    }
}

}  // namespace helpers

#endif  // !HELPERS_H
