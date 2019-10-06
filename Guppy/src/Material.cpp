#include "Material.h"

#include "Material.h"
// HANDLERS
#include "MaterialHandler.h"

// BASE

Material::Base::Base(const MATERIAL &&type, const CreateInfo *pCreateInfo)  //
    : TYPE(type),                                                           //
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
      Material::Base(MATERIAL::DEFAULT, pCreateInfo),
      Buffer::DataItem<Default::DATA>(pData)  //
{
    pData_->color = pCreateInfo->color;
    pData_->flags = pCreateInfo->flags;
    pData_->Ka = pCreateInfo->ambientCoeff;
    pData_->Ks = pCreateInfo->specularCoeff;
    pData_->opacity = pCreateInfo->opacity;
    pData_->shininess = pCreateInfo->shininess;
    pData_->eta = pCreateInfo->eta;
    pData_->reflectionFactor = pCreateInfo->reflectionFactor;
    setTextureData();
    setData();
}

void Material::Default::Base::setTinyobjData(const tinyobj::material_t &m) {
    pData_->shininess = m.shininess;
    pData_->Ka = {m.ambient[0], m.ambient[1], m.ambient[2]};
    pData_->color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    pData_->Ks = {m.specular[0], m.specular[1], m.specular[2]};
    setData();
}