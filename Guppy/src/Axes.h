#ifndef AXES_H
#define AXES_H

#include <glm\glm.hpp>

#include "Helpers.h"
#include "Mesh.h"

const float AXES_MAX_SIZE = 2000.0f;

class Axes : public LineMesh {
   public:
    Axes(glm::mat4 model = glm::mat4(1.0f), float lineSize = 1.0f, bool showNegative = false);
};

class VisualHelper : public Axes {
   public:
    VisualHelper(const Object3d& obj3d, float lineSize = 1.0f) : Axes(obj3d.getData().model, lineSize) {}
};

#endif  // !AXES_H