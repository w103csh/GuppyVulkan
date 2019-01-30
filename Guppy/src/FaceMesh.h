#ifndef FACE_MESH_H
#define FACE_MESH_H

#include "Mesh.h"

class FaceMesh : public LineMesh {
   public:
    FaceMesh(MeshCreateInfo *pCreateInfo);
};

#endif  //! FACE_MESH_H