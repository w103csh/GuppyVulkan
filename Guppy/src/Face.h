
#ifndef FACE_H
#define FACE_H

#include <array>
#include <algorithm>

#include "Vertex.h"

class Face {
   public:
    Vertex::Complete &operator[](uint8_t index) {
        assert(index >= 0 && index < 3);
        return vertices_[index];
    }

    inline void reverse() { std::reverse(std::begin(vertices_), std::end(vertices_)); }
    inline void setSmoothingGroup(uint32_t id) {
        for (auto &v : vertices_) v.smoothingGroupId = id;
    }

    void calcNormal();
    void calcTangentSpaceVectors();

   private:
    std::array<Vertex::Complete, 3> vertices_;
};

#endif  // !FACE_H