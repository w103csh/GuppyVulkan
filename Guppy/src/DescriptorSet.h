#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Helpers.h"

namespace Descriptor {

class Handler;

// **********************
//      Binding map
// **********************

// key:     { binding, arrayElement }
typedef std::pair<uint32_t, uint32_t> bindingMapKey;
// value:   { descriptorType, offsets, (optional) descriptor ID } // TODO: proper ID instead of string
typedef std::tuple<DESCRIPTOR, std::set<uint32_t>, std::string> bindingMapValue;
typedef std::pair<const bindingMapKey, bindingMapValue> bindingMapKeyValue;
typedef std::map<bindingMapKey, bindingMapValue> bindingMap;

// **********************
//      Set
// **********************

namespace Set {

const uint32_t OFFSET_ALL = UINT32_MAX;

struct Resource {
    Resource(uint32_t offset) : offset(offset), status(STATUS::PENDING) {}
    const uint32_t offset;
    STATUS status;
    std::vector<VkDescriptorSet> descriptorSets;
};

class Base {
    friend class Descriptor::Handler;

   public:
    Base(const DESCRIPTOR_SET&& type, const std::string&& macroName, const Descriptor::bindingMap&& bindingMap);

    const DESCRIPTOR_SET TYPE;
    const std::string MACRO_NAME;
    const Descriptor::bindingMap BINDING_MAP;

    VkDescriptorSetLayout layout;
    VkShaderStageFlags stages;

    Descriptor::Set::Resource& getResource(const uint32_t& offset);

   private:
    std::vector<Descriptor::Set::Resource> resources_;
};

// **********************
//      Default
// **********************

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