
#include "Material.h"

STATUS Material::Base::getStatus() const {
    if (hasTexture() && pTexture_->status != STATUS::READY) {
        return STATUS::PENDING_TEXTURE;
    }
    return STATUS::READY;
}

Material::Base::DATA Material::Base::getData(const std::unique_ptr<Material::Base> &pMaterial) {
    float xRepeat, yRepeat;
    xRepeat = yRepeat = pMaterial->getRepeat();

    // Deal with non-square images
    if (pMaterial->hasTexture()) {
        auto aspect = pMaterial->getTexture()->aspect;
        if (aspect > 1.0f) {
            yRepeat *= aspect;
        } else if (aspect < 1.0f) {
            xRepeat *= (1 / aspect);
        }
    }

    return {pMaterial->getAmbientCoeff(),
            pMaterial->getFlags(),
            pMaterial->getDiffuseCoeff(),
            pMaterial->getOpacity(),
            pMaterial->getSpecularCoeff(),
            pMaterial->getShininess(),
            xRepeat,
            yRepeat};
}