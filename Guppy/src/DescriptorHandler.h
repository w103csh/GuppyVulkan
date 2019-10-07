#ifndef DESCRIPTOR_HANDLER_H
#define DESCRIPTOR_HANDLER_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "Game.h"
#include "DescriptorSet.h"

// clang-format off
namespace Material  { class Base; }
namespace Texture   { class Base; }
namespace Shader    { class Base; }
// clang-format on

namespace Descriptor {

// HANDLER

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;

    // POOL
    inline const VkDescriptorPool& getPool() { return pool_; }

    // SET
    const Descriptor::Set::Base& getDescriptorSet(const DESCRIPTOR_SET& type) const {
        for (auto& pSet : pDescriptorSets_) {
            if (pSet->TYPE == type) return std::ref(*pSet.get());
        }
        assert(false);
        throw std::runtime_error("Unrecognized set type");
    }
    Descriptor::Set::resourceHelpers getResourceHelpers(const std::set<PASS> passTypes, const PIPELINE& pipelineType,
                                                        const std::vector<DESCRIPTOR_SET>& descSetTypes) const;

    // DESCRIPTOR
    void getBindData(const PIPELINE& pipelineType, Descriptor::Set::bindDataMap& bindDataMap,
                     const std::shared_ptr<Material::Base>& pMaterial = nullptr,
                     const std::shared_ptr<Texture::Base>& pTexture = nullptr);
    void updateBindData(const std::vector<std::string> textureIds);

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
        assert(false);
        exit(EXIT_FAILURE);
    }

    void prepareDescriptorSet(std::unique_ptr<Descriptor::Set::Base>& pSet);

    void allocateDescriptorSets(const Descriptor::Set::Resource& resource, std::vector<VkDescriptorSet>& descriptorSets);

    void updateDescriptorSets(const Descriptor::bindingMap& bindingMap, const Descriptor::OffsetsMap& offsets,
                              Set::resourceInfoMapSetsPair& pair, const std::shared_ptr<Material::Base>& pMaterial = nullptr,
                              const std::shared_ptr<Texture::Base>& pTexture = nullptr) const;

    VkWriteDescriptorSet getWrite(const Descriptor::bindingMapKeyValue& keyValue, const VkDescriptorSet& set) const;
    void getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet, std::vector<uint32_t>& dynamicOffsets,
                           const std::shared_ptr<Material::Base>& pMaterial);

    // BINDING
    VkDescriptorSetLayoutBinding getDecriptorSetLayoutBinding(const Descriptor::bindingMapKeyValue& keyValue,
                                                              const VkShaderStageFlags& stageFlags,
                                                              const Uniform::offsets& offsets) const;

    std::vector<std::unique_ptr<Descriptor::Set::Base>> pDescriptorSets_;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_HANDLER_H
