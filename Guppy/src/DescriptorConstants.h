
#ifndef DESCRIPTOR_CONSTANTS_H
#define DESCRIPTOR_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Types.h"
#include "UniformConstants.h"

enum class PIPELINE : uint32_t;
enum class RENDER_PASS : uint32_t;

enum class DESCRIPTOR_SET {
    // DEFAULT
    UNIFORM_DEFAULT,
    SAMPLER_DEFAULT,
    SAMPLER_CUBE_DEFAULT,
    PROJECTOR_DEFAULT,
    // PBR
    UNIFORM_PBR,
    // PARALLAX
    UNIFORM_PARALLAX,
    SAMPLER_PARALLAX,
    // SCREEN SPACE
    UNIFORM_SCREEN_SPACE_DEFAULT,
    SAMPLER_SCREEN_SPACE_DEFAULT,
    // Add new to DESCRIPTOR_SET_ALL in code file.
};

namespace Descriptor {

extern const std::set<DESCRIPTOR> DESCRIPTORS;

// Wrapper class for multimap. Uniforms are unique to desriptor sets, but
// samplers can have multiples of the same type. This wraps the insert to
// enforce this.
struct OffsetsMap {
   public:
    using Type = std::multimap<DESCRIPTOR, Uniform::offsets>;
    OffsetsMap(const Type&& map = {}) : offsets_(map) {}
    void insert(const DESCRIPTOR& key, const Uniform::offsets& value);
    const auto& map() const { return offsets_; }

   private:
    Type offsets_;
};

// VISITORS
// clang-format off
struct GetVkType {
    template <typename T> VkDescriptorType operator()(const T& type) const { assert(false); }
    VkDescriptorType operator()(const UNIFORM&          type) const { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }
    VkDescriptorType operator()(const UNIFORM_DYNAMIC&  type) const { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; }
    VkDescriptorType operator()(const COMBINED_SAMPLER& type) const { return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; }
};
struct IsUniform {
    template <typename T> bool operator()(const T& type) const { return false; }
    bool operator()(const UNIFORM& type) const { return true; }
};
struct GetUniform {
    template <typename T> const UNIFORM operator()(const T& type) const { return UNIFORM::DONT_CARE; }
    const UNIFORM operator()(const UNIFORM& type) const { return type; }
};
struct IsUniformDynamic {
    template <typename T> bool operator()(const T& type) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC& type) const { return true; }
};
struct GetUniformDynamic {
    template <typename T> const UNIFORM_DYNAMIC operator()(const T& type) const { return UNIFORM_DYNAMIC::DONT_CARE; }
    const UNIFORM_DYNAMIC operator()(const UNIFORM_DYNAMIC& type) const { return type; }
};
struct IsCombinedSampler {
    template <typename T> bool operator()(const T& type) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type) const { return true; }
};
struct GetCombinedSampler {
    template <typename T> const COMBINED_SAMPLER operator()(const T& type) const { return COMBINED_SAMPLER::DONT_CARE; }
    const COMBINED_SAMPLER operator()(const COMBINED_SAMPLER& type) const { return type; }
};
struct IsCombinedSamplerMaterial {
    template <typename T> bool operator()(const T& type) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type) const { return type == COMBINED_SAMPLER::MATERIAL; }
};
struct IsCombinedSamplerPipeline {
    template <typename T> bool operator()(const T& type) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type) const { return type == COMBINED_SAMPLER::PIPELINE; }
};
// clang-format on

namespace Set {

const std::string MACRO_ID_PREFIX = "_DS_";
extern const std::set<DESCRIPTOR_SET> ALL;

struct BindData {
    uint32_t firstSet;
    std::vector<std::vector<VkDescriptorSet>> descriptorSets;
    std::vector<uint32_t> dynamicOffsets;
};

// Data for creation and merging of descriptor set layouts.
struct Resource {
    OffsetsMap offsets;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::set<RENDER_PASS> passTypes;
    std::set<PIPELINE> pipelineTypes;
    VkShaderStageFlags stages = 0;
};

/*  0: Pass types the helper was made for.
 *  1: Set resource
 *  2: Set resource offset
 *  3: Set type
 *  4: Pass types the resource can be used for. (Can be different from resource pointer's
 *      type set because of culling. Saved for use in potential culling of shader modules.)
 */
using resourceTuple =
    std::tuple<std::set<RENDER_PASS>, const Descriptor::Set::Resource*, uint32_t, DESCRIPTOR_SET, std::set<RENDER_PASS>>;
using resourceTuples = std::vector<resourceTuple>;
using resourceHelpers = std::vector<resourceTuples>;

// Shader text replace info
using textReplaceTuple = std::tuple<std::string, uint8_t, OffsetsMap, std::set<RENDER_PASS>>;
using textReplaceTuples = std::vector<textReplaceTuple>;

}  // namespace Set

}  // namespace Descriptor

#endif  // !DESCRIPTOR_CONSTANTS_H
