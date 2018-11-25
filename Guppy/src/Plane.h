#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Constants.h"

constexpr auto PLANE_VERTEX_SIZE = 4;
constexpr auto PLANE_INDEX_SIZE = 6;

// **********************
// Plane
// **********************

class Plane {
   protected:
    void createVertices(Mesh *pMesh, bool doubleSided);
};

// **********************
// ColorPlane
// **********************

class ColorPlane : public Plane, public ColorMesh {
   public:
    ColorPlane(std::unique_ptr<Material> pMaterial = std::make_unique<Material>(), glm::mat4 rot = glm::mat4(1.0f),
               bool doubleSided = false);
};

// **********************
// TexturePlane
// **********************

class TexturePlane : public Plane, public TextureMesh {
   public:
    TexturePlane(std::shared_ptr<Texture::Data> pTexture, glm::mat4 model = glm::mat4(1.0f), bool doubleSided = false);
    TexturePlane(std::unique_ptr<Material> pMaterial, glm::mat4 model = glm::mat4(1.0f), bool doubleSided = false);
};
