
#include "MaterialHandler.h"

#include "Shell.h"

namespace {
const VkDeviceSize DEFAULT_MAX_SIZE = 20;
const VkDeviceSize PBR_MAX_SIZE = 20;
}  // namespace

Material::Handler::Handler(Game *pGame)
    : Game::Handler(pGame),
      // MANAGERS
      defaultBuffer_{
          "Default Material", DEFAULT_MAX_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
          VK_SHARING_MODE_EXCLUSIVE},
      pbrBuffer_{
          "PBR Material", PBR_MAX_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
          VK_SHARING_MODE_EXCLUSIVE} {}

void Material::Handler::init() {
    reset();
    defaultBuffer_.init(shell().context(), settings());
    pbrBuffer_.init(shell().context(), settings());
}

void Material::Handler::reset() {
    defaultBuffer_.destroy(shell().context().dev);
    pbrBuffer_.destroy(shell().context().dev);
}

Buffer::Item::Info Material::Handler::insert(const VkDevice &dev, const Material::Info *pMaterialInfo) {
    switch (pMaterialInfo->type) {
        case MATERIAL::DEFAULT:
            return defaultBuffer_.insert(dev, Default::getData(pMaterialInfo));
            break;
        case MATERIAL::PBR:
            return pbrBuffer_.insert(dev, PBR::getData(pMaterialInfo));
            break;
        default:
            assert(false && "Add new materials here.");
            return {};
    }
}
