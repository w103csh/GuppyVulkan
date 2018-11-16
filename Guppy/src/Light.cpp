
#include <glm/gtc/matrix_access.hpp>

#include "Light.h"

glm::vec3 Light::Spot::getDirection() const {
    glm::vec3 dir = glm::row(obj3d_.model, 2) * -1.0f;
    return glm::normalize(dir);
}

glm::vec3 Light::Spot::getPosition() const {
    // I believe this is the "w" homogenous factor from the book.
    // Still not 100% sure.
    auto w = obj3d_.model[3];
    glm::vec3 pos = -w * obj3d_.model;
    return pos;
}
