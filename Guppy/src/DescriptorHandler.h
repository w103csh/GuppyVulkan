#ifndef DESCRIPTOR_HANDLER_H
#define DESCRIPTOR_HANDLER_H

#include <map>
#include <set>
#include <string>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Game.h"
#include "DescriptorSet.h"

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
    friend class Pipeline::Base;  // TODO: remove this!!!!!!!!!!!!!!!!!!!!!!!!!!!
   public:
    Handler(Game* pGame);

    void init() override;

    // POOL
    inline const VkDescriptorPool& getPool() { return pool_; }
    // SET
    const Descriptor::Set::Base& getDescriptorSet(const DESCRIPTOR_SET& type) { return std::ref(*getSet(type).get()); }
    // DESCRIPTOR
    void getBindData(Mesh::Base& pMesh);

   private:
    void reset() override;

    // POOL
    void createPool();
    VkDescriptorPool pool_;

    // LAYOUT
    void createLayouts();

    // DESCRIPTOR
    uint32_t getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets) const;

    inline std::unique_ptr<Descriptor::Set::Base>& getSet(const DESCRIPTOR_SET& type) {
        for (auto& pSet : pDescriptorSets_) {
            if (pSet->TYPE == type) return pSet;
        }
        throw std::runtime_error("Unrecognized set type");
    }

    void prepareDescriptorSet(std::unique_ptr<Descriptor::Set::Base>& pSet);

    Descriptor::Set::resourceHelpers getResourceHelpers(const std::set<RENDER_PASS> passTypes, const PIPELINE& pipelineType,
                                                        const std::vector<DESCRIPTOR_SET>& descSetTypes);

    void allocateDescriptorSets(const Descriptor::Set::Resource& resource, std::vector<VkDescriptorSet>& descriptorSets);
    void updateDescriptorSets(const std::unique_ptr<Descriptor::Set::Base>& pSet, const Mesh::Base& mesh,
                              const Descriptor::OffsetsMap& offsets, std::vector<VkDescriptorSet>& descriptorSets) const;

    VkWriteDescriptorSet getWrite(const Descriptor::bindingMapKeyValue& keyValue) const;
    void getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet, std::vector<uint32_t>& dynamicOffsets,
                           Mesh::Base& mesh);

    std::vector<std::unique_ptr<Descriptor::Set::Base>> pDescriptorSets_;

    // BINDING
    VkDescriptorSetLayoutBinding getDecriptorSetLayoutBinding(const Descriptor::bindingMapKeyValue& keyValue,
                                                              const VkShaderStageFlags& stageFlags,
                                                              const Uniform::offsets& offsets) const;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_HANDLER_H
