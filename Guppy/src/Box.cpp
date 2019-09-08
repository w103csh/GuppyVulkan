
#include "Box.h"

#include "Helpers.h"
#include "Plane.h"
// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

namespace {
void addPlane(const glm::mat4& t, const Geometry::CreateInfo& geoInfo, std::vector<Face>& faces) {
    auto planeFaces = Plane::make(geoInfo);
    for (auto& face : planeFaces) face.transform(t);
    faces.insert(faces.end(), planeFaces.begin(), planeFaces.end());
}
void addFaces(Base* pMesh, const Geometry::CreateInfo& geoInfo) {
    // Mimic approach in loadObj in FileLoader. This way everything is
    // using the same ideas (for testing)...
    unique_vertices_map_non_smoothing vertexMap = {};
    auto faces = Box::make(geoInfo);
    for (auto& face : faces) face.indexVertices(vertexMap, pMesh);
}
}  // namespace

std::vector<Face> Box::make(const Geometry::CreateInfo& geoInfo) {
    auto offset = Plane::DEFAULT_DIMENSION / 2.0f;
    std::vector<Face> faces;

    // front
    auto transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, 0.0f, offset});
    addPlane(transform, geoInfo, faces);
    // back
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, 0.0f, -offset}, M_PI_FLT, CARDINAL_Y);
    addPlane(transform, geoInfo, faces);

    // top
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, offset, 0.0f}, -M_PI_2_FLT, CARDINAL_X);
    addPlane(transform, geoInfo, faces);
    // bottom
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, -offset, 0.0f}, M_PI_2_FLT, CARDINAL_X);
    addPlane(transform, geoInfo, faces);

    // left
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{offset, 0.0f, 0.0f}, M_PI_2_FLT, CARDINAL_Y);
    addPlane(transform, geoInfo, faces);
    // right
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{-offset, 0.0f, 0.0f}, -M_PI_2_FLT, CARDINAL_Y);
    addPlane(transform, geoInfo, faces);

    return faces;
}

Box::Color::Color(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                  std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Color(handler, "Color Box", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->geometryCreateInfo);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

Box::Texture::Texture(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                      std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Texture(handler, "Texture Box", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->geometryCreateInfo);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
