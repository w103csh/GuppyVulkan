
#include "FaceMesh.h"

#include "Face.h"

FaceMesh::FaceMesh(MeshCreateInfo* pCreateInfo) : LineMesh("FaceMesh", pCreateInfo) {
    vertices_.resize(Face::NUM_VERTICES * 2, {});
    status_ = STATUS::PENDING_BUFFERS;
}
