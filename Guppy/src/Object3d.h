#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <limits>
#include <vector>

#include "Vertex.h"

typedef std::array<glm::vec4, 6> BoundingBox;
const BoundingBox DefaultBoundingBox = {
    glm::vec4(FLT_MAX, 0.0f, 0.0f, 1.0f),   // xMin
    glm::vec4(-FLT_MAX, 0.0f, 0.0f, 1.0f),  // xMax
    glm::vec4(0.0f, FLT_MAX, 0.0f, 1.0f),   // yMin
    glm::vec4(0.0f, -FLT_MAX, 0.0f, 1.0f),  // yMax
    glm::vec4(0.0f, 0.0f, FLT_MAX, 1.0f),   // zMin
    glm::vec4(0.0f, 0.0f, -FLT_MAX, 1.0f)   // zMax
};

struct Object3d {
   public:
    Object3d(glm::mat4 model = glm::mat4(1.0f)) : obj3d_{model}, boundingBox_(DefaultBoundingBox){};

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
        if (v.pos.x < boundingBox_[0].x) boundingBox_[0].x = v.pos.x;  // xMin
        if (v.pos.x > boundingBox_[1].x) boundingBox_[1].x = v.pos.x;  // xMax
        if (v.pos.y < boundingBox_[2].y) boundingBox_[2].y = v.pos.y;  // yMin
        if (v.pos.y > boundingBox_[3].y) boundingBox_[3].y = v.pos.y;  // yMax
        if (v.pos.z < boundingBox_[4].z) boundingBox_[4].z = v.pos.z;  // zMin
        if (v.pos.z > boundingBox_[5].z) boundingBox_[5].z = v.pos.z;  // zMax
    }

    Data obj3d_;

   private:
    /* This needs to be transformed so it should be private!
       TODO: If a vertex is changed then this needs to be updated using one
       of the updateBoundingBox functions... this is not great. ATM it would
       be safer to just always use the update that takes all the vertices.
    */
    BoundingBox boundingBox_;
};

#endif  // !OBJECT3D_H