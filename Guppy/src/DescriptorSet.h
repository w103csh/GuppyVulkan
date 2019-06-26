#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <iterator>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ConstantsAll.h"
#include "DescriptorConstants.h"

namespace Descriptor {

class Handler;

// key:     { binding, arrayElement }
typedef std::pair<uint32_t, uint32_t> bindingMapKey;
// value:   { descriptorType, (optional) descriptor ID } // TODO: proper ID instead of string
typedef std::pair<DESCRIPTOR, std::string_view> bindingMapValue;
typedef std::pair<const bindingMapKey, bindingMapValue> bindingMapKeyValue;
typedef std::map<bindingMapKey, bindingMapValue> bindingMap;

//  SET
namespace Set {

const uint32_t OFFSET_ALL = UINT32_MAX;

class Base {
    friend class Descriptor::Handler;

   public:
    Base(const DESCRIPTOR_SET&& type, const std::string&& macroName, const Descriptor::bindingMap&& bindingMap);

    const DESCRIPTOR_SET TYPE;
    const std::string MACRO_NAME;
    const Descriptor::bindingMap BINDING_MAP;

    inline bool isInitialized() const { return !resources_[defaultResourceOffset_].pipelineTypes.empty(); }

    inline const auto& getResource(const uint32_t& offset) const { return resources_[offset]; }
    const Descriptor::OffsetsMap getDescriptorOffsets(const uint32_t& offset) const;
    constexpr const auto& getDefaultResourceOffset() const { return defaultResourceOffset_; }

    void updateOffsets(const Uniform::offsetsMap offsetsMap, const Descriptor::bindingMapKeyValue& bindingMapKeyValue,
                       const PIPELINE& pipelineType);

    bool hasTextureMaterial() const;

    // ITERATOR
    void findResourceForPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type);
    void findResourceForDefaultPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type);
    void findResourceSimilar(std::vector<Resource>::iterator& it, const PIPELINE& piplineType,
                             const std::set<RENDER_PASS>& passTypes, const DESCRIPTOR& descType,
                             const Uniform::offsets& offsets);

   private:
    inline auto& getDefaultResource() { return resources_[defaultResourceOffset_]; }

    const uint32_t defaultResourceOffset_;
    std::vector<Resource> resources_;
    Uniform::offsetsMap uniformOffsets_;
    // Texture offsets (0 if no texture) to resource offset map
    std::map<uint32_t, std::map<uint32_t, std::vector<VkDescriptorSet>>> descriptorSetsMap_;
};

//  DEFAULT
namespace Default {
class Uniform : public Set::Base {
   public:
    Uniform();
};
class Sampler : public Set::Base {
   public:
    Sampler();
};
class CubeSampler : public Set::Base {
   public:
    CubeSampler();
};
class ProjectorSampler : public Set::Base {
   public:
    ProjectorSampler();
};
}  // namespace Default

}  // namespace Set
}  // namespace Descriptor

#endif  // !DESCRIPTOR_SET_H