#ifndef MATERIAL_HANDLER_H
#define MATERIAL_HANDLER_H

#include <memory>
#include <string>

#include "BufferManagerDescriptor.h"
#include "Game.h"
#include "Material.h"
#include "PBR.h"

namespace Material {

template <class TBase>
using ItemPointer = std::shared_ptr<TBase>;
    
template <class TDerived>
using ManagerType = Buffer::Manager::Descriptor<Material::Base, TDerived, ItemPointer>;

// TODO: inner class of Handler?
template <class TDerived>
class Manager : public ManagerType<TDerived> {
   public:
    Manager(const std::string &&name, const DESCRIPTOR &&descriptorType, const VkDeviceSize &&maxSize)
        : ManagerType<TDerived>{
              std::forward<const std::string>(name),
              std::forward<const DESCRIPTOR>(descriptorType),
              std::forward<const VkDeviceSize>(maxSize),
              false,
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
              static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
          } {}

    void updateTexture(const VkDevice &dev, const std::shared_ptr<Texture::Base> &pTexture) {
        for (auto &pItem : ManagerType<TDerived>::pItems)
            if (pItem->getTexture() == pTexture) {
                pItem->setTextureData();
                ManagerType<TDerived>::updateData(dev, pItem->BUFFER_INFO);
            }
    }
};

class Handler : public Game::Handler {
    friend class Mesh::Handler;

   public:
    Handler(Game *pGame);

    void init() override;
    template <class T>
    inline T &getMaterial(std::shared_ptr<Material::Base> &pMaterial) {
        const auto &offset = pMaterial->BUFFER_INFO.itemOffset;
        return std::ref(*(Material::Default::Base *)(getManager<T>().pItems[offset].get()));
    }

    inline void update(std::shared_ptr<Material::Base> &pMaterial) {
        if (pMaterial->DIRTY) {
            switch (pMaterial->TYPE) {
                case MATERIAL::DEFAULT:
                    defMgr_.updateData(shell().context().dev, pMaterial->BUFFER_INFO);
                    break;
                case MATERIAL::PBR:
                    pbrMgr_.updateData(shell().context().dev, pMaterial->BUFFER_INFO);
                    break;
                default:
                    assert(false && "Unhandled case");
            }
        }
    }

    void updateTexture(const std::shared_ptr<Texture::Base> &pTexture);

   private:
    void reset() override;

    template <typename TMaterialCreateInfo>
    std::shared_ptr<Material::Base> &makeMaterial(TMaterialCreateInfo *pCreateInfo) {
        /*static_assert(false, "Not implemented");*/
    }
    // DEFAULT
    template <>
    std::shared_ptr<Material::Base> &makeMaterial(Material::Default::CreateInfo *pCreateInfo) {
        defMgr_.insert(shell().context().dev, pCreateInfo);
        return defMgr_.pItems.back();
    }
    // PBR
    template <>
    std::shared_ptr<Material::Base> &makeMaterial(Material::PBR::CreateInfo *pCreateInfo) {
        pbrMgr_.insert(shell().context().dev, pCreateInfo);
        return pbrMgr_.pItems.back();
    }

    // clang-format off
    template <class T> inline Material::Manager<T>& getManager() { /*static_assert(false, "Not implemented");*/ }
    template <> inline Material::Manager<Material::Default::Base>& getManager() { return defMgr_; }
    template <> inline Material::Manager<Material::PBR::Base>& getManager() { return pbrMgr_; }
    // clang-format on

    Material::Manager<Material::Default::Base> defMgr_;
    Material::Manager<Material::PBR::Base> pbrMgr_;
};

}  // namespace Material

#endif  // !MATERIAL_HANDLER_H
