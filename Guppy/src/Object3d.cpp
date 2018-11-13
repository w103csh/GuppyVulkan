
#include "Object3d.h"

void Object3d::putOnTop(const BoundingBox& inBoundingBox) {
    auto myBoundingBox = getBoundingBox();  // bounding box needs to be transformed

    // TODO: account for UP_VECTOR

    float inXCtr = 0.0f, inYCtr = 0.0f, inZCtr = 0.0f;
    float myXCtr = 0.0f, myYCtr = 0.0f, myZCtr = 0.0f;

    float xMin, xMax, yMin, yMax, zMin, zMax;

    for (int i = 0; i < 2; i++) {
        xMin = yMin = zMin = FLT_MAX;
        xMax = yMax = zMax = -FLT_MAX;

        auto& bb = i == 0 ? inBoundingBox : myBoundingBox;

        for (auto& v : bb) {
            if (v.x < xMin) xMin = v.x;  // xMin
            if (v.x > xMax) xMax = v.x;  // xMax
            if (v.y < yMin) yMin = v.y;  // yMin
            if (v.y > yMax) yMax = v.y;  // yMax
            if (v.z < zMin) zMin = v.z;  // zMin
            if (v.z > zMax) zMax = v.z;  // zMax
        }

        if (i == 0) {
            inXCtr = xMin + std::abs((xMax - xMin) / 2);
            inYCtr = yMin + std::abs((yMax - yMin) / 2);
            inZCtr = zMax;  // top
        } else if (i == 1) {
            myXCtr = xMin + std::abs((xMax - xMin) / 2);
            myYCtr = yMin + std::abs((yMax - yMin) / 2);
            myZCtr = zMin;  // bottom
        }
    }

    auto tm = glm::translate(glm::mat4(1.0f),       //
                             {(inXCtr - myXCtr),    //
                              (inYCtr - myYCtr),    //
                              (inZCtr - myZCtr)});  //

    obj3d_.model = tm * obj3d_.model;
}

BoundingBox Object3d::getBoundingBox() const {
    BoundingBox bb = {};
    for (size_t i = 0; i < boundingBox_.size(); i++) {
        bb[i] = obj3d_.model * boundingBox_[i];
    }
    return bb;
}
