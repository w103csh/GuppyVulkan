
#include "Object3d.h"

void Object3d::putOnTop(const BoundingBox& boundingBox) {
    float t_xCtr = boundingBox_.xMin + std::abs((boundingBox_.xMax - boundingBox_.xMin) / 2);
    float o_xCtr = boundingBox.xMin + std::abs((boundingBox.xMax - boundingBox.xMin) / 2);

    float t_yCtr = boundingBox_.yMin + std::abs((boundingBox_.yMax - boundingBox_.yMin) / 2);
    float o_yCtr = boundingBox.yMin + std::abs((boundingBox.yMax - boundingBox.yMin) / 2);

    glm::vec3 t(                              //
        o_xCtr - t_xCtr,                      //
        t_yCtr - o_yCtr,                      //
        boundingBox.zMax - boundingBox_.zMin  //
    );

    auto tm = glm::translate(glm::mat4(1.0f), t);
    obj3d_.model = glm::translate(glm::mat4(1.0f), t) * obj3d_.model;
}

Object3d::BoundingBox Object3d::getBoundingBox() const {
    BoundingBox bb = {0, 0, 0, 0, 0, 0};
    glm::vec4 c = {};

    c = glm::column(obj3d_.model, 0);
    bb.xMin += ((boundingBox_.xMin * c.x) + c.w);
    bb.xMax += ((boundingBox_.xMax * c.x) + c.w);

    c = glm::row(obj3d_.model, 1);
    bb.yMin += ((boundingBox_.yMin * c.y) + c.w);
    bb.yMax += ((boundingBox_.yMax * c.y) + c.w);

    c = glm::row(obj3d_.model, 2);
    bb.zMin += ((boundingBox_.zMin * c.z) + c.w);
    bb.zMax += ((boundingBox_.zMax * c.z) + c.w);

    return bb;
}
