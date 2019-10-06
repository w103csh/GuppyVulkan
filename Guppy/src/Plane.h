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

struct Info {
    uint32_t horizontalDivisions = 1;
    uint32_t verticalDivisions = 1;
};

struct CreateInfo : public Mesh::CreateInfo {
    Plane::Info planeInfo;
};

std::vector<Face> make(const Plane::Info& planeInfo = {}, const Mesh::Geometry::Info& geoInfo = {});

class Color : public Mesh::Color {
    friend class Handler;

   protected:
    Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
    friend class Handler;

   protected:
    Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
            std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Plane
};  // namespace Mesh

#endif  //! PLANE_H