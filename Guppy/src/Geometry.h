#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/glm.hpp>

namespace Geometry {

// Should this be in the Mesh namespace?
struct CreateInfo {
    bool doubleSided = false;
    bool reverseWinding = false;
    glm::mat4 transform{1.0f};
};

}  // namespace Geometry

#endif  //! GEOMETRY_H
