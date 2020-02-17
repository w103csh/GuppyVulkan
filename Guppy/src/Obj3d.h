/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OBJ_3D_H
#define OBJ_3D_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <vector>

#include "Constants.h"
#include "Vertex.h"

namespace Obj3d {

struct BoundingBoxMinMax {
    float xMin, xMax;
    float yMin, yMax;
    float zMin, zMax;
};

using BoundingBox = std::array<glm::vec3, 6>;

const BoundingBox DEFAULT_BOUNDING_BOX = {
    glm::vec3{FLT_MAX, 0.0f, 0.0f},   // xMin
    glm::vec3{-FLT_MAX, 0.0f, 0.0f},  // xMax
    glm::vec3{0.0f, FLT_MAX, 0.0f},   // yMin
    glm::vec3{0.0f, -FLT_MAX, 0.0f},  // yMax
    glm::vec3{0.0f, 0.0f, FLT_MAX},   // zMin
    glm::vec3{0.0f, 0.0f, -FLT_MAX}   // zMax
};

class Interface {
   public:
    virtual glm::vec3 worldToLocal(const glm::vec3& v, const bool isPosition = false, const uint32_t index = 0) const = 0;
    virtual glm::vec3 getWorldSpaceDirection(const glm::vec3& d = FORWARD_VECTOR, const uint32_t index = 0) const = 0;
    virtual glm::vec3 getWorldSpacePosition(const glm::vec3& p = {}, const uint32_t index = 0) const = 0;
    virtual BoundingBoxMinMax getBoundingBoxMinMax(const bool transform = true, const uint32_t index = 0) const = 0;
    virtual bool testBoundingBox(const Ray& ray, const float& tMin, const bool useDirection = true,
                                 const uint32_t index = 0) const = 0;
    virtual BoundingBox getBoundingBox(const uint32_t index = 0) const = 0;
    // Model space to world space
    virtual const glm::mat4& model(const uint32_t index = 0) const = 0;

    virtual void transform(const glm::mat4 t, const uint32_t index = 0) = 0;
    virtual void putOnTop(const BoundingBoxMinMax& boundingBox, const uint32_t index = 0) = 0;
};

glm::mat4 TranslateToTop(const BoundingBoxMinMax& bottomBBMM, const BoundingBoxMinMax& topBBMM);

// ABSTRACT BASE

class AbstractBase : public Interface {
   public:
    virtual inline glm::vec3 worldToLocal(const glm::vec3& v, const bool isPosition = false,
                                          const uint32_t index = 0) const override {
        glm::vec3 local = glm::inverse(model(index)) * glm::vec4(v, isPosition ? 1.0f : 0.0f);
        return isPosition ? local : glm::normalize(local);
    }

    virtual inline glm::vec3 getWorldSpaceDirection(const glm::vec3& d = FORWARD_VECTOR,
                                                    const uint32_t index = 0) const override {
        glm::vec3 direction = model(index) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    virtual inline glm::vec3 getWorldSpacePosition(const glm::vec3& p = {}, const uint32_t index = 0) const override {
        return model(index) * glm::vec4(p, 1.0f);  //
    }

    // I did the order this way so that the incoming tranform doesn't get scaled...
    // This could be problematic. Not sure yet.
    virtual inline void transform(const glm::mat4 t, const uint32_t index = 0) override {
        auto& m = const_cast<glm::mat4&>(model(index));
        m = t * m;
    }

    BoundingBoxMinMax getBoundingBoxMinMax(const bool transform = true, const uint32_t index = 0) const override;

    virtual bool testBoundingBox(const Ray& ray, const float& tMin, const bool useDirection = true,
                                 const uint32_t index = 0) const override;
    virtual void putOnTop(const BoundingBoxMinMax& boundingBox, const uint32_t index = 0) override;

    BoundingBox getBoundingBox(const uint32_t index = 0) const override;

    template <typename T>
    void worldToLocal(T& vecs, const uint32_t index = 0) const {
        auto inverse = glm::inverse(model(index));
        for (auto& v : vecs) {
            v = inverse * v;
        }
    }

    template <class T>
    inline void updateBoundingBox(const std::vector<T>& vs) {
        for (auto& v : vs) updateBoundingBox(v);
    }

    template <class T>
    inline void updateBoundingBox(const T& v) {
        if (v.position.x < boundingBox_[0].x) boundingBox_[0] = v.position;  // xMin
        if (v.position.x > boundingBox_[1].x) boundingBox_[1] = v.position;  // xMax
        if (v.position.y < boundingBox_[2].y) boundingBox_[2] = v.position;  // yMin
        if (v.position.y > boundingBox_[3].y) boundingBox_[3] = v.position;  // yMax
        if (v.position.z < boundingBox_[4].z) boundingBox_[4] = v.position;  // zMin
        if (v.position.z > boundingBox_[5].z) boundingBox_[5] = v.position;  // zMax
    }

    inline void updateBoundingBox(const BoundingBoxMinMax& bbmm) {
        if (bbmm.xMin < boundingBox_[0].x) boundingBox_[0].x = bbmm.xMin;  // xMin
        if (bbmm.xMax > boundingBox_[1].x) boundingBox_[1].x = bbmm.xMax;  // xMax
        if (bbmm.yMin < boundingBox_[2].y) boundingBox_[2].y = bbmm.yMin;  // yMin
        if (bbmm.yMax > boundingBox_[3].y) boundingBox_[3].y = bbmm.yMax;  // yMax
        if (bbmm.zMin < boundingBox_[4].z) boundingBox_[4].z = bbmm.zMin;  // zMin
        if (bbmm.zMax > boundingBox_[5].z) boundingBox_[5].z = bbmm.zMax;  // zMax
    }

   protected:
    AbstractBase() : boundingBox_(DEFAULT_BOUNDING_BOX) {}

   private:
    /* This needs to be transformed so it should be private!
       TODO: If a vertex is changed then this needs to be updated using one
       of the updateBoundingBox functions... this is not great. ATM it would
       be safer to just always use the update that takes all the vertices.
    */
    BoundingBox boundingBox_;
};

}  // namespace Obj3d

#endif  // !OBJ_3D_H