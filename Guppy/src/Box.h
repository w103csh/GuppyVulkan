#ifndef BOX_H
#define BOX_H

#include <vector>
#include <memory>

#include "Face.h"
#include "Geometry.h"
#include "Instance.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
namespace Box {

std::vector<Face> make(const Mesh::Geometry::Info& geoInfo = {});

class Color : public Mesh::Color {
   public:
    Color(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
          std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
   public:
    Texture(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
            std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Box
}  // namespace Mesh

#endif  // !BOX_H