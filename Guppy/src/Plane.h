#ifndef PLANE_H
#define PLANE_H

#include <vector>

#include "ConstantsAll.h"
#include "Face.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
namespace Plane {

constexpr auto DEFAULT_DIMENSION = 1.0f;
constexpr auto VERTEX_SIZE = 4;
constexpr auto INDEX_SIZE = 6;

std::vector<Face> make(const Mesh::Geometry::Info& geoInfo = {});

class Color : public Mesh::Color {
    friend class Handler;

   protected:
    Color(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
          std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
    friend class Handler;

   protected:
    Texture(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
            std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Plane
};  // namespace Mesh

#endif  //! PLANE_H