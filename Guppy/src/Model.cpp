//
//#define TINYOBJLOADER_IMPLEMENTATION
//#include <tiny_obj_loader.h>
//
//#include <algorithm>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <limits>
//#include <unordered_map>
//
//#include "Helpers.h"
//#include "Model.h"
//
//void Model::addToModel(const std::vector<Vertex>& vertices, const std::vector<VB_INDEX_TYPE>& indices) {
//    // TODO: add data validation
//
//    // vertices
//
//    vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
//
//    // indices
//
//    VB_INDEX_TYPE maxIndex = 0;
//    for (auto index : indices) {
//        indices_.push_back(index + (max_index_ + 1));
//        if (index > maxIndex) maxIndex = index;
//    }
//    max_index_ += (maxIndex + 1);
//}
//
//void Model::addToModel(Plane&& p) { addToModel(p.getVertices(), p.getIndices()); }
//
//void Model::loadAxes() {
//    float max_ = 2000.f;
//    float min_ = max_ * -1;
//
//    std::vector<Vertex> axes = {
//        {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // -X (WHITE)
//        {{min_, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // -X
//        {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // X (RED)
//        {{max_, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // X
//        {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // -Y (WHITE)
//        {{0.0f, min_, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // -Y
//        {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Y (GREEN)
//        {{0.0f, max_, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Y
//        {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // -Z (WHITE)
//        {{0.0f, 0.0f, min_}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // -Z
//        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // Z (BLUE)
//        {{0.0f, 0.0f, max_}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // Z
//
//        // TEST LINES
//        //{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // yellow through all positive
//        //{{max_, max_, max_}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // yellow
//    };
//
//    // Obviously this is a bit wonky atm
//    m_linesCount += axes.size();
//    lines_offset_ += sizeof(Vertex) * m_linesCount;
//
//    vertices_.insert(vertices_.end(), axes.begin(), axes.end());
//}
//
//void Model::loadDefault() {
//    addToModel(Plane(2.0f, 2.0f, true));
//    //addToModel(Plane(2.0f, 2.0f,                      // size
//    //                 false,                           // double-sided
//    //                 glm::vec3(0.0f, 0.0f, 0.0f),     // position
//    //                 glm::rotate(                     // rotation
//    //                     glm::mat4(1.0f),             // identity
//    //                     glm::radians(-90.0f),        // angle
//    //                     glm::vec3(1.0f, 0.0f, 0.0f)  // axis
//    //                     )));
//     //addToModel(Plane(2.0f, 2.0f, true, glm::vec3(0.0f, 0.0f, 0.5f),
//     //                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))));
//     //addToModel(Plane(2.0f, 2.0f, true, glm::vec3(0.0f, 0.0f, -0.5f),
//     //                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f))));
//     //addToModel(Plane());
//     //addToModel(Plane(2.0f, 2.0f, false, glm::vec3(0.0f, 0.5f, 0.0f)));
//}
//
//void Model::loadChalet() {
//    tinyobj::attrib_t attrib;
//    std::vector<tinyobj::shape_t> shapes;
//    std::vector<tinyobj::material_t> materials;
//    std::string err;
//
//    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, CHALET_MODEL_PATH.c_str())) {
//        throw std::runtime_error(err);
//    }
//
//    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
//
//    for (const auto& shape : shapes) {
//        for (const auto& index : shape.mesh.indices) {
//            Vertex vertex = {};
//
//            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
//                          attrib.vertices[3 * index.vertex_index + 2]};
//
//            vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
//                               1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
//
//            vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
//
//            if (uniqueVertices.count(vertex) == 0) {
//                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
//
//                vertices_.push_back(vertex);
//            }
//
//            indices_.push_back(uniqueVertices[vertex]);
//        }
//    }
//}