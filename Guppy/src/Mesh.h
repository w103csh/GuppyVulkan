
#ifndef MESH_H
#define MESH_H

#include <future>
#include <glm/glm.hpp>
#include <vector>

#include "Constants.h"
#include "Face.h"
#include "Helpers.h"
#include "LoadingHandler.h"
#include "Material.h"
#include "Shell.h"
#include "Object3d.h"
#include "PipelineHandler.h"
#include "Texture.h"

// clang-format off
namespace Scene { class Handler; }
// clang-format on

// **********************
//  Mesh
// **********************

typedef struct MeshCreateInfo {
    bool isIndexed = true;
    glm::mat4 model = glm::mat4(1.0f);
    size_t offset = 0;
    PIPELINE pipelineType = PIPELINE::DONT_CARE;
    Material::Info materialInfo = {};
    bool selectable = true;
} MeshCreateInfo;

class Mesh : public Object3d {
   public:
    typedef enum FLAG {
        POLY = 0x00000001,
        LINE = 0x00000002,
        // THROUGH 0x00000008
    } FLAG;

    Mesh(const MESH&& type, const VERTEX&& vertexType, const FLAG&& flags, const std::string& name,
         MeshCreateInfo* pCreateInfo);
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTRs OF THIS CLASS.
        IF THIS IS REMOVED THE DESTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Mesh() = default;

    const MESH TYPE;
    const VERTEX VERTEX_TYPE;
    const PIPELINE PIPELINE_TYPE;
    const FlagBits FLAGS;
    const std::string NAME;

    inline size_t getOffset() const { return offset_; }
    inline STATUS getStatus() const { return status_; }

    // MATERIAL
    inline Material::Base& getMaterial() const { return std::ref(*pMaterial_); }
    inline bool hasNormalMap() const {
        return pMaterial_->hasTexture() && pMaterial_->getTexture()->flags & Texture::FLAG::NORMAL;
    }

    inline void setStatusPendingBuffers() {
        assert(status_ == STATUS::PENDING || status_ == STATUS::PENDING_TEXTURE);
        status_ = STATUS::PENDING_BUFFERS;
    }

    // INIT
    void setSceneData(size_t offset);

    // LOADING
    virtual void prepare(const Scene::Handler& sceneHandler, bool load);

    // VERTEX
    virtual Vertex::Complete getVertexComplete(size_t index) const = 0;
    virtual void addVertex(const Vertex::Complete& v, int32_t index = -1) = 0;
    virtual inline void addVertex(const Face& face);
    virtual inline uint32_t getVertexCount() const = 0;  // TODO: this shouldn't be public
    virtual const glm::vec3& getVertexPositionAtOffset(size_t offset) const = 0;
    void updateBuffers(const VkDevice& dev);
    inline VkBuffer& getVertexBuffer() { return vertexRes_.buffer; }

    // INDEX
    inline uint32_t getFaceCount() const {
        assert(indices_.size() % Face::NUM_VERTICES == 0);
        return static_cast<uint32_t>(indices_.size()) / Face::NUM_VERTICES;
    }
    inline Face getFace(size_t faceIndex) {
        VB_INDEX_TYPE idx0 = indices_[faceIndex + 0];
        VB_INDEX_TYPE idx1 = indices_[faceIndex + 1];
        VB_INDEX_TYPE idx2 = indices_[faceIndex + 2];
        return {getVertexComplete(idx0), getVertexComplete(idx1), getVertexComplete(idx2), idx0, idx1, idx2, 0};
    }
    inline void addIndices(std::vector<VB_INDEX_TYPE>& is) {
        for (auto i : is) indices_.push_back(i);
    }
    inline void addIndex(VB_INDEX_TYPE index) { indices_.push_back(index); }

    //// INSTANCE
    // inline void addInstance(glm::mat4 model, std::unique_ptr<Material> pMaterial) {
    //    instances_.push_back({std::move(model), std::move(pMaterial)});
    //}
    inline uint32_t getInstanceCount() const { return instances_.size() + 1; }
    // inline uint32_t getInstanceSize() const { return static_cast<uint32_t>(sizeof(PushConstants) * getInstanceCount()); }
    // inline void getInstanceData(std::vector<PushConstants>& pushConstants) const {
    //    pushConstants.push_back({getData().model, pMaterial_->getData()});
    //    for (auto& instance : instances_) {
    //        pushConstants.push_back({instance.first, instance.second->getData()});
    //    }
    //}

    // FACE
    inline bool isSelectable() { return selectable_; }
    void selectFace(const Ray& ray, float& tMin, Face& face, size_t offset) const;
    void updateTangentSpaceData();

    // DRAWING
    void draw(const VkCommandBuffer& cmd, const uint8_t& frameIndex) const;

    // PIPELINE
    void updatePipelineReferences(const PIPELINE& type, const VkPipeline& pipeline);

    virtual void destroy(const VkDevice& dev);

   protected:
    // VERTEX
    void loadBuffers(const Scene::Handler& sceneHandler);
    virtual inline const void* getVertexData() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    // INDEX
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexCount() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    // INFO
    bool isIndexed_;
    size_t offset_;
    bool selectable_;
    // INSTANCE
    std::vector<std::pair<glm::mat4, std::unique_ptr<Material::Base>>> instances_;
    // PIPELINE
    Pipeline::DescriptorSetsReference descReference_;

    STATUS status_;
    BufferResource vertexRes_;
    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource indexRes_;
    std::unique_ptr<Loading::Resources> pLdgRes_;
    std::unique_ptr<Material::Base> pMaterial_;

   private:
    void createBufferData(const Shell::Context& ctx, const Game::Settings& settings, const VkCommandBuffer& cmd,
                          BufferResource& stgRes, VkDeviceSize bufferSize, const void* data, BufferResource& res,
                          VkBufferUsageFlagBits usage, std::string bufferType);

    void bindPushConstants(VkCommandBuffer cmd) const;  // TODO: I hate this...
};

// **********************
// ColorMesh
// **********************

class ColorMesh : public Mesh {
   public:
    ColorMesh(const std::string&& name, MeshCreateInfo* pCreateInfo)
        : Mesh{MESH::COLOR, VERTEX::COLOR, FLAG::POLY, name, pCreateInfo} {}

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
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Color) * vertices_.size();
        return p_bufferSize;
    }
    const glm::vec3& getVertexPositionAtOffset(size_t offset) const override { return vertices_[offset].position; }

   protected:
    ColorMesh(const FLAG&& flags, const std::string&& name, MeshCreateInfo* pCreateInfo)
        : Mesh{MESH::COLOR, VERTEX::COLOR, std::forward<const FLAG>(flags), name, pCreateInfo} {}

    std::vector<Vertex::Color> vertices_;
};

// **********************
// LineMesh
// **********************

class LineMesh : public ColorMesh {
   public:
    LineMesh(const std::string&& name, MeshCreateInfo* pCreateInfo)
        : ColorMesh(FLAG::LINE, std::forward<const std::string>(name), pCreateInfo) {}
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(const std::string&& name, MeshCreateInfo* pCreateInfo)
        : Mesh{MESH::COLOR, VERTEX::TEXTURE, FLAG::POLY, name, pCreateInfo} {}

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
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        return p_bufferSize;
    }
    const glm::vec3& getVertexPositionAtOffset(size_t offset) const override { return vertices_[offset].position; }

    // LOADING
    void prepare(const Scene::Handler& sceneHandler, bool load) override;

   protected:
    std::vector<Vertex::Texture> vertices_;
};

#endif  // !MESH_H
