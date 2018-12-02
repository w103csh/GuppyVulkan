
#ifndef MESH_H
#define MESH_H

#include <future>
#include <glm/glm.hpp>
#include <vector>

#include "Constants.h"
#include "Helpers.h"
#include "LoadingResourceHandler.h"
#include "Material.h"
#include "MyShell.h"
#include "Object3d.h"
#include "PipelineHandler.h"
#include "Texture.h"

// **********************
//  Mesh
// **********************

class Mesh : public Object3d {
   public:
    Mesh(std::unique_ptr<Material> pMaterial, glm::mat4 model = glm::mat4(1.0f));
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTRs OF THIS CLASS.
        IF THIS IS REMOVED THE DESTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Mesh() = default;

    // GETTERS
    inline std::string getMarkerName() const { return markerName_; }
    inline size_t getOffset() const { return offset_; }
    inline STATUS getStatus() const { return status_; }
    inline PIPELINE_TYPE getTopologyType() const { return pipelineType_; }
    inline Vertex::TYPE getVertexType() const { return vertexType_; }
    inline const Material& getMaterial() const { return std::cref(*pMaterial_); }

    inline void setStatusPendingBuffers() {
        assert(status_ == STATUS::PENDING || status_ == STATUS::PENDING_TEXTURE);
        status_ = STATUS::PENDING_BUFFERS;
    }
    inline void setStatusRedraw() {
        // Only allow update to redraw if ready.
        assert(status_ == STATUS::READY);
        status_ = STATUS::REDRAW;
    }
    inline void setStatusReady() {
        // Only allow update to ready if a redraw is requested.
        assert(status_ == STATUS::REDRAW);
        status_ = STATUS::READY;
    }

    // INIT
    void setSceneData(size_t offset);

    // LOADING
    virtual std::future<Mesh*> load(const MyShell::Context& ctx, std::function<void(Mesh*)> callbacak = nullptr);
    virtual void prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes);

    // VERTEX
    virtual Vertex::Complete getVertexComplete(size_t index) = 0;
    virtual void addVertex(const Vertex::Complete& v, int32_t index = -1) = 0;
    virtual inline uint32_t getVertexCount() const = 0;  // TODO: this shouldn't be public

    // INDEX
    inline void addIndices(std::vector<VB_INDEX_TYPE>& is) {
        for (auto i : is) indices_.push_back(i);
    }
    inline void addIndex(VB_INDEX_TYPE index) { indices_.push_back(index); }

    // DRAWING
    void drawInline(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                    const VkDescriptorSet& descSet) const;
    // TODO: this shouldn't be virtual... its only for textures
    virtual void drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                               const VkDescriptorSet& descSet, const std::array<uint32_t, 1>& dynUboOffsets, size_t frameIndex,
                               const VkCommandBufferInheritanceInfo& inheritanceInfo, const VkViewport& viewport,
                               const VkRect2D& scissor) const {};

    virtual void destroy(const VkDevice& dev);

   protected:
    // LOADING
    Mesh* async_load(const MyShell::Context& ctx, std::function<void(Mesh*)> callbacak = nullptr);

    // VERTEX
    void loadVertexBuffers(const VkDevice& dev);
    virtual inline const void* getVertexData() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    // INDEX
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexSize() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    STATUS status_;
    std::string markerName_;
    Vertex::TYPE vertexType_;
    PIPELINE_TYPE pipelineType_;
    BufferResource vertex_res_;
    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource index_res_;
    size_t offset_;
    std::unique_ptr<Material> pMaterial_;
    std::unique_ptr<LoadingResources> pLdgRes_;

   private:
    void createVertexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res);
    void createIndexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res);
};

// **********************
// ColorMesh
// **********************

class ColorMesh : public Mesh {
   public:
    ColorMesh(std::unique_ptr<Material> pMaterial, glm::mat4 model = glm::mat4(1.0f));

    typedef enum FLAGS {
        POLY = 0x00000001,
        LINE = 0x00000002,
        // THROUGH 0x00000008
    } FLAGS;

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) override {
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

   protected:
    std::vector<Vertex::Color> vertices_;
    FlagBits flags_;
};

// **********************
// LineMesh
// **********************

class LineMesh : public ColorMesh {
   public:
    LineMesh();
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(std::unique_ptr<Material> pMaterial, glm::mat4 model = glm::mat4(1.0f));

    // INIT
    void setSceneData(const MyShell::Context& ctx, size_t offset);

    void prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) override;
    void drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                       const VkDescriptorSet& descSet, const std::array<uint32_t, 1>& dynUboOffsets, size_t frameIndex,
                       const VkCommandBufferInheritanceInfo& inheritanceInfo, const VkViewport& viewport,
                       const VkRect2D& scissor) const override;

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) override {
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

    // TEXTURE
    inline uint32_t getTextureOffset() const { return pMaterial_->getTexture().offset; }
    void tryCreateDescriptorSets(std::unique_ptr<DescriptorResources>& pRes);

    void destroy(const VkDevice& dev) override;

   protected:
    std::vector<Vertex::Texture> vertices_;

   private:
    std::vector<VkDescriptorSet> descSets_;
    std::vector<VkCommandBuffer> secCmds_;
};

#endif  // !MESH_H
