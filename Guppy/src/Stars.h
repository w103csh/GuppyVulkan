#ifndef STARS_H
#define STARS_H

#include <memory>

#include "Instance.h"
#include "Material.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
class Stars : public Mesh::Color {
   public:
    Stars(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};
}  // namespace Mesh

#endif  // !STARS_H