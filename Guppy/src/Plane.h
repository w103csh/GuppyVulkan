#ifndef PLANE_H
#define PLANE_H

#include <vector>

#include "Constants.h"
#include "Face.h"
#include "Mesh.h"

// clang-format off
namespace Mesh      { class Handler; }
// clang-format on

namespace Plane {

constexpr auto DEFAULT_DIMENSION = 1.0f;
constexpr auto VERTEX_SIZE = 4;
constexpr auto INDEX_SIZE = 6;

std::vector<Face> make(bool doubleSided = false);

class Color : public Mesh::Color {
    friend class Mesh::Handler;

   protected:
    Color(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
          std::shared_ptr<Material::Base>& pMaterial, bool doubleSided = false);
};

class Texture : public Mesh::Texture {
    friend class Mesh::Handler;

   protected:
    Texture(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
            std::shared_ptr<Material::Base>& pMaterial, bool doubleSided = false);
};

};  // namespace Plane

#endif  //! PLANE_H