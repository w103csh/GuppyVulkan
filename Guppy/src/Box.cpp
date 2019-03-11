
#include "Box.h"

#include "Helpers.h"
#include "Plane.h"

namespace {
void addPlane(const glm::mat4& t, const bool doubleSided, std::vector<Face>& faces) {
    auto planeFaces = Plane::make(doubleSided);
    for (auto& face : planeFaces) face.transform(t);
    faces.insert(faces.end(), planeFaces.begin(), planeFaces.end());
}
void addFaces(Mesh::Base* pMesh, bool& doubleSided) {
    // Mimic approach in loadObj in FileLoader. This way everything is
    // using the same ideas (for testing)...
    unique_vertices_map_non_smoothing vertexMap = {};
    auto faces = Box::make(doubleSided);
    for (auto& face : faces) face.indexVertices(vertexMap, pMesh);
}
}  // namespace

std::vector<Face> Box::make(bool doubleSided) {
    auto offset = Plane::DEFAULT_DIMENSION / 2.0f;
    std::vector<Face> faces;

    // front
    auto transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, 0.0f, offset});
    addPlane(transform, doubleSided, faces);
    // back
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, 0.0f, -offset}, M_PI_FLT, CARDINAL_Y);
    addPlane(transform, doubleSided, faces);

    // top
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, offset, 0.0f}, -M_PI_2_FLT, CARDINAL_X);
    addPlane(transform, doubleSided, faces);
    // bottom
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, -offset, 0.0f}, M_PI_2_FLT, CARDINAL_X);
    addPlane(transform, doubleSided, faces);

    // left
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{offset, 0.0f, 0.0f}, M_PI_2_FLT, CARDINAL_Y);
    addPlane(transform, doubleSided, faces);
    // right
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{-offset, 0.0f, 0.0f}, -M_PI_2_FLT, CARDINAL_Y);
    addPlane(transform, doubleSided, faces);

    return faces;
}

Box::Color::Color(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
                  bool doubleSided)
    : Mesh::Color(handler, "ColorPlane", pCreateInfo, pMaterial) {
    addFaces(this, doubleSided);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

Box::Texture::Texture(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
                      bool doubleSided)
    : Mesh::Texture(handler, "TexturePlane", pCreateInfo, pMaterial) {
    addFaces(this, doubleSided);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
