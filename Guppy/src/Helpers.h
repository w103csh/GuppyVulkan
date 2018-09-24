//#pragma once
//
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/hash.hpp>
//
//#include "Vertex.h"
//
//namespace std
//{
//    // Hash function for Vertex class
//    template<> struct hash<Vertex>
//    {
//        size_t operator()(Vertex const& vertex) const
//        {
//            return ((hash<glm::vec3>()(vertex.pos) ^
//                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
//                     (hash<glm::vec2>()(vertex.texCoord) << 1);
//        }
//    };
//}