#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/glm.hpp>

namespace Geometry {

struct CreateInfo {
    bool doubleSided = false;
    bool invert = false;
    glm::mat4 transform{1.0f};
};

}  // namespace Geometry

#endif  //! GEOMETRY_H
