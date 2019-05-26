#include "Material.h"

#include "Material.h"

#include "MaterialHandler.h"

Material::Base::Base(const MATERIAL &&type, Material::CreateInfo *pCreateInfo)  //
    : TYPE(type),                                                               //
      pTexture_(pCreateInfo->pTexture),
      repeat_(pCreateInfo->repeat) {
    status_ = (hasTexture() && pTexture_->status != STATUS::READY) ? STATUS::PENDING_TEXTURE : STATUS::READY;
}

Material::Default::Base::Base(const Buffer::Info &&info, Default::DATA *pData, Default::CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<Default::DATA>(pData),                //
      Material::Base(MATERIAL::DEFAULT, pCreateInfo)         //
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

void Material::Default::Base::setTextureData() {
    float xRepeat, yRepeat;
    xRepeat = yRepeat = repeat_;

    if (pTexture_ != nullptr) {
        // REPEAT
        // Deal with non-square images
        auto aspect = pTexture_->aspect;
        if (aspect > 1.0f) {
            yRepeat *= aspect;
        } else if (aspect < 1.0f) {
            xRepeat *= (1 / aspect);
        }
        // FLAGS
        pData_->flags &= ~Material::FLAG::PER_ALL;
        pData_->flags |= Material::FLAG::PER_TEXTURE_COLOR;
        pData_->texFlags = pTexture_->flags;
    } else {
        pData_->texFlags = 0;
    }

    pData_->xRepeat = xRepeat;
    pData_->yRepeat = yRepeat;

    if (status_ == STATUS::PENDING_TEXTURE && pTexture_->status == STATUS::READY)  //
        status_ = STATUS::READY;

    DIRTY = true;
}

void Material::Default::Base::setTinyobjData(const tinyobj::material_t &m) {
    pData_->shininess = m.shininess;
    pData_->Ka = {m.ambient[0], m.ambient[1], m.ambient[2]};
    pData_->color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    pData_->Ks = {m.specular[0], m.specular[1], m.specular[2]};
    setData();
}
