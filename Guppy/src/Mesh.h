
#ifndef MESH_H
#define MESH_H

#include <future>
#include <type_traits>
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
    Mesh();
    Mesh(std::string modelPath);
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTR OF THIS CLASS.
        IF THIS IS REMOVED THE DESCTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESCTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Mesh() = default;

    inline Vertex::TYPE getVertexType() { return vertexType_; }
    inline PipelineHandler::TOPOLOGY getTopologyType() { return topoType_; }
    inline std::string getMarkerName() { return markerName_; }
    inline bool isReady() { return status_ == STATUS::READY; }

    void setSceneData(const MyShell::Context& ctx, size_t offset);
    virtual std::future<Mesh*> load(const MyShell::Context& ctx);
    virtual void prepare(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, DescriptorResources& desc_res);
    virtual const VkCommandBuffer& draw(const VkDevice& dev, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                        const VkDescriptorSet& descSet, size_t frameIndex,
                                        const VkCommandBufferInheritanceInfo& inheritanceInfo) const = 0;
    virtual void destroy(const VkDevice& dev);

   protected:
    enum class STATUS { PENDING = 0, FILE_LOADED, MODEL_LOADED, TEXTURE_LOADED, READY };

    virtual void loadObj() = 0;
    virtual Mesh* async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) = 0;
    void loadModel(const VkDevice& dev, const VkCommandBuffer& transferCmd, std::vector<BufferResource>& stgResources);
    void loadSubmit(const MyShell::Context& ctx, const VkCommandBuffer& graphicsCmd, const VkCommandBuffer& transferCmd,
                    std::vector<BufferResource>& stgResources);

    virtual inline const void* getVertexData() const = 0;
    virtual inline uint32_t getVertexCount() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexSize() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    Vertex::TYPE vertexType_;
    PipelineHandler::TOPOLOGY topoType_;
    STATUS status_;
    std::string modelPath_;
    std::string markerName_;
    std::vector<VB_INDEX_TYPE> indices_;
    std::vector<VkDescriptorSet> desc_sets_;
    BufferResource vertex_res_;
    BufferResource index_res_;
    size_t offset_;
    std::vector<VkCommandBuffer> cmds_;

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

    const VkCommandBuffer& draw(const VkDevice& dev, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                const VkDescriptorSet& descSet, size_t frameIndex,
                                const VkCommandBufferInheritanceInfo& inheritanceInfo) const override;

    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Color) * vertices_.size();
        return p_bufferSize;
    }

   protected:
    Mesh* async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) override;
    void loadObj() override;

    std::vector<Vertex::Color> vertices_;

   private:
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(std::string texturePath);

    void prepare(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, DescriptorResources& desc_res);
    const VkCommandBuffer& draw(const VkDevice& dev, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                const VkDescriptorSet& descSet, size_t frameIndex,
                                const VkCommandBufferInheritanceInfo& inheritanceInfo) const override;
    void destroy(const VkDevice& dev) override;

    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        return p_bufferSize;
    }

    void draw2(const VkDevice& dev, const VkCommandBuffer& pCmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
               size_t frameIndex, const VkCommandBufferInheritanceInfo& inheritanceInfo,
               const VkViewport& viewport, const VkRect2D& scissor) const;

   protected:
    Mesh* async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) override;
    void createDescriptorSets(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info,
                                DescriptorResources& desc_res);
    void loadObj() override;
    void loadTexture(const MyShell::Context& ctx, const VkCommandBuffer& graphicsCmd, const VkCommandBuffer& transferCmd,
                     std::vector<BufferResource>& stgResources);

    std::vector<Vertex::Texture> vertices_;

   private:
    std::string texturePath_;
    Texture::TextureData texture_;
    std::vector<VkDescriptorSet> descSets_;  // these cannot be shared unfortunately... at least I can't see how
};

#endif  // !MESH_H
