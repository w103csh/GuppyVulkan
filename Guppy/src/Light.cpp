
#include "Light.h"

Light::Positional::Data Light::Positional::getData() {
    auto position = glm::vec4(getPosition(), 1.0f);
    return {position, La_, flags_, L_};
};
