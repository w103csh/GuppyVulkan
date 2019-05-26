#ifndef DESCRIPTOR_HANDLER_H
#define DESCRIPTOR_HANDLER_H

#include <map>
#include <set>
#include <string>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Game.h"
#include "DescriptorSet.h"
#include "ShaderHandler.h"

// clang-format off
namespace Mesh      { class Base; }
namespace Pipeline  { class Base; }
namespace Shader    { class Base; }
// clang-format on

namespace Descriptor {

// **********************
//      Handler
// **********************

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;

    // POOL
    inline const VkDescriptorPool& getPool() { return pool_; }
    // SET
    const Descriptor::Set::Base& getDescriptorSet(const DESCRIPTOR_SET& type) { return std::ref(*getSet(type).get()); }
    // DESCRIPTOR
    uint32_t getDescriptorCount(const Descriptor::bindingMapValue& value) const;
    void getReference(Mesh::Base& pMesh);

   private:
    void reset() override;

    // POOL
    void createPool();
    VkDescriptorPool pool_;

    // LAYOUT
    void createLayouts();

    // SET
    inline std::unique_ptr<Descriptor::Set::Base>& getSet(const DESCRIPTOR_SET& type) {
        for (auto& pSet : pDescriptorSets_) {
            if (pSet->TYPE == type) return pSet;
        }
        throw std::runtime_error("Unrecognized set type");
    }
    void allocateDescriptorSets(const std::unique_ptr<Descriptor::Set::Base>& pSet, Descriptor::Set::Resource& resource);
    void updateDescriptorSets(const std::unique_ptr<Descriptor::Set::Base>& pSet, Descriptor::Set::Resource& resource,
                              Mesh::Base& mesh) const;
    void getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet, std::vector<uint32_t>& dynamicOffsets,
                           Mesh::Base& mesh);
    std::vector<std::unique_ptr<Descriptor::Set::Base>> pDescriptorSets_;

    // BINDING
    VkDescriptorSetLayoutBinding getDecriptorSetLayoutBinding(const Descriptor::bindingMapKeyValue& keyValue,
                                                              const VkShaderStageFlags& stageFlags) const;
    // WRITE
    VkWriteDescriptorSet getWrite(const Descriptor::bindingMapKeyValue& keyValue) const;
    // virtual VkCopyDescriptorSet getCopy() = 0;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_HANDLER_H
