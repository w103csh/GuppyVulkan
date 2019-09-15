
#include "MeshConstants.h"

#include "Face.h"

Mesh::GenericCreateInfo::GenericCreateInfo()
    : CreateInfo{PIPELINE::TRI_LIST_COLOR}, indexVertices(true), smoothNormals(true), name("Generic Color Mesh") {}
