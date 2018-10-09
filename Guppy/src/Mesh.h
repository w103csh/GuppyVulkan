
#ifndef MESH_H
#define MESH_H

#include <future>
#include <type_traits>
#include <vector>

#include "Constants.h"
#include "MyShell.h"
#include "Helpers.h"
#include "StagingBufferHandler.h"
#include "Texture.h"

// **********************
//  Mesh
// **********************

class Mesh {
   public:
    Mesh();
    Mesh(std::string modelPath);

    inline bool isReady() { return status_ == STATUS::READY;  }
    virtual std::future<Mesh*> load(const MyShell::Context& ctx);
    virtual void prepare(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout, const VkDescriptorPool& desc_pool,
                         const VkDescriptorBufferInfo& ubo_info) = 0;
    virtual void draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                      uint32_t index) const = 0;
    virtual void destroy(const VkDevice& dev);

   protected:
    enum class STATUS { PENDING = 0, FILE_LOADED, MODEL_LOADED, TEXTURE_LOADED, READY };

    virtual void loadObj() = 0;
    virtual Mesh* async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) = 0;
    void loadModel(const VkDevice& dev, const VkCommandBuffer& transferCmd, std::vector<StagingBufferResource>& stgResources);
    void loadSubmit(const MyShell::Context& ctx, const VkCommandBuffer& graphicsCmd, const VkCommandBuffer& transferCmd,
                    std::vector<StagingBufferResource>& stgResources);

    virtual inline const void* getVertexData() const = 0;
    virtual inline uint32_t getVertexCount() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexSize() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    STATUS status_;
    std::string modelPath_;
    std::vector<VB_INDEX_TYPE> indices_;
    std::vector<VkDescriptorSet> desc_sets_;
    BufferResource vertex_res_;
    BufferResource index_res_;

   private:
    void createVertexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, StagingBufferResource& stg_res);
    void createIndexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, StagingBufferResource& stg_res);

    virtual void Mesh::update_descriptor_set(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout,
                                             const VkDescriptorPool& desc_pool, const VkDescriptorBufferInfo& ubo_info) = 0;
};

// **********************
// ColorMesh
// **********************

class ColorMesh : public Mesh {
   public:
    ColorMesh() : Mesh(){};

    void draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
              uint32_t index) const override;
    void prepare(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout, const VkDescriptorPool& desc_pool,
                 const VkDescriptorBufferInfo& ubo_info) override;

    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Color) * vertices_.size();
        return p_bufferSize;
    }

   protected:
    void loadObj() override;
    void update_descriptor_set(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout, const VkDescriptorPool& desc_pool,
                               const VkDescriptorBufferInfo& ubo_info) override;
    Mesh* async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) override;

    std::vector<Vertex::Color> vertices_;

   private:
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(std::string texturePath);

    void draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
              uint32_t index) const override;
    void prepare(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout, const VkDescriptorPool& desc_pool,
                 const VkDescriptorBufferInfo& ubo_info) override;
    void destroy(const VkDevice& dev) override;

    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        return p_bufferSize;
    }

   protected:
    Mesh* async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) override;
    void loadObj() override;
    void loadTexture(const MyShell::Context& ctx, const VkCommandBuffer& graphicsCmd, const VkCommandBuffer& transferCmd,
                     std::vector<StagingBufferResource>& stgResources);

    void update_descriptor_set(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout, const VkDescriptorPool& desc_pool,
                               const VkDescriptorBufferInfo& ubo_info) override;

    std::vector<Vertex::Texture> vertices_;

   private:
    std::string texturePath_;
    Texture::TextureData texture_;
};

#endif  // !MESH_H
