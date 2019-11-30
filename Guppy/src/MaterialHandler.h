/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MATERIAL_HANDLER_H
#define MATERIAL_HANDLER_H

#include <memory>
#include <string>

#include "DescriptorManager.h"
#include "Game.h"
#include "Material.h"
#include "Particle.h"
#include "PBR.h"

namespace Material {

template <class TBase>
using ItemPointer = std::shared_ptr<TBase>;

template <class TDerived>
using ManagerType = Descriptor::Manager<Material::Base, TDerived, ItemPointer>;

// TODO: inner class of Handler?
template <class TDerived>
class Manager : public ManagerType<TDerived> {
   public:
    Manager(const std::string &&name, const DESCRIPTOR &&descriptorType, const VkDeviceSize &&maxSize,
            const bool keepMapped = false)
        : ManagerType<TDerived>{
              std::forward<const std::string>(name),
              std::forward<const DESCRIPTOR>(descriptorType),
              std::forward<const VkDeviceSize>(maxSize),
              std::forward<const bool>(keepMapped),
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
    friend class ::Particle::Handler;

   public:
    Handler(Game *pGame);

    void init() override;

    template <class T>
    inline auto &getMaterial(std::shared_ptr<Material::Base> &pMaterial) {
        return std::ref<T>(pMaterial.get());
    }

    template <class T>
    inline void update(const std::shared_ptr<T> &pMaterial, const int index = -1) {
        getManager<T>().updateData(shell().context().dev, pMaterial->BUFFER_INFO, index);
    }

    inline void update(const std::shared_ptr<Material::Base> &pMaterial, const int index = -1) {
        // clang-format off
        switch (std::visit(Descriptor::GetUniformDynamic{}, pMaterial->getDescriptorType())) {
            case UNIFORM_DYNAMIC::MATERIAL_DEFAULT:         update(std::static_pointer_cast<Material::Default::Base>(pMaterial), std::forward<const int>(index)); break;
            case UNIFORM_DYNAMIC::MATERIAL_PBR:             update(std::static_pointer_cast<Material::PBR::Base>(pMaterial), std::forward<const int>(index)); break;
            case UNIFORM_DYNAMIC::MATERIAL_OBJ3D:           update(std::static_pointer_cast<Material::Obj3d::Default>(pMaterial), std::forward<const int>(index)); break;
            default: assert(false && "Unhandled material type."); exit(EXIT_FAILURE);
        }
        // clang-format on
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
    // OBJ3D
    template <>
    std::shared_ptr<Material::Base> &makeMaterial(Material::Obj3d::CreateInfo *pCreateInfo) {
        obj3dMgr_.insert(shell().context().dev, pCreateInfo);
        return obj3dMgr_.pItems.back();
    }

    // clang-format off
    template <class T> inline Manager<T>& getManager() { /*static_assert(false, "Not implemented");*/ }
    template <> inline Manager<Material::Default::Base>& getManager() { return defMgr_; }
    template <> inline Manager<Material::PBR::Base>& getManager() { return pbrMgr_; }
    template <> inline Manager<Material::Obj3d::Default>& getManager() { return obj3dMgr_; }
    // clang-format on

    Manager<Material::Default::Base> defMgr_;
    Manager<Material::PBR::Base> pbrMgr_;
    Manager<Material::Obj3d::Default> obj3dMgr_;
};

}  // namespace Material

#endif  // !MATERIAL_HANDLER_H
