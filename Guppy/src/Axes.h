#ifndef AXES_H
#define AXES_H

#include <glm\glm.hpp>

#include "Helpers.h"
#include "Mesh.h"

const float AXES_MAX_SIZE = 2000.0f;

class Axes : public LineMesh {
   public:
    Axes(MeshCreateInfo* pCreateInfo, float lineSize = 1.0f, bool showNegative = false);
};

class VisualHelper : public Axes {
   public:
    VisualHelper(MeshCreateInfo* pCreateInfo, float lineSize = 1.0f) : Axes(pCreateInfo, lineSize) {}
};

#endif  // !AXES_H