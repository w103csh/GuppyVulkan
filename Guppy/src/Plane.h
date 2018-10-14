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
    virtual void createVertices(float width = 2.0f, float height = 2.0f) = 0;
};

// **********************
// ColorPlane
// **********************

class ColorPlane : public Plane, public ColorMesh {
   public:
    ColorPlane();
    ColorPlane(float width, float height, bool doubleSided = false, glm::vec3 pos = glm::vec3(), glm::mat4 rot = glm::mat4(1.0f));

   private:
    void createVertices(float width = 2.0f, float height = 2.0f) override;
};

// **********************
// TexturePlane
// **********************

class TexturePlane : public Plane, public TextureMesh {
   public:
    TexturePlane();

   private:
    void createVertices(float width = 2.0f, float height = 2.0f) override;
};

// class TexturePlane : public Plane, public TextureMesh {
//   public:
//    TexturePlane();
//    TexturePlane(std::string texturePath);
//    TexturePlane(float width, float height, bool doubleSided = false, glm::vec3 pos = glm::vec3(), glm::mat4 rot =
//    glm::mat4(1.0f));
//
//   private:
//    void createVertices(float width = 2.0f, float height = 2.0f) override;
//};
