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
//void Model::addToModel(const std::vector<Vertex>& vertices, const std::vector<VB_INDEX_TYPE>& indices)
//{
//    // TODO: add data validation
//
//    // vertices
//
//    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
//
//    // indices
//
//    VB_INDEX_TYPE maxIndex = 0;
//    for (auto index : indices)
//    {
//        m_indices.push_back(index + (m_maxIndex+1));
//        if (index > maxIndex)
//            maxIndex = index;
//    }
//    m_maxIndex += (maxIndex+1);
//}
//
//void Model::addToModel(Plane&& p)
//{
//    addToModel(p.getVertices(), p.getIndices());
//}
//
//void Model::loadAxes()
//{
//    auto max = std::numeric_limits<float>::max();
//    auto min = std::numeric_limits<float>::min();
//    m_vertices = {
//        {{ max, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }}, // X (GREEN)
//        {{ min, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
//        {{ 0.0f, max, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }}, // Y (BLUE)
//        {{ 0.0f, min, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
//        {{ 0.0f, 0.0f, max }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }}, // Z (RED)
//        {{ 0.0f, 0.0f, min }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
//    };
//
//    // Obviously this is a bit wonky atm
//    m_linesOffset = sizeof(Vertex) * m_vertices.size();
//}
//
//void Model::loadDefault()
//{
//   addToModel(Plane(
//        2.0f, 2.0f, true,
//        glm::vec3(0.0f, 0.0f, 0.5f),
//        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
//    ));
//    addToModel(Plane(
//        2.0f, 2.0f, true,
//        glm::vec3(0.0f, 0.0f, -0.5f),
//        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
//    ));
//    addToModel(Plane());
//    addToModel(Plane(
//        2.0f, 2.0f, false,
//        glm::vec3(0.0f, 0.5f, 0.0f)
//    ));
//}
//
//void Model::loadChalet()
//{
//    tinyobj::attrib_t attrib;
//    std::vector<tinyobj::shape_t> shapes;
//    std::vector<tinyobj::material_t> materials;
//    std::string err;
//
//    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, CHALET_MODEL_PATH.c_str()))
//    {
//        throw std::runtime_error(err);
//    }
//
//    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
//
//    for (const auto& shape : shapes)
//    {
//        for (const auto& index : shape.mesh.indices)
//        {
//            Vertex vertex = {};
//
//            vertex.pos = {
//                attrib.vertices[3 * index.vertex_index + 0],
//                attrib.vertices[3 * index.vertex_index + 1],
//                attrib.vertices[3 * index.vertex_index + 2]
//            };
//
//            vertex.texCoord = {
//                attrib.texcoords[2 * index.texcoord_index + 0],
//                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
//            };
//
//            vertex.color = { 1.0f, 1.0f, 1.0f };
//
//            if (uniqueVertices.count(vertex) == 0)
//            {
//                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
//
//                m_vertices.push_back(vertex);
//            }
//
//            m_indices.push_back(uniqueVertices[vertex]);
//        }
//    }
//}