
#ifndef MESH_H
#define MESH_H

#include <future>
#include <vector>

#include "Constants.h"
#include "MyShell.h"
#include "Helpers.h"
#include "PipelineHandler.h"
#include "Texture.h"

// **********************
//  Mesh
// **********************

class Mesh {
   public:
    enum class STATUS { PENDING = 0, VERTICES_LOADED, PENDING_TEXTURE, READY };

    Mesh();
    Mesh(std::string modelPath);
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTR OF THIS CLASS.
        IF THIS IS REMOVED THE DESCTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESCTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Mesh() = default;

    // GETTERS

    inline std::string getMarkerName() const { return markerName_; }
    inline size_t getOffset() const { return offset_; }
    inline PipelineHandler::TOPOLOGY getTopologyType() const { return topoType_; }
    inline Vertex::TYPE getVertexType() const { return vertexType_; }
    inline STATUS getStatus() const { return status_; }
    STATUS status_;

    // INIT
    virtual void setSceneData(const MyShell::Context& ctx, size_t offset);

    // LOADING
    virtual std::future<Mesh*> load(const MyShell::Context& ctx);
    virtual void prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes);

    // DRAWING
    void drawInline(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                    const VkDescriptorSet& descSet) const;
    // TODO: this shouldn't be virtual... its only for textures
    virtual void drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                               const VkDescriptorSet& descSet, size_t frameIndex,
                               const VkCommandBufferInheritanceInfo& inheritanceInfo, const VkViewport& viewport,
                               const VkRect2D& scissor) const {};

    virtual void destroy(const VkDevice& dev);

   protected:
    // LOADING
    Mesh* async_load(const MyShell::Context& ctx);
    virtual void loadObj() = 0;

    // VERTEX
    void loadVertexBuffers(const VkDevice& dev);
    virtual inline const void* getVertexData() const = 0;
    virtual inline uint32_t getVertexCount() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    // INDEX
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexSize() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    std::string modelPath_;
    std::string markerName_;
    Vertex::TYPE vertexType_;
    PipelineHandler::TOPOLOGY topoType_;
    BufferResource vertex_res_;
    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource index_res_;
    size_t offset_;
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
    ColorMesh();

    // VERTEX
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Color) * vertices_.size();
        return p_bufferSize;
    }

   protected:
    // LOADING
    // Mesh* async_load(const MyShell::Context& ctx) override;
    void loadObj() override;

    std::vector<Vertex::Color> vertices_;

   private:
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(std::shared_ptr<Texture::TextureData> pTex);
    TextureMesh(std::shared_ptr<Texture::TextureData> pTex, std::string modelPath);

    // INIT
    void setSceneData(const MyShell::Context& ctx, size_t offset) override;

    void prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) override;
    void drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                       const VkDescriptorSet& descSet, size_t frameIndex, const VkCommandBufferInheritanceInfo& inheritanceInfo,
                       const VkViewport& viewport, const VkRect2D& scissor) const override;

    // VERTEX
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        return p_bufferSize;
    }

    // TEXTURE
    inline void tryCreateDescriptorSets(std::unique_ptr<DescriptorResources>& pRes) {
        if (pTex_->status == Texture::STATUS::READY) {
            PipelineHandler::createTextureDescriptorSets(pTex_->imgDescInfo, pTex_->offset, pRes);
            status_ = STATUS::READY;
        }
    }
    inline uint32_t getTextureOffset() const { return pTex_->offset; }

    void destroy(const VkDevice& dev) override;

   protected:
    // LOADING
    // Mesh* async_load(const MyShell::Context& ctx) override;
    void loadObj() override;

    std::vector<Vertex::Texture> vertices_;

   private:
    std::shared_ptr<Texture::TextureData> pTex_;
    std::vector<VkDescriptorSet> descSets_;
    std::vector<VkCommandBuffer> secCmds_;
};

#endif  // !MESH_H
