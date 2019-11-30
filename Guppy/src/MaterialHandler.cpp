/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "MaterialHandler.h"

#include "Shell.h"

Material::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      defMgr_{"Default Material", UNIFORM_DYNAMIC::MATERIAL_DEFAULT, 50},  //
      pbrMgr_{"PBR Material", UNIFORM_DYNAMIC::MATERIAL_PBR, 5},
      obj3dMgr_{"Default Obj3d Material", UNIFORM_DYNAMIC::MATERIAL_OBJ3D, 50, true} {}

void Material::Handler::init() {
    reset();
    defMgr_.init(shell().context());
    pbrMgr_.init(shell().context());
    obj3dMgr_.init(shell().context());
}

void Material::Handler::updateTexture(const std::shared_ptr<Texture::Base>& pTexture) {
    assert(pTexture->status == STATUS::READY);
    defMgr_.updateTexture(shell().context().dev, pTexture);
    pbrMgr_.updateTexture(shell().context().dev, pTexture);
    obj3dMgr_.updateTexture(shell().context().dev, pTexture);
}

void Material::Handler::reset() {
    defMgr_.destroy(shell().context().dev);
    pbrMgr_.destroy(shell().context().dev);
    obj3dMgr_.destroy(shell().context().dev);
}
