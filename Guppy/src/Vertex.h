
#ifndef VERTEX_H
#define VERTEX_H

#include <array>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"

namespace Vertex {

const uint32_t BINDING = 0;

struct Color {
    static void getBindingDescriptions(std::vector<VkVertexInputBindingDescription> &descs);
    static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> &descs);
    glm::vec3 position;
    glm::vec3 normal{0.0f};
    glm::vec4 color{1.0f};
};

struct Texture {
    static void getBindingDescriptions(std::vector<VkVertexInputBindingDescription> &descs);
    static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> &descs);
    static void getScreenQuadAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> &descs);
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

/*  This is used for .obj loading, and the Face class. At some point I might just use
    this instead of breaking the data up because its a pain.
*/
class Complete {
   public:
    // default
    Complete() : position(), normal(), smoothingGroupId(), color(), texCoord(), tangent(), binormal(){};
    // color
    Complete(const Color &v)
        : position(v.position), normal(v.normal), smoothingGroupId(), color(v.color), texCoord(), tangent(), binormal(){};
    // texture
    Complete(const Texture &v)
        : position(v.position),
          normal(v.normal),
          smoothingGroupId(),
          color(),
          texCoord(v.texCoord),
          tangent(v.tangent),
          binormal(v.bitangent){};
    // complete
    Complete(const glm::vec3 &p, const glm::vec3 &n, const uint32_t &sgi, const glm::vec4 &c, const glm::vec2 &tc,
             const glm::vec3 &t, const glm::vec3 &b)
        : position(p), normal(n), smoothingGroupId(sgi), color(c), texCoord(tc), tangent(t), binormal(b){};

    struct hash_vertex_complete_smoothing {
        bool operator()(const Vertex::Complete &a, const Vertex::Complete &b) const {
            return                                                                   //
                glm::all(glm::epsilonEqual(a.position, b.position, FLT_EPSILON)) &&  //
                a.smoothingGroupId == b.smoothingGroupId;
        }
        size_t operator()(const Vertex::Complete &vertex) const {
            return ((std::hash<glm::vec3>()(vertex.position) ^ (std::hash<int>()(vertex.smoothingGroupId) << 1)) >> 1);
        }
    };

    struct hash_vertex_complete_non_smoothing {
        bool operator()(const Vertex::Complete &a, const Vertex::Complete &b) const {
            return                                                                   //
                glm::all(glm::epsilonEqual(a.position, b.position, FLT_EPSILON)) &&  //
                glm::all(glm::epsilonEqual(a.normal, b.normal, FLT_EPSILON));
        }
        size_t operator()(const Vertex::Complete &vertex) const {
            return ((std::hash<glm::vec3>()(vertex.position) ^ (std::hash<glm::vec3>()(vertex.normal) << 1)) >> 1);
        }
    };

    // bool operator==(const Complete &other) const {
    //    return                                                                  //
    //        glm::all(glm::epsilonEqual(position, other.position, FLT_EPSILON))  //
    //        && other.smoothingGroupId == smoothingGroupId;
    //}

    inline bool compareTexCoords(const Complete &other) const {
        return glm::all(glm::epsilonEqual(texCoord, other.texCoord, FLT_EPSILON));
    }

    inline Color getColorVertex() const { return {position, normal, color}; }
    inline Texture getTextureVertex() const { return {position, normal, texCoord, tangent, binormal}; }

    inline void transform(const glm::mat4 &t) { position = t * glm::vec4{position, 1.0f}; }

    // shared
    glm::vec3 position;
    glm::vec3 normal;
    uint32_t smoothingGroupId;
    // color
    glm::vec4 color;
    // texture
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 binormal;
};

}  // namespace Vertex

// **********************
// Hash functions
// **********************

// namespace std {
// // Hash function for Complete
// template <>
// struct hash<Vertex::Complete> {
//     size_t operator()(const Vertex::Complete &vertex) const {
//         // TODO: wtf is this doing?
//         return ((hash<glm::vec3>()(vertex.position) ^ (hash<int>()(vertex.smoothingGroupId) << 1)) >> 1);
//     }
// };
// }  // namespace std

#endif  // !VERTEX_H