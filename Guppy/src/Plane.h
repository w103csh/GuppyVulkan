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
    void createVertices(Mesh* pMesh, bool doubleSided);
};

// **********************
// ColorPlane
// **********************

class ColorPlane : public Plane, public ColorMesh {
   public:
    ColorPlane(MeshCreateInfo* pCreateInfo, bool doubleSided = false);
};

// **********************
// TexturePlane
// **********************

class TexturePlane : public Plane, public TextureMesh {
   public:
    TexturePlane(MeshCreateInfo* pCreateInfo, bool doubleSided = false);
};
