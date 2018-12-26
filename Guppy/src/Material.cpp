
#include "Material.h"

STATUS Material::getStatus() const {
    if (hasTexture() && pTexture_->status != STATUS::READY) {
        return STATUS::PENDING_TEXTURE;
    }
    return STATUS::READY;
}

Material::Data Material::getData() const {
    float xRepeat, yRepeat;
    xRepeat = yRepeat = repeat_;

    // Deal with non-square images
    if (hasTexture()) {
        if (pTexture_->aspect > 1.0f) {
            yRepeat *= pTexture_->aspect;
        } else if (pTexture_->aspect < 1.0f) {
            xRepeat *= (1 / pTexture_->aspect);
        }
    }

    return {
        ambientCoeff_, flags_, diffuseCoeff_, opacity_, specularCoeff_, shininess_, xRepeat, yRepeat,
    };
}