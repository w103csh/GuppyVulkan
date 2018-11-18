
#include "Object3d.h"

void Object3d::putOnTop(const BoundingBoxMinMax& inBoundingBoxMinMax) {
    auto myBoundingBoxMinMax = getBoundingBoxMinMax();

    // TODO: account for UP_VECTOR

    // in
    float inXCtr, inYCtr, inZCtr;
    inXCtr = inBoundingBoxMinMax.xMin + std::abs((inBoundingBoxMinMax.xMax - inBoundingBoxMinMax.xMin) / 2);
    inYCtr = inBoundingBoxMinMax.yMax;  // top
    inZCtr = inBoundingBoxMinMax.zMin + std::abs((inBoundingBoxMinMax.zMax - inBoundingBoxMinMax.zMin) / 2);

    // my
    float myXCtr, myYCtr, myZCtr;
    myXCtr = myBoundingBoxMinMax.xMin + std::abs((myBoundingBoxMinMax.xMax - myBoundingBoxMinMax.xMin) / 2);
    myYCtr = myBoundingBoxMinMax.yMin;  // bottom
    myZCtr = myBoundingBoxMinMax.zMin + std::abs((myBoundingBoxMinMax.zMax - myBoundingBoxMinMax.zMin) / 2);

    auto tm = glm::translate(glm::mat4(1.0f),  //
                             {
                                 //(inXCtr - myXCtr),  //
                                 0.0f,
                                 (inYCtr - myYCtr),
                                 //(inZCtr - myZCtr)  //
                                 0.0f,
                             });

    model_ = tm * model_;
}

BoundingBoxMinMax Object3d::getBoundingBoxMinMax() const {
    BoundingBoxMinMax bbmm = {FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX};
    for (auto v : boundingBox_) {
        v = model_ * glm::vec4(v, 1.0f);
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
        bb[i] = model_ * glm::vec4(boundingBox_[i], 1.0f);
    }
    return bb;
}
