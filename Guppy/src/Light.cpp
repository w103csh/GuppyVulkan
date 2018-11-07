
#include "Light.h"

void Light::Positional::getData(Positional::Data& data) {
    data.position = glm::vec4(getPosition(), 1.0f);
    data.La = La_;
    data.flags = flags_;
    data.L = L_;
};
