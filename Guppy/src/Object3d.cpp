
#include "Object3d.h"

void Object3d::putOnTop(const BoundingBoxMinMax& inBoundingBoxMinMax) {
    auto myBoundingBoxMinMax = getBoundingBoxMinMax();

    // TODO: account for UP_VECTOR

    // in
    float inXCtr, inYCtr, inZCtr;
    inXCtr = inBoundingBoxMinMax.xMin + std::abs((inBoundingBoxMinMax.xMax - inBoundingBoxMinMax.xMin) / 2);
    inYCtr = inBoundingBoxMinMax.yMin + std::abs((inBoundingBoxMinMax.yMax - inBoundingBoxMinMax.yMin) / 2);
    inZCtr = inBoundingBoxMinMax.zMax;  // top

    // my
    float myXCtr, myYCtr, myZCtr;
    myXCtr = myBoundingBoxMinMax.xMin + std::abs((myBoundingBoxMinMax.xMax - myBoundingBoxMinMax.xMin) / 2);
    myYCtr = myBoundingBoxMinMax.yMin + std::abs((myBoundingBoxMinMax.yMax - myBoundingBoxMinMax.yMin) / 2);
    myZCtr = myBoundingBoxMinMax.zMin;  // bottom

    auto tm = glm::translate(glm::mat4(1.0f),       //
                             {(inXCtr - myXCtr),    //
                              (inYCtr - myYCtr),    //
                              (inZCtr - myZCtr)});  //

    obj3d_.model = tm * obj3d_.model;
}

BoundingBoxMinMax Object3d::getBoundingBoxMinMax() const {
    BoundingBoxMinMax bbmm = {FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX};
    for (auto v : boundingBox_) {
        v = obj3d_.model * glm::vec4(v, 1.0f);
        if (v.x < bbmm.xMin) bbmm.xMin = v.x;  // xMin
        if (v.x > bbmm.xMax) bbmm.xMax = v.x;  // xMax
        if (v.y < bbmm.yMin) bbmm.yMin = v.y;  // yMin
        if (v.y > bbmm.yMax) bbmm.yMax = v.y;  // yMax
        if (v.z < bbmm.zMin) bbmm.zMin = v.z;  // zMin
        if (v.z > bbmm.zMax) bbmm.zMax = v.z;  // zMax
    }
    return bbmm;
}

BoundingBox Object3d::getBoundingBox() const {
    BoundingBox bb = {};
    for (size_t i = 0; i < boundingBox_.size(); i++) {
        bb[i] = obj3d_.model * glm::vec4(boundingBox_[i], 1.0f);
    }
    return bb;
}
