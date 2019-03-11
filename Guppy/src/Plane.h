#ifndef PLANE_H
#define PLANE_H

#include <vector>

#include "Constants.h"
#include "Face.h"
#include "Mesh.h"

namespace Plane {

constexpr auto DEFAULT_DIMENSION = 1.0f;
constexpr auto VERTEX_SIZE = 4;
constexpr auto INDEX_SIZE = 6;

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

};  // namespace Plane

#endif  //! PLANE_H