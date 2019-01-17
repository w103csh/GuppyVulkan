
#ifndef MESH_H
#define MESH_H

#include <future>
#include <glm/glm.hpp>
#include <vector>

#include "Constants.h"
#include "Face.h"
#include "Helpers.h"
#include "LoadingResourceHandler.h"
#include "Material.h"
#include "Shell.h"
#include "Object3d.h"
#include "PipelineHandler.h"
#include "Texture.h"

// **********************
//  Mesh
// **********************

typedef struct MeshCreateInfo {
    bool isIndexed = true;
    std::string markerName = "";
    glm::mat4 model = glm::mat4(1.0f);
    size_t offset = 0;
    std::unique_ptr<Material> pMaterial = nullptr;
    bool selectable = true;
} MeshCreateInfo;

class Mesh : public Object3d {
   public:
    typedef enum FLAGS {
        POLY = 0x00000001,
        LINE = 0x00000002,
        // THROUGH 0x00000008
    } FLAGS;

    Mesh(MeshCreateInfo* pCreateInfo);
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTRs OF THIS CLASS.
        IF THIS IS REMOVED THE DESTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Mesh() = default;

    // GETTERS
    inline std::string getMarkerName() const { return markerName_; }
    inline size_t getOffset() const { return offset_; }
    inline STATUS getStatus() const { return status_; }
    inline Vertex::TYPE getVertexType() const { return vertexType_; }
    inline Material& getMaterial() const { return std::ref(*pMaterial_); }
    // inline MeshCreateInfo getCreateInfo() const { return {markerName_, model_, offset_, pickable_, nullptr}; }

    inline void setStatusPendingBuffers() {
        assert(status_ == STATUS::PENDING || status_ == STATUS::PENDING_TEXTURE);
        status_ = STATUS::PENDING_BUFFERS;
    }

    // INIT
    void setSceneData(size_t offset);

    // LOADING
    virtual void prepare(const VkDevice& dev, const Game::Settings& settings, bool load);

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
    void updatePipelineReferences(const PIPELINE_TYPE& type, const VkPipeline& pipeline);

    virtual void destroy(const VkDevice& dev);

   protected:
    // VERTEX
    void loadBuffers(const VkDevice& dev, const Game::Settings& settings);
    virtual inline const void* getVertexData() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    // INDEX
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexCount() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    // PIPELINE
    PIPELINE_TYPE pipelineType_;
    Pipeline::DescriptorSetsReference descReference_;

    // create info
    bool isIndexed_;
    std::string markerName_;
    size_t offset_;
    std::unique_ptr<Material> pMaterial_;
    bool selectable_;
    // derived class specific
    FlagBits flags_;
    Vertex::TYPE vertexType_;
    // INSTANCE
    std::vector<std::pair<glm::mat4, std::unique_ptr<Material>>> instances_;

    STATUS status_;
    BufferResource vertexRes_;
    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource indexRes_;
    std::unique_ptr<LoadingResources> pLdgRes_;

   private:
    void createBufferData(const Game::Settings& settings, const VkDevice& dev, const VkCommandBuffer& cmd,
                          BufferResource& stgRes, VkDeviceSize bufferSize, const void* data, BufferResource& res,
                          VkBufferUsageFlagBits usage, std::string markerName);
};

// **********************
// ColorMesh
// **********************

class ColorMesh : public Mesh {
   public:
    ColorMesh(MeshCreateInfo* pCreateInfo);

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
    std::vector<Vertex::Color> vertices_;
};

// **********************
// LineMesh
// **********************

class LineMesh : public ColorMesh {
   public:
    LineMesh(MeshCreateInfo* pCreateInfo);
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(MeshCreateInfo* pCreateInfo);

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
    void prepare(const VkDevice& dev, const Game::Settings& settings, bool load) override;

   protected:
    std::vector<Vertex::Texture> vertices_;
};

#endif  // !MESH_H
