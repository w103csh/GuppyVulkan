#ifndef MESH_CONSTANTS_H
#define MESH_CONSTANTS_H

#include <vector>
#include <string>

#include "Mesh.h"

namespace Mesh {

struct GenericCreateInfo : public CreateInfo {
    GenericCreateInfo();
    std::vector<Face> faces;
    bool indexVertices;
    bool smoothNormals;
    std::string name;
};

}  // namespace Mesh

#endif  //! MESH_CONSTANTS_H