#include "Material.h"

#include "Material.h"

#include "MaterialHandler.h"

Material::Base::Base(Material::Info* pInfo) : TYPE(pInfo->type), pTexture_(pInfo->pTexture) {
    // bufferInfo = pInfo->pHandler->insert(dev, pInfo);
}
