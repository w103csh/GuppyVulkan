
#ifndef VERTEX_H
#define VERTEX_H

#include <array>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace Vertex {

enum class TYPE { COLOR = 0, TEXTURE };

struct Color {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
};

struct Texture {
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
    Complete() : position(), normal(), smoothingGroupId(), color(), texCoord(), tangent(), bitangent(){};
    // color
    Complete(const Color &v)
        : position(v.position), normal(v.normal), smoothingGroupId(), color(v.color), texCoord(), tangent(), bitangent(){};
    // texture
    Complete(const Texture &v)
        : position(v.position),
          normal(v.normal),
          smoothingGroupId(),
          color(),
          texCoord(v.texCoord),
          tangent(v.tangent),
          bitangent(v.bitangent){};
    // complete
    Complete(const glm::vec3 &p, const glm::vec3 &n, const uint32_t &sgi, const glm::vec4 &c, const glm::vec2 &tc,
             const glm::vec3 &t, const glm::vec3 &b)
        : position(p), normal(n), smoothingGroupId(sgi), color(c), texCoord(tc), tangent(t), bitangent(b){};

    bool operator==(const Complete &other) const {
        return //
            position == other.position
            // && texCoord == other.texCoord
            && other.smoothingGroupId == smoothingGroupId;
    }

    inline Color getColorData() const { return {position, normal, color}; }
    inline Texture getTextureData() const { return {position, normal, texCoord, tangent, bitangent}; }

    // shared
    glm::vec3 position;
    glm::vec3 normal;
    uint32_t smoothingGroupId;
    // color
    glm::vec4 color;
    // texture
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

// color
VkVertexInputBindingDescription getColorBindDesc();
std::vector<VkVertexInputAttributeDescription> getColorAttrDesc();
// texture
VkVertexInputBindingDescription getTexBindDesc();
std::vector<VkVertexInputAttributeDescription> getTexAttrDesc();

std::string getTypeName(TYPE type);

}  // namespace Vertex

// **********************
// Hash functions
// **********************

namespace std {
// Hash function for Complete
template <>
struct hash<Vertex::Complete> {
    size_t operator()(Vertex::Complete const &vertex) const {
        // TODO: wtf is this doing?
        return (hash<glm::vec3>()(vertex.position) ^ (hash<int>()(vertex.smoothingGroupId) << 1)) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std

#endif  // !VERTEX_H