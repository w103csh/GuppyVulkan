
#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <limits>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>

// This is here for convenience
#include "Extensions.h"

enum class MODEL_FILE_TYPE { UNKNOWN = 0, OBJ };

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

static std::string getFileName(std::string s) {
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(i + 1, s.length() - i));
    }
    return ("");
}

static std::string getFileExt(std::string s) {
    s = getFileName(s);
    char sep = '.';
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(i + 1, s.length() - i));
    }
    return ("");
}

static MODEL_FILE_TYPE getModelFileType(std::string s) {
    s = getFileExt(s);
    if (s == "obj" || s == "OBJ") {
        return MODEL_FILE_TYPE::OBJ;
    }
    return MODEL_FILE_TYPE::UNKNOWN;
}

bool hasStencilComponent(VkFormat format);

VkFormat findSupportedFormat(const VkPhysicalDevice &phyDev, const std::vector<VkFormat> &candidates, const VkImageTiling tiling,
                             const VkFormatFeatureFlags features);

VkFormat findDepthFormat(const VkPhysicalDevice &phyDev);

bool getMemoryType(uint32_t typeBits, VkFlags reqMask, uint32_t *typeIndex);

VkDeviceSize createBuffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                          const VkMemoryPropertyFlags &props, VkBuffer &buff, VkDeviceMemory &mem);

void copyBuffer(const VkCommandBuffer &cmd, const VkBuffer &srcBuff, const VkBuffer &dstBuff, const VkDeviceSize &size);

void createImage(const VkDevice &dev, const std::vector<uint32_t> &queueFamilyIndices, const VkSampleCountFlagBits &numSamples,
                 const VkFormat &format, const VkImageTiling &tiling, const VkImageUsageFlags &usage, const VkFlags &reqMask,
                 uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkImage &image, VkDeviceMemory &memory);

void copyBufferToImage(const VkCommandBuffer &cmd, uint32_t width, uint32_t height, uint32_t layerCount, const VkBuffer &src_buf,
                       const VkImage &dst_img);

void createImageView(const VkDevice &device, const VkImage &image, const uint32_t &mipLevels, const VkFormat &format,
                     const VkImageAspectFlags &aspectFlags, const VkImageViewType &viewType, uint32_t layerCount,
                     VkImageView &view);

void transitionImageLayout(const VkCommandBuffer &cmd, const VkImage &image, const VkFormat &format, const VkImageLayout &oldLayout,
                           const VkImageLayout &newLayout, VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages,
                           uint32_t mipLevels, uint32_t arrayLayers);

glm::mat4 affine(glm::vec3 scale = glm::vec3(1.0f), glm::vec3 translate = glm::vec3(0.0f), float angle = 0.0f,
                 glm::vec3 rotationAxis = glm::vec3(1.0f), glm::mat4 model = glm::mat4(1.0f));

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
    uint32_t count;
    VkDeviceSize size;
    VkDescriptorBufferInfo info;
    VkBuffer buffer;
    VkDeviceMemory memory;
};

struct BufferResource {
    VkBuffer buffer;
    VkDeviceMemory memory;
};

struct FrameData {
    // signaled when this struct is ready for reuse
    VkFence fence;

    VkCommandBuffer primary_cmd;
    std::vector<VkCommandBuffer> worker_cmds;

    VkBuffer buf;
    uint8_t *base;
    VkDescriptorSet desc_set;
};

struct DrawResources {
    std::thread thread;
    VkCommandBuffer cmd;
};

template <typename T>
struct SharedStruct : std::enable_shared_from_this<T> {
    const std::shared_ptr<const T> getPtr() const { return shared_from_this(); }
};

struct Object3d {
   public:
    struct Data {
        glm::mat4 model = glm::mat4(1.0f);
    };

    Object3d(glm::mat4 model = glm::mat4(1.0f)) : obj3d_{model} {};

    virtual inline glm::vec3 getDirection() {
        glm::vec3 dir = glm::row(obj3d_.model, 2);
        return glm::normalize(dir);
    }

    virtual inline glm::vec3 getPosition() { return obj3d_.model[3]; }
    virtual inline void transform(glm::mat4 t) { obj3d_.model *= t; }

    inline Data getData() const { return obj3d_; }

   protected:
    Data obj3d_;
};

#endif  // !HELPERS_H
