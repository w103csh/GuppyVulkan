
#include "Plane.h"

// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

namespace {
void addFaces(Base* pMesh, const Mesh::Geometry::Info& geoInfo) {
    // Mimic approach in loadObj in FileLoader. This way everything is
    // using the same ideas (for testing)...
    unique_vertices_map_smoothing vertexMap = {};
    auto faces = Plane::make(geoInfo);
    for (auto& face : faces) face.indexVertices(vertexMap, pMesh);
}
}  // namespace

std::vector<Face> Plane::make(const Mesh::Geometry::Info& geoInfo) {
    float width, height;
    width = height = Plane::DEFAULT_DIMENSION;
    float l = (width / 2 * -1), r = (width / 2);
    float b = (height / 2 * -1), t = (height / 2);

    std::vector<Face> faces;

    // bottom left
    faces.push_back({});
    faces.back()[0] = {
        // geom - bottom left
        {l, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {1.0f, 0.0f, 0.0f, 1.0f},  // color (red)
        {0.0f, 1.0f},              // texCoord (bottom left)
        {},                        // tangent
        {}                         // bitangent
    };
    faces.back()[1] = {
        // geom - bottom right
        {r, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 0.0f, 1.0f, 1.0f},  // color (blue)
        {1.0f, 1.0f},              // texCoord (bottom right)
        {},                        // tangent
        {}                         // bitangent
    };
    faces.back()[2] = {
        // geom - top left
        {l, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 1.0f, 0.0f, 1.0f},  // color (green)
        {0.0f, 0.0f},              // texCoord (top left)
        {},                        // tangent
        {}                         // bitangent
    };

    if (geoInfo.reverseFaceWinding) {
        faces.back().reverseWinding();
        // faces.back().setSmoothingGroup(1);
    }

    // top right
    faces.push_back({});
    faces.back()[0] = {
        // geom - top left
        {l, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 1.0f, 0.0f, 1.0f},  // color (green)
        {0.0f, 0.0f},              // texCoord (top left)
        {},                        // tangent
        {}                         // bitangent
    };
    faces.back()[1] = {
        // geom - bottom right
        {r, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 0.0f, 1.0f, 1.0f},  // color (blue)
        {1.0f, 1.0f},              // texCoord (bottom right)
        {},                        // tangent
        {}                         // bitangent
    };
    faces.back()[2] = {
        // geom - top right
        {r, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {1.0f, 1.0f, 0.0f, 1.0f},  // color (yellow)
        {1.0f, 0.0f},              // texCoord (top right)
        {},                        // tangent
        {}                         // bitangent
    };

    if (geoInfo.reverseFaceWinding) {
        faces.back().reverseWinding();
        // faces.back().setSmoothingGroup(1);
    }

    // TODO: validate face size is DEFAULT_DIMENSION

    for (auto& face : faces) face.transform(geoInfo.transform);

    return faces;
}

Plane::Color::Color(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                    std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Color(handler, "Color Plane", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->settings.geometryInfo);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

Plane::Texture::Texture(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                        std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Texture(handler, "Texture Plane", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->settings.geometryInfo);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
