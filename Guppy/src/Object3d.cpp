
#include "Object3d.h"

BoundingBoxMinMax Object3d::getBoundingBoxMinMax(bool transform, uint32_t index) const {
    BoundingBoxMinMax bbmm = {FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX};
    for (auto v : boundingBox_) {
        if (transform) v = model(index) * glm::vec4(v, 1.0f);
        if (v.x < bbmm.xMin) bbmm.xMin = v.x;  // xMin
        if (v.x > bbmm.xMax) bbmm.xMax = v.x;  // xMax
        if (v.y < bbmm.yMin) bbmm.yMin = v.y;  // yMin
        if (v.y > bbmm.yMax) bbmm.yMax = v.y;  // yMax
        if (v.z < bbmm.zMin) bbmm.zMin = v.z;  // zMin
        if (v.z > bbmm.zMax) bbmm.zMax = v.z;  // zMax
    }
    return bbmm;
}

bool Object3d::testBoundingBox(const Ray &ray, const float &tMin, bool useDirection, uint32_t index) const {
    // Numbers are ransformed into world space
    auto bbmm = getBoundingBoxMinMax(true, index);

    /* TODO: this is potential a step that could be done outside of this function to save time.

       TODO: test "useDirection" to see if it is functioning correctly. Also, maybe a separate function would be smart to
       save 3 steps.

       Also, in order to get accurate values for "t", this and eany other functions used to determine "t" during the
       intersection process should use be relevant to the initial values of "e", and "d" of the ray.
    */
    // This gives the proper signs for testing in right direction.
    const auto &d = useDirection ? ray.direction : ray.d;

    float a, t_min,      //
        t_min1, t_max1,  //
        t_min2, t_max2;

    // X interval
    a = 1.0f / d.x;
    if (a >= 0.0f) {
        t_min1 = a * (bbmm.xMin - ray.e.x);
        t_max1 = a * (bbmm.xMax - ray.e.x);
    } else {
        t_min1 = a * (bbmm.xMax - ray.e.x);
        t_max1 = a * (bbmm.xMin - ray.e.x);
    }

    // Y interval
    a = 1.0f / d.y;
    if (a >= 0.0f) {
        t_min2 = a * (bbmm.yMin - ray.e.y);
        t_max2 = a * (bbmm.yMax - ray.e.y);
    } else {
        t_min2 = a * (bbmm.yMax - ray.e.y);
        t_max2 = a * (bbmm.yMin - ray.e.y);
    }

    // Test for an intersection of X & Y intervals
    if (t_min1 > tMin || t_min1 > t_max2 || t_min2 > t_max1) return false;

    // Update "t_min1" & "t_max2" to have the intersection of X & Y
    t_min1 = (std::max)(t_min1, t_min2);
    t_max1 = (std::min)(t_max1, t_max2);

    a = 1.0f / d.z;
    if (a >= 0.0f) {
        t_min2 = a * (bbmm.zMin - ray.e.z);
        t_max2 = a * (bbmm.zMax - ray.e.z);
    } else {
        t_min2 = a * (bbmm.zMax - ray.e.z);
        t_max2 = a * (bbmm.zMin - ray.e.z);
    }

    // Test for an intersection of X & Y & Z intervals
    if (t_min1 > tMin || t_min1 > t_max2 || t_min2 > t_max1) return false;

    // Update "t_min1" & "t_max2" to have the intersection of X & Y & Z
    t_min1 = (std::max)(t_min1, t_min2);
    t_max1 = (std::min)(t_max1, t_max2);

    if (useDirection) {
        // Test against incoming "tMin" to see if entire bounding box is behind
        // the previous closest intersection.
        t_min = (std::min)(t_min1, t_max1);
        if (t_min > tMin || t_min < 0) return false;
    }

    return true;
}

void Object3d::putOnTop(const BoundingBoxMinMax &inBoundingBoxMinMax, uint32_t index) {
    auto myBoundingBoxMinMax = getBoundingBoxMinMax(true, index);
    auto t = translateToTop(inBoundingBoxMinMax, myBoundingBoxMinMax);
    transform(t, index);
}

BoundingBox Object3d::getBoundingBox(uint32_t index) const {
    BoundingBox bb = {};
    for (size_t i = 0; i < boundingBox_.size(); i++) {
        bb[i] = model(index) * glm::vec4(boundingBox_[i], 1.0f);
    }
    return bb;
}