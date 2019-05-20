#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "Helpers.h"

namespace Descriptor {

class Handler;

// **********************
//      Binding map
// **********************

// key:     { set, binding, arrayElement }
typedef std::tuple<uint32_t, uint32_t, uint32_t> bindingMapKey;
// value:   { descriptorType, offsets }
typedef std::pair<DESCRIPTOR, std::set<uint32_t>> bindingMapValue;
typedef std::pair<const bindingMapKey, bindingMapValue> bindingMapKeyValue;
typedef std::map<bindingMapKey, bindingMapValue> bindingMap;

// **********************
//      Set
// **********************

namespace Set {

const uint32_t OFFSET_ALL = UINT32_MAX;

struct Resource {
    uint32_t offset;
    STATUS status = STATUS::PENDING;
    std::vector<VkDescriptorSet> descriptorSets;
};

class Base {
    friend class Descriptor::Handler;

   public:
    Base(const DESCRIPTOR_SET&& type, const Descriptor::bindingMap&& bindingMap);

    const DESCRIPTOR_SET TYPE;
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
}  // namespace Default

}  // namespace Set

}  // namespace Descriptor

#endif  // !DESCRIPTOR_SET_H