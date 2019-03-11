#ifndef BOX_H
#define BOX_H

#include <vector>

#include "Face.h"
#include "Mesh.h"

namespace Box {

std::vector<Face> make(bool doubleSided = false);

class Color : public Mesh::Color {
   public:
    Color(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
          bool doubleSided = false);
};

class Texture : public Mesh::Texture {
   public:
    Texture(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
            bool doubleSided = false);
};

}  // namespace Box

#endif  // !BOX_H