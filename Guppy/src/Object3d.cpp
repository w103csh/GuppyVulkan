
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

    transform(tm);
}

BoundingBoxMinMax Object3d::getBoundingBoxMinMax(bool transform) const {
    BoundingBoxMinMax bbmm = {FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX};
    for (auto v : boundingBox_) {
        if (transform) v = model_ * glm::vec4(v, 1.0f);
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

bool Object3d::testBoundingBox(const Ray &ray) const {
    float a;
    auto bbmm = getBoundingBoxMinMax(true);

    float t_xMin, t_xMax;
    a = 1.0f / ray.d.x;
    if (a >= 0.0f) {
        t_xMin = a * (bbmm.xMin - ray.e.x);
        t_xMax = a * (bbmm.xMax - ray.e.x);
    } else {
        t_xMin = a * (bbmm.xMax - ray.e.x);
        t_xMax = a * (bbmm.xMin - ray.e.x);
    }

    float t_yMin, t_yMax;
    a = 1.0f / ray.d.y;
    if (a >= 0.0f) {
        t_yMin = a * (bbmm.yMin - ray.e.y);
        t_yMax = a * (bbmm.yMax - ray.e.y);
    } else {
        t_yMin = a * (bbmm.yMax - ray.e.y);
        t_yMax = a * (bbmm.yMin - ray.e.y);
    }

    float t_zMin, t_zMax;
    a = 1.0f / ray.d.z;
    if (a >= 0.0f) {
        t_zMin = a * (bbmm.zMin - ray.e.z);
        t_zMax = a * (bbmm.zMax - ray.e.z);
    } else {
        t_zMin = a * (bbmm.zMax - ray.e.z);
        t_zMax = a * (bbmm.zMin - ray.e.z);
    }

    if (t_xMin > t_yMax ||  // x test
        t_xMin > t_zMax ||  // x test
        t_yMin > t_xMax ||  // y test
        t_yMin > t_zMax ||  // y test
        t_zMin > t_xMax ||  // z test
        t_zMin > t_yMax)    // z test
    {
        return false;
    }
    return true;
}
