#ifndef MATERIAL_HANDLER_H
#define MATERIAL_HANDLER_H

#include "BufferManager.h"
#include "Material.h"
#include "PBR.h"

#include "Game.h"

namespace Material {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;

    // Let the nightmare begin...
    Buffer::Item::Info insert(const VkDevice &dev, const Material::Info *pMaterialInfo);

    template <typename T>
    T &get(const Buffer::Item::Info &info);
    // DEFAULT
    template <>
    Material::Default::DATA &get(const Buffer::Item::Info &info) {
        return defaultBuffer_.get(info);
    }
    // PBR
    template <>
    PBR::DATA &get(const Buffer::Item::Info &info) {
        return pbrBuffer_.get(info);
    }

   private:
    void reset() override;
    // TODO: some clever way to make this not hardcoded???
    Buffer::Manager<Material::Default::DATA> defaultBuffer_;
    Buffer::Manager<Material::PBR::DATA> pbrBuffer_;
};

}  // namespace Material

#endif  // !MATERIAL_HANDLER_H
