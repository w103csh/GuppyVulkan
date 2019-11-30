/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Material.h"

#include "Material.h"
// HANDLERS
#include "MaterialHandler.h"

// BASE

Material::Base::Base(const DESCRIPTOR &&descType, const CreateInfo *pCreateInfo)  //
    : Descriptor::Base(std::forward<const DESCRIPTOR>(descType)),
      pTexture_(pCreateInfo->pTexture),
      repeat_(pCreateInfo->repeat) {
    status_ = (hasTexture() && pTexture_->status != STATUS::READY) ? STATUS::PENDING_TEXTURE : STATUS::READY;
}

// FUNCTIONS

STATUS Material::SetDefaultTextureData(Material::Base *pMaterial, DATA *pData) {
    assert(pMaterial != nullptr);

    float xRepeat, yRepeat;
    xRepeat = yRepeat = pMaterial->getRepeat();

    if (pMaterial->getTexture() != nullptr) {
        // REPEAT
        // Deal with non-square images
        auto aspect = pMaterial->getTexture()->aspect;
        if (aspect > 1.0f) {
            yRepeat *= aspect;
        } else if (aspect < 1.0f) {
            xRepeat *= (1 / aspect);
        }
        // FLAGS
        pData->flags &= ~Material::FLAG::PER_MAX_ENUM;
        pData->flags |= Material::FLAG::PER_TEXTURE_COLOR;
        pData->texFlags = pMaterial->getTexture()->flags;
    } else {
        pData->texFlags = 0;
    }

    pData->xRepeat = xRepeat;
    pData->yRepeat = yRepeat;

    pMaterial->dirty = true;

    return (pMaterial->getStatus() == STATUS::PENDING_TEXTURE && pMaterial->getTexture()->status == STATUS::READY)
               ? STATUS::READY
               : pMaterial->getStatus();
}

// DEFAULT

Material::Default::Base::Base(const Buffer::Info &&info, Default::DATA *pData, const Default::CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Material::Base(UNIFORM_DYNAMIC::MATERIAL_DEFAULT, pCreateInfo),
      Buffer::DataItem<Default::DATA>(pData)  //
{
    SetData(pCreateInfo, pData_);
    setTextureData();
    setData();
}

void Material::Default::Base::setTinyobjData(const tinyobj::material_t &m) {
    SetTinyobjData(m, pData_);
    setData();
}