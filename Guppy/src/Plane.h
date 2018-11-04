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
   public:
    static void createIndices(std::vector<VB_INDEX_TYPE> &indices, bool doubleSided = false);

   private:
    virtual void createVertices() = 0;
};

// **********************
// ColorPlane
// **********************

class ColorPlane : public Plane, public ColorMesh {
   public:
    ColorPlane(glm::mat4 rot = glm::mat4(1.0f), bool doubleSided = false);

   private:
    void createVertices() override;
};

// **********************
// TexturePlane
// **********************

class TexturePlane : public Plane, public TextureMesh {
   public:
    TexturePlane(std::shared_ptr<Texture::Data> pTexture, glm::mat4 model = glm::mat4(1.0f), bool doubleSided = false);

   private:
    void createVertices() override;
};
