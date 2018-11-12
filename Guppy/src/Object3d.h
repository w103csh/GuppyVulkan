#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <limits>
#include <vector>

#include "Vertex.h"

struct Object3d {
   public:
    Object3d(glm::mat4 model = glm::mat4(1.0f)) : obj3d_{model} {};

    struct BoundingBox {
        float xMin = FLT_MAX, xMax = -FLT_MAX;
        float yMin = FLT_MAX, yMax = -FLT_MAX;
        float zMin = FLT_MAX, zMax = -FLT_MAX;
    };

    struct Data {
        glm::mat4 model = glm::mat4(1.0f);
    };

    inline Data getData() const { return obj3d_; }

    virtual inline glm::vec3 getPosition() const { return obj3d_.model[3]; }
    virtual inline glm::vec3 getDirection() const {
        glm::vec3 dir = glm::row(obj3d_.model, 2);
        return glm::normalize(dir);
    }
    virtual inline void transform(const glm::mat4 t) { obj3d_.model *= t; }

    BoundingBox getBoundingBox() const;
    void putOnTop(const BoundingBox& boundingBox);

   protected:
    template <typename T>
    inline void updateBoundingBox(const std::vector<T>& vs) {
        for (auto& v : vs) updateBoundingBox(v);
    }
    inline void updateBoundingBox(Vertex::Base v) {
        if (v.pos.x < boundingBox_.xMin) boundingBox_.xMin = v.pos.x;
        if (v.pos.x > boundingBox_.xMax) boundingBox_.xMax = v.pos.x;
        if (v.pos.y < boundingBox_.yMin) boundingBox_.yMin = v.pos.y;
        if (v.pos.y > boundingBox_.yMax) boundingBox_.yMax = v.pos.y;
        if (v.pos.z < boundingBox_.zMin) boundingBox_.zMin = v.pos.z;
        if (v.pos.z > boundingBox_.zMax) boundingBox_.zMax = v.pos.z;
    }

    Data obj3d_;

   private:
    BoundingBox boundingBox_;  // this needs to be transformed so it should be private
};

#endif  // !OBJECT3D_H