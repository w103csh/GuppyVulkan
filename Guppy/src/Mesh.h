
#ifndef MESH_H
#define MESH_H

#include <future>
#include <glm/glm.hpp>
#include <set>
#include <vector>

#include "ConstantsAll.h"
#include "Geometry.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Instance.h"
#include "Material.h"
#include "ObjDrawInst3d.h"
#include "Shell.h"
#include "Texture.h"

// clang-format off
class Face;
namespace Loading       { struct Resource; }
namespace Scene         { class Handler; }
// clang-format on

namespace Mesh {

typedef uint32_t INDEX;
constexpr Mesh::INDEX BAD_OFFSET = UINT32_MAX;

class Handler;

using CreateInfo = struct {
    Geometry::CreateInfo geometryCreateInfo = {};
    bool isIndexed = true;  // This is dumb
    bool mappable = false;
    PIPELINE pipelineType = PIPELINE::ALL_ENUM;
    std::set<PASS> passTypes = Uniform::PASS_ALL_SET;
    bool selectable = true;
};

// **********************
//      Base
// **********************

class Base : public NonCopyable, public Handlee<Mesh::Handler>, public ObjDrawInst3d {
    friend class Mesh::Handler;
    friend class Descriptor::Handler;  // Reference (TODO: get rid of this)

   public:
    typedef enum FLAG {
        POLY = 0x00000001,
        LINE = 0x00000002,
        // THROUGH 0x00000008
    } FLAG;

    const FlagBits FLAGS;
    const bool MAPPABLE;
    const std::string NAME;
    const std::set<PASS> PASS_TYPES;
    const PIPELINE PIPELINE_TYPE;
    const MESH TYPE;
    const VERTEX VERTEX_TYPE;

    inline Mesh::INDEX getOffset() const { return offset_; }
    inline FlagBits getStatus() const { return status_; }

    // MATERIAL
    inline auto& getMaterial() const { return pMaterial_; }
    inline bool hasNormalMap() const {
        return pMaterial_->hasTexture() && pMaterial_->getTexture()->flags & ::Sampler::USAGE::NORMAL;
    }

    inline void setStatus(const STATUS&& status) {
        switch (status) {
            case STATUS::PENDING_BUFFERS:
                assert(status_ == STATUS::PENDING_VERTICES);
                status_ = STATUS::PENDING_BUFFERS;
                break;
            default:
                assert(false && "Unhandled case for mesh status");
                break;
        }
    }

    // LOADING
    void prepare();

    // VERTEX
    virtual Vertex::Complete getVertexComplete(size_t index) const = 0;
    virtual void addVertex(const Vertex::Complete& v, int32_t index = -1) = 0;
    virtual void addVertex(const Face& face);
    virtual inline uint32_t getVertexCount() const = 0;  // TODO: this shouldn't be public
    virtual const glm::vec3& getVertexPositionAtOffset(size_t offset) const = 0;
    void updateBuffers();
    inline VkBuffer& getVertexBuffer() { return vertexRes_.buffer; }

    // INDEX
    uint32_t getFaceCount() const;
    Face getFace(size_t faceIndex);
    inline void addIndices(std::vector<VB_INDEX_TYPE>& is) {
        for (auto i : is) indices_.push_back(i);
    }
    inline void addIndex(VB_INDEX_TYPE index) { indices_.push_back(index); }

    // FACE
    inline bool isSelectable() { return selectable_; }
    void selectFace(const Ray& ray, float& tMin, Face& face, size_t offset) const;
    void updateTangentSpaceData();

    // DRAWING
    bool shouldDraw(const PASS& passTypeComp, const PIPELINE& pipelineType) const;
    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pipelineBindData, const VkCommandBuffer& cmd,
              const uint8_t frameIndex) const;

    virtual void destroy();

   protected:
    Base(Mesh::Handler& handler, const MESH&& type, const VERTEX&& vertexType, const FLAG&& flags, const std::string&& name,
         CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
         std::shared_ptr<Material::Base>& pMaterial);
    Base() = delete;
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTRs OF THIS CLASS.
        IF THIS IS REMOVED THE DESTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Base();

    // VERTEX
    void loadBuffers();
    virtual inline const void* getVertexData() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize(bool assert = false) const = 0;

    // INDEX
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexCount() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize(bool assert = false) const {
        VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();
        if (assert) assert(bufferSize == indexRes_.memoryRequirements.size);
        return bufferSize;
    }

    FlagBits status_;

    // INFO
    bool isIndexed_;
    bool selectable_;

    // DSECRIPTOR
    const Descriptor::Set::BindData& getDescriptorSetBindData(const PASS& passType) const;
    Descriptor::Set::bindDataMap bindDataMap_;

    BufferResource vertexRes_;
    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource indexRes_;
    std::unique_ptr<Loading::Resources> pLdgRes_;
    std::shared_ptr<Material::Base> pMaterial_;

   private:
    void createBufferData(const VkCommandBuffer& cmd, BufferResource& stgRes, VkDeviceSize bufferSize, const void* data,
                          BufferResource& res, VkBufferUsageFlagBits usage, std::string bufferType);

    void bindPushConstants(VkCommandBuffer cmd) const;  // TODO: I hate this...

    Mesh::INDEX offset_;
};

// **********************
//      Color
// **********************

class Color : public Base {
    friend class Mesh::Handler;

   public:
    ~Color();

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) const override {
        assert(index < vertices_.size());
        return {vertices_[index]};
    }
    inline void addVertex(const Vertex::Complete& v, int32_t index = -1) override {
        if (index == -1) {
            vertices_.push_back(v.getColorVertex());
        } else {
            assert(index < vertices_.size());
            vertices_[index] = v.getColorVertex();
        }
        updateBoundingBox(vertices_.back());
    }
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const override { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize(bool assert = false) const override {
        VkDeviceSize bufferSize = sizeof(Vertex::Color) * vertices_.size();
        if (assert) assert(bufferSize == vertexRes_.memoryRequirements.size);
        return bufferSize;
    }
    const glm::vec3& getVertexPositionAtOffset(size_t offset) const override { return vertices_[offset].position; }

   protected:
    Color(Mesh::Handler& handler, const std::string&& name, CreateInfo* pCreateInfo,
          std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
          const MESH&& type = MESH::COLOR);
    Color(Mesh::Handler& handler, const FLAG&& flags, const std::string&& name, CreateInfo* pCreateInfo,
          std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
          const MESH&& type = MESH::COLOR);

    std::vector<Vertex::Color> vertices_;
};

// **********************
//      Line
// **********************

class Line : public Color {
    friend class Mesh::Handler;

   public:
    ~Line();

   protected:
    Line(Mesh::Handler& handler, const std::string&& name, CreateInfo* pCreateInfo,
         std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

// **********************
//      Texture
// **********************

class Texture : public Base {
    friend class Mesh::Handler;

   public:
    ~Texture();

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) const override {
        assert(index < vertices_.size());
        return {vertices_.at(index)};
    }
    inline void addVertex(const Vertex::Complete& v, int32_t index = -1) override {
        if (index == -1) {
            vertices_.push_back(v.getTextureVertex());
        } else {
            assert(index < vertices_.size());
            vertices_[index] = v.getTextureVertex();
        }
        updateBoundingBox(vertices_.back());
    }
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const override { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize(bool assert = false) const override {
        VkDeviceSize bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        if (assert) assert(bufferSize == vertexRes_.memoryRequirements.size);
        return bufferSize;
    }
    const glm::vec3& getVertexPositionAtOffset(size_t offset) const override { return vertices_[offset].position; }

   protected:
    Texture(Mesh::Handler& handler, const std::string&& name, CreateInfo* pCreateInfo,
            std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);

    std::vector<Vertex::Texture> vertices_;
};

}  // namespace Mesh

#endif  // !MESH_H
