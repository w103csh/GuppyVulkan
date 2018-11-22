
#ifndef FACE_H
#define FACE_H

#include <array>

#include "Vertex.h"

class Face {
   public:
    Vertex::Complete &operator[](uint8_t index) {
        assert(index >= 0 && index < 3);
        return vertices_[index];
    }

    void calcNormal();
    void calcImageSpaceData();

   private:
    std::array<Vertex::Complete, 3> vertices_;
};

#endif  // !FACE_H