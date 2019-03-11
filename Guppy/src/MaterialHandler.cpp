
#include "MaterialHandler.h"

#include "Shell.h"

namespace {
const VkDeviceSize DEFAULT_MAX_SIZE = 20;
const VkDeviceSize PBR_MAX_SIZE = 20;
}  // namespace

Material::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      defMgr_{"Default Material", DESCRIPTOR::MATERIAL_DEFAULT, 50},  //
      pbrMgr_{"PBR Material", DESCRIPTOR::MATERIAL_PBR, 50} {}

void Material::Handler::init() {
    reset();
    defMgr_.init(shell().context(), settings());
    pbrMgr_.init(shell().context(), settings());
}

void Material::Handler::reset() {
    defMgr_.destroy(shell().context().dev);
    pbrMgr_.destroy(shell().context().dev);
}
