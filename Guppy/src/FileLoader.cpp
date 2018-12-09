
#define TINYOBJLOADER_IMPLEMENTATION

#include <fstream>
#include <sstream>

#include "FileLoader.h"
#include "Shell.h"

std::string FileLoader::readFile(std::string filepath) {
    std::ifstream file(ROOT_PATH + filepath, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string str = ss.str();

    file.close();

    return str;
}

void FileLoader::getObjData(Shell *sh, tinyobj_data &data) {
    std::string warn, err;
    if (!tinyobj::LoadObj(&data.attrib, &data.shapes, &data.materials, &warn, &err, data.filename.c_str(),
                          data.mtl_basedir.c_str())) {
        sh->log(Shell::LOG_ERR, err.c_str());
        throw std::runtime_error(err);
    }
    if (!warn.empty()) {
        sh->log(Shell::LOG_WARN, warn.c_str());
    }
}

// for (size_t i = 0; i < 3; ++i) {
//    auto &vertex = face[i];
//    auto it = uniqueVertices.find(vertex);
//
//    // auto it = std::find_if(uniqueVertices2.begin(), uniqueVertices2.end(), [&vertex](auto &p) {
//    //    return p.first == vertex && glm::all(glm::epsilonEqual(p.first.texCoord, vertex.texCoord, FLT_EPSILON));
//    //});
//
//    // if (it != uniqueVertices2.end()) {
//    //    assert(false);
//    //} else {
//    uint32_t index = 0;  // TODO: this shouldn't be initialized
//
//    bool samePositionDifferentTexCoords = false;
//    // it = std::find_if(uniqueVertices1.begin(), uniqueVertices1.end(), [&vertex](auto &p) { return p.first == vertex;
//    // });
//
//    // if (it != uniqueVertices1.end()) {
//    if (it != uniqueVertices.end()) {
//        // Vertex with same position already in the buffer...
//        auto &sharedVertex = const_cast<Vertex::Complete &>((*it).first);
//
//        // Averge vertex attributes
//        vertex.normal = sharedVertex.normal = glm::normalize(sharedVertex.normal + vertex.normal);
//        vertex.tangent = sharedVertex.tangent += vertex.tangent;
//        vertex.bitangent = sharedVertex.bitangent += vertex.bitangent;
//
//        // If the vertex shares a position, but has a different texture coordinate we still
//        // need to smooth the tangent space vectors, but need a new index.
//        samePositionDifferentTexCoords = !glm::all(glm::epsilonEqual(vertex.texCoord, sharedVertex.texCoord, FLT_EPSILON));
//
//        // Update vertex in mesh
//        pMesh->addVertex(vertex, (*it).second);
//    }
//
//    if (it == uniqueVertices.end() || samePositionDifferentTexCoords) {
//        if (samePositionDifferentTexCoords) {
//            assert(false);
//        }
//        // New unique vertex...
//        index = static_cast<uint32_t>(pMesh->getVertexCount());
//        uniqueVertices.insert(std::make_pair(vertex, index));
//        pMesh->addVertex(std::move(vertex));
//    }
//
//    // Add an index to the buffer
//    pMesh->addIndex(index);
//    //}
//}