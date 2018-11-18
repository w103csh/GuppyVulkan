
#include <glm/gtc/matrix_access.hpp>

#include "Light.h"

//glm::vec3 Light::Spot::getDirection() const {
//    glm::vec3 dir1 = glm::row(model_, 2);
//    auto normie = glm::normalize(dir1);
//    glm::vec3 dir = glm::row(model_, 2) * -1.0f;
//    return glm::normalize(dir);
//}
//
//glm::vec3 Light::Spot::getPosition() const {
//    auto x = -model_ * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//    auto normie = model_[3];
//    // I believe this is the "w" homogenous factor from the book.
//    // Still not 100% sure.
//    auto w = model_[3];
//    return -w * model_;
//}

// virtual inline glm::vec3 getPosition() const { return model_[3]; }
// virtual inline glm::vec3 getDirection() const {
//    glm::vec3 dir = glm::row(model_, 2);
//    return glm::normalize(dir);
//}