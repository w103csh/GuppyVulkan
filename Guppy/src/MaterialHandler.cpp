
#include "MaterialHandler.h"

#include "Shell.h"

Material::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      defMgr_{"Default Material", UNIFORM_DYNAMIC::MATERIAL_DEFAULT, 50},  //
      pbrMgr_{"PBR Material", UNIFORM_DYNAMIC::MATERIAL_PBR, 50},
      fntnMgr_{"Particle Fountain Material", UNIFORM_DYNAMIC::MATERIAL_PARTICLE_FOUNTAIN, 50, true} {}

void Material::Handler::init() {
    reset();
    defMgr_.init(shell().context(), settings());
    pbrMgr_.init(shell().context(), settings());
    fntnMgr_.init(shell().context(), settings());
}

void Material::Handler::updateTexture(const std::shared_ptr<Texture::Base>& pTexture) {
    assert(pTexture->status == STATUS::READY);
    defMgr_.updateTexture(shell().context().dev, pTexture);
    pbrMgr_.updateTexture(shell().context().dev, pTexture);
    fntnMgr_.updateTexture(shell().context().dev, pTexture);
}

void Material::Handler::reset() {
    defMgr_.destroy(shell().context().dev);
    pbrMgr_.destroy(shell().context().dev);
    fntnMgr_.destroy(shell().context().dev);
}
