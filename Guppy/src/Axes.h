#ifndef AXES_H
#define AXES_H

#include <glm\glm.hpp>

#include "Helpers.h"
#include "Mesh.h"

const float AXES_MAX_SIZE = 2000.0f;

class Axes : public LineMesh {
   public:
    Axes(glm::mat4 model, bool showNegative = false);
};

class VisualHelper : public Axes {
   public:
    VisualHelper(const Object3d& obj3d, float size) : Axes(glm::scale(obj3d.getData().model, glm::vec3(size))) {}
};

#endif  // !AXES_H