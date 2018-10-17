
#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <cstring>
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

bool has_stencil_component(VkFormat format);

VkFormat find_supported_format(const VkPhysicalDevice &physical_dev, const std::vector<VkFormat> &candidates,
                               const VkImageTiling tiling, const VkFormatFeatureFlags features);

VkFormat find_depth_format(const VkPhysicalDevice &physical_dev);

bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);

VkDeviceSize create_buffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                           const VkMemoryPropertyFlags &props, VkBuffer &buf, VkDeviceMemory &buf_mem);

void copy_buffer(const VkCommandBuffer &cmd, const VkBuffer &src_buf, const VkBuffer &dst_buf, const VkDeviceSize &size);

void create_image(const VkDevice &dev, const std::vector<uint32_t> &queueFamilyIndices, const uint32_t width, uint32_t height,
                  uint32_t mip_levels, const VkSampleCountFlagBits &num_samples, const VkFormat &format,
                  const VkImageTiling &tiling, const VkImageUsageFlags &usage, const VkFlags &requirements_mask, VkImage &image,
                  VkDeviceMemory &image_memory);

void copy_buffer_to_image(const VkCommandBuffer &cmd, const VkBuffer &src_buf, const VkImage &dst_img, const uint32_t &width,
                          const uint32_t &height);

void create_image_view(const VkDevice &device, const VkImage &image, const uint32_t &mip_levels, const VkFormat &format,
                       const VkImageAspectFlags &aspectFlags, const VkImageViewType &viewType, VkImageView &image_view);

void transition_image_layout(const VkCommandBuffer &cmd, VkImage &image, const uint32_t &mip_levels, const VkFormat &format,
                             const VkImageLayout &old_layout, const VkImageLayout &new_layout, VkPipelineStageFlags src_stages,
                             VkPipelineStageFlags dst_stages);

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

#endif  // !HELPERS_H
