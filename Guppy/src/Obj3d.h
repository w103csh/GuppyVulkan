#ifndef OBJ_3D_H
#define OBJ_3D_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <vector>

#include "Helpers.h"
#include "Vertex.h"

const uint32_t MODEL_ALL = UINT32_MAX;
typedef std::array<glm::vec3, 6> BoundingBox;

const BoundingBox DEFAULT_BOUNDING_BOX = {
    glm::vec3(FLT_MAX, 0.0f, 0.0f),   // xMin
    glm::vec3(-FLT_MAX, 0.0f, 0.0f),  // xMax
    glm::vec3(0.0f, FLT_MAX, 0.0f),   // yMin
    glm::vec3(0.0f, -FLT_MAX, 0.0f),  // yMax
    glm::vec3(0.0f, 0.0f, FLT_MAX),   // zMin
    glm::vec3(0.0f, 0.0f, -FLT_MAX)   // zMax
};

struct BoundingBoxMinMax {
    float xMin, xMax;
    float yMin, yMax;
    float zMin, zMax;
};

static glm::mat4 translateToTop(const BoundingBoxMinMax& bottomBBMM, const BoundingBoxMinMax& topBBMM) {
    // TODO: account for UP_VECTOR

    // bottom
    float inXCtr, inYCtr, inZCtr;
    inXCtr = bottomBBMM.xMin + std::abs((bottomBBMM.xMax - bottomBBMM.xMin) / 2);
    inYCtr = bottomBBMM.yMax;  // top
    inZCtr = bottomBBMM.zMin + std::abs((bottomBBMM.zMax - bottomBBMM.zMin) / 2);

    // top
    float myXCtr, myYCtr, myZCtr;
    myXCtr = topBBMM.xMin + std::abs((topBBMM.xMax - topBBMM.xMin) / 2);
    myYCtr = topBBMM.yMin;  // bottom
    myZCtr = topBBMM.zMin + std::abs((topBBMM.zMax - topBBMM.zMin) / 2);

    return glm::translate(glm::mat4(1.0f),  //
                          {
                              //(inXCtr - myXCtr),  //
                              0.0f,
                              (inYCtr - myYCtr),
                              //(inZCtr - myZCtr)  //
                              0.0f,
                          });
}

class Obj3d {
   public:
    // TODO: find a way to ensure a model has been set during construction.
    Obj3d() : boundingBox_(DEFAULT_BOUNDING_BOX){};

    virtual uint32_t getModelCount() { return 1; }
    inline glm::mat4 getModel(uint32_t index = 0) const { return const_cast<glm::mat4&>(model(index)); }

    virtual inline glm::vec3 worldToLocal(const glm::vec3& v, bool isPosition = false, uint32_t index = 0) const {
        glm::vec3 local = glm::inverse(model(index)) * glm::vec4(v, isPosition ? 1.0f : 0.0f);
        return isPosition ? local : glm::normalize(local);
    }

    template <typename T>
    void worldToLocal(T& vecs, uint32_t index = 0) const {
        auto inverse = glm::inverse(model(index));
        for (auto& v : vecs) {
            v = inverse * v;
        }
    }

    virtual inline glm::vec3 getWorldSpaceDirection(const glm::vec3& d = FORWARD_VECTOR, uint32_t index = 0) const {
        glm::vec3 direction = model(index) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    virtual inline glm::vec3 getWorldSpacePosition(const glm::vec3& p = {}, uint32_t index = 0) const {  //
        return model(index) * glm::vec4(p, 1.0f);
    }

    // I did the order this way so that the incoming tranform doesn't get scaled...
    // This could be problematic. Not sure yet.
    virtual inline void transform(const glm::mat4 t, uint32_t index = 0) {
        auto& m = const_cast<glm::mat4&>(model(index));
        m = t * m;
    }

    BoundingBoxMinMax getBoundingBoxMinMax(bool transform = true, uint32_t index = 0) const;

    virtual bool testBoundingBox(const Ray& ray, const float& tMin, bool useDirection = true, uint32_t index = 0) const;

    virtual void putOnTop(const BoundingBoxMinMax& boundingBox, uint32_t index = 0);

   protected:
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

    BoundingBox getBoundingBox(uint32_t index = 0) const;

    // Model space to world space
    virtual const glm::mat4& model(uint32_t index = 0) const = 0;

   private:
    /* This needs to be transformed so it should be private!
       TODO: If a vertex is changed then this needs to be updated using one
       of the updateBoundingBox functions... this is not great. ATM it would
       be safer to just always use the update that takes all the vertices.
    */
    BoundingBox boundingBox_;
};

#endif  // !OBJ_3D_H