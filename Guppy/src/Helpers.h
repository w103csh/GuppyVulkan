
#ifndef HELPERS_H
#define HELPERS_H

#include <array>
#include <assert.h>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <utility>

#include "Constants.h"
#include "Extensions.h"  // This is here for convenience

typedef uint32_t FlagBits;
// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
typedef uint32_t VB_INDEX_TYPE;
typedef uint8_t SCENE_INDEX_TYPE;

// TODO: make a data structure so this can be const in the handlers.
template <typename TEnum, typename TType>
using enumPointerTypeMap = std::map<TEnum, std::unique_ptr<TType>>;

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

enum class STATUS {
    //
    PENDING = 0,
    READY,
    PENDING_VERTICES,
    PENDING_BUFFERS,
    PENDING_MATERIAL,
    PENDING_TEXTURE,
    REDRAW,
    UPDATE_BUFFERS,
};

enum class VERTEX {
    //
    COLOR = 0,
    TEXTURE
};

// **********************
//      Shader
// **********************

enum class SHADER {
    LINK = -1,
    // These are index values...
    // DEFAULT
    COLOR_VERT = 0,
    COLOR_FRAG,
    LINE_FRAG,
    TEX_VERT,
    TEX_FRAG,
    // PBR
    PBR_COLOR_FRAG,
    PBR_TEX_FRAG,
    // Add new to SHADER_ALL and SHADER_LINK_MAP.
};

const std::array<SHADER, 7> SHADER_ALL = {
    // DEFAULT
    SHADER::COLOR_VERT,  //
    SHADER::COLOR_FRAG,  //
    SHADER::LINE_FRAG,   //
    SHADER::TEX_VERT,    //
    SHADER::TEX_FRAG,    //
    // PBR
    SHADER::PBR_COLOR_FRAG,  //
    SHADER::PBR_TEX_FRAG,    //
};

enum class SHADER_LINK {
    //
    COLOR_FRAG = 0,
    TEX_FRAG,
    BLINN_PHONG_FRAG,
    PBR_FRAG,
    // Add new to SHADER_LINK_ALL and SHADER_LINK_MAP.
};

const std::array<SHADER_LINK, 4> SHADER_LINK_ALL = {
    // These need to be in same order as the enum
    SHADER_LINK::COLOR_FRAG,  //
    SHADER_LINK::TEX_FRAG,
    SHADER_LINK::BLINN_PHONG_FRAG,
    SHADER_LINK::PBR_FRAG,
};

const std::map<SHADER, std::set<SHADER_LINK>> SHADER_LINK_MAP = {
    // TODO: why is this necessary? it seems like it shouldn't be.
    {SHADER::COLOR_FRAG, {SHADER_LINK::COLOR_FRAG, SHADER_LINK::BLINN_PHONG_FRAG}},
    {SHADER::TEX_FRAG, {SHADER_LINK::TEX_FRAG, SHADER_LINK::BLINN_PHONG_FRAG}},
    {SHADER::PBR_COLOR_FRAG, {SHADER_LINK::COLOR_FRAG, SHADER_LINK::PBR_FRAG}},
    {SHADER::PBR_TEX_FRAG, {SHADER_LINK::TEX_FRAG, SHADER_LINK::PBR_FRAG}},
};

// **********************
//      Pipeline
// **********************

enum class PIPELINE {
    DONT_CARE = -1,
    // These are index values...
    TRI_LIST_COLOR = 0,
    LINE = 1,
    TRI_LIST_TEX = 2,
    PBR_COLOR = 3,
    PBR_TEX = 4,
    BP_TEX_CULL_NONE = 5,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP below.
};

const std::array<PIPELINE, 6> PIPELINE_ALL = {
    PIPELINE::TRI_LIST_COLOR,    //
    PIPELINE::LINE,              //
    PIPELINE::TRI_LIST_TEX,      //
    PIPELINE::PBR_COLOR,         //
    PIPELINE::PBR_TEX,           //
    PIPELINE::BP_TEX_CULL_NONE,  //
};

const std::map<VERTEX, std::set<PIPELINE>> VERTEX_PIPELINE_MAP = {
    //
    {
        VERTEX::COLOR,
        {
            //
            PIPELINE::TRI_LIST_COLOR,  //
            PIPELINE::LINE,            //
            PIPELINE::PBR_COLOR        //
        }                              //
    },
    {
        VERTEX::TEXTURE,
        {
            //
            PIPELINE::TRI_LIST_TEX,      //
            PIPELINE::PBR_TEX,           //
            PIPELINE::BP_TEX_CULL_NONE,  //
        },
    }  //
};

// **********************
//
// **********************

enum class PUSH_CONSTANT {
    DONT_CARE = -1,
    //
    DEFAULT = 0,
    PBR,
};

enum class DESCRIPTOR {
    // CAMERA
    CAMERA_PERSPECTIVE_DEFAULT,
    // LIGHT
    LIGHT_POSITIONAL_DEFAULT,
    LIGHT_POSITIONAL_PBR,
    LIGHT_SPOT_DEFAULT,
    // FOG,
    FOG_DEFAULT,
    // MATERIAL
    MATERIAL_DEFAULT,
    MATERIAL_PBR,
    // SAMPLER
    SAMPLER_DEFAULT,
};

const std::map<DESCRIPTOR, VkDescriptorType> DESCRIPTOR_TYPE_MAP = {
    // DEFAULT
    {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {DESCRIPTOR::FOG_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {DESCRIPTOR::LIGHT_SPOT_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {DESCRIPTOR::MATERIAL_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC},
    {DESCRIPTOR::SAMPLER_DEFAULT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    // PBR
    {DESCRIPTOR::LIGHT_POSITIONAL_PBR, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {DESCRIPTOR::MATERIAL_PBR, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC}
    //
};

const std::set<DESCRIPTOR> DESCRIPTOR_UNIFORM_ALL = {
    DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT,
    DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT,
    DESCRIPTOR::LIGHT_POSITIONAL_PBR,
    DESCRIPTOR::LIGHT_SPOT_DEFAULT,
    DESCRIPTOR::FOG_DEFAULT,
};

const std::set<DESCRIPTOR> DESCRIPTOR_MATERIAL_ALL = {
    DESCRIPTOR::MATERIAL_DEFAULT,
    DESCRIPTOR::MATERIAL_PBR,
};

const std::set<DESCRIPTOR> DESCRIPTOR_SAMPLER_ALL = {
    DESCRIPTOR::SAMPLER_DEFAULT,
};

enum class DESCRIPTOR_SET {
    //
    UNIFORM_DEFAULT,
    SAMPLER_DEFAULT,
    UNIFORM_PBR,
};

enum class HANDLER {
    //
    DESCRIPTOR,
    MATERIAL,
    TEXTURE,
};

enum class MATERIAL {
    //
    DEFAULT,
    PBR,
};

enum class INPUT_ACTION {
    //
    UP,
    DOWN,
    DBLCLK,
    UNKNOWN
};

enum class MESH {
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

class NonCopyable {
   public:
    NonCopyable() = default;
    ~NonCopyable() = default;

   private:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = delete;
    NonCopyable &operator=(NonCopyable &&) = delete;
};

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

static std::string replaceFirstOccurrence(const std::string &toReplace, const std::string &replaceWith, std::string &s) {
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

std::vector<macroInfo> getMacroReplaceInfo(const std::string &macroIdentifierPrefix, const std::string &text);

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

static bool checkVertexPipelineMap(VERTEX key, PIPELINE value) {
    for (const auto &type : VERTEX_PIPELINE_MAP.at(key)) {
        if (type == value) return true;
    }
    return false;
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

bool getMemoryType(const VkPhysicalDeviceMemoryProperties &memProps, uint32_t typeBits, VkMemoryPropertyFlags reqMask,
                   uint32_t *typeIndex);

VkDeviceSize createBuffer(const VkDevice &dev, const VkDeviceSize &size, const VkBufferUsageFlags &usage,
                          const VkMemoryPropertyFlags &props, const VkPhysicalDeviceMemoryProperties &memProps,
                          VkBuffer &buff, VkDeviceMemory &mem);

void copyBuffer(const VkCommandBuffer &cmd, const VkBuffer &srcBuff, const VkBuffer &dstBuff, const VkDeviceSize &size);

void createImage(const VkDevice &dev, const VkPhysicalDeviceMemoryProperties &memProps,
                 const std::vector<uint32_t> &queueFamilyIndices, const VkSampleCountFlagBits &numSamples,
                 const VkFormat &format, const VkImageTiling &tiling, const VkImageUsageFlags &usage, const VkFlags &reqMask,
                 uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkImage &image,
                 VkDeviceMemory &memory);

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

static void destroyCommandBuffers(const VkDevice &dev, const VkCommandPool &pool, std::vector<VkCommandBuffer> &cmds) {
    if (!cmds.empty()) {
        vkFreeCommandBuffers(dev, pool, static_cast<uint32_t>(cmds.size()), cmds.data());
        cmds.clear();
    }
}

void decomposeScale(const glm::mat4 &m, glm::vec3 &scale);

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

struct BufferResource {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryRequirements memoryRequirements{};
};

struct ImageResource {
    VkFormat format{};
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};

// TODO: check other places like LoadingResourceHandler to see if this
// could be used.
struct SubmitResource {
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageMasks;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> signalSemaphores;
};

// template <typename T>
// struct SharedStruct : std::enable_shared_from_this<T> {
//    const std::shared_ptr<const T> getPtr() const { return shared_from_this(); }
//};

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

struct DescriptorResource {
    VkDescriptorBufferInfo info = {};
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

template <typename TEnum>
struct hash_pair_enum_size_t {
    inline std::size_t operator()(const std::pair<TEnum, size_t> &p) const {
        std::hash<int> int_hasher;
        std::hash<size_t> size_t_hasher;
        return int_hasher(static_cast<int>(p.first)) ^ size_t_hasher(p.second);
    }
};

namespace helpers {

static inline bool isDescriptorTypeDynamic(const DESCRIPTOR &type) {
    const auto &descriptorType = DESCRIPTOR_TYPE_MAP.at(type);
    return descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
           descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}

static void destroyImageResource(const VkDevice &dev, ImageResource &res) {
    if (res.view != VK_NULL_HANDLE) vkDestroyImageView(dev, res.view, nullptr);
    res.view = VK_NULL_HANDLE;
    if (res.image != VK_NULL_HANDLE) vkDestroyImage(dev, res.image, nullptr);
    res.image = VK_NULL_HANDLE;
    if (res.memory != VK_NULL_HANDLE) vkFreeMemory(dev, res.memory, nullptr);
    res.memory = VK_NULL_HANDLE;
}

}  // namespace helpers

#endif  // !HELPERS_H
