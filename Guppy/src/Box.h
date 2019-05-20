#ifndef BOX_H
#define BOX_H

#include <vector>
#include <memory>

#include "Face.h"
#include "Geometry.h"
#include "Instance.h"
#include "Mesh.h"

namespace Box {

std::vector<Face> make(const Geometry::CreateInfo& geoInfo = {});

class Color : public Mesh::Color {
   public:
    Color(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
          std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
   public:
    Texture(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
            std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Box

#endif  // !BOX_H