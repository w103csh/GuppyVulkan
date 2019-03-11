#include "Material.h"

#include "Material.h"

#include "MaterialHandler.h"

Material::Base::Base(Material::CreateInfo *pCreateInfo)  //
    : TYPE(pCreateInfo->type),                           //
      pTexture_(pCreateInfo->pTexture) {
    status_ = (hasTexture() && pTexture_->status != STATUS::READY) ? STATUS::PENDING_TEXTURE : STATUS::READY;
}

Material::Default::Base::Base(const Buffer::Info &&info, DATA *pData, Default::CreateInfo *pCreateInfo)
    : Material::Base(pCreateInfo),                           //
      Buffer::DataItem<DATA>(pData),                         //
      Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      repeat_(pCreateInfo->repeat) {
    pData_->Ka = pCreateInfo->ambientCoeff;
    pData_->flags = pCreateInfo->flags;
    pData_->Kd = pCreateInfo->diffuseCoeff;
    pData_->opacity = pCreateInfo->opacity;
    pData_->Ks = pCreateInfo->specularCoeff;
    pData_->shininess = pCreateInfo->shininess;
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
        pData_->texFlags = pTexture_->flags;
    }

    pData_->xRepeat = xRepeat;
    pData_->yRepeat = yRepeat;
}

void Material::Default::Base::setTinyobjData(const tinyobj::material_t &m) {
    pData_->shininess = m.shininess;
    pData_->Ka = {m.ambient[0], m.ambient[1], m.ambient[2]};
    pData_->Kd = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    pData_->Ks = {m.specular[0], m.specular[1], m.specular[2]};
    setData();
}
