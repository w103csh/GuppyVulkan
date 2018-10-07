
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

    inline bool isReady() const { return ready_; }
    virtual std::future<Mesh*> load(const MyShell::Context& ctx, const CommandData& cmd_data);

    virtual void prepare(const MyShell::Context& ctx, const std::vector<VkDescriptorSetLayout>& layouts,
                         const VkDescriptorPool& desc_pool, const VkDescriptorBufferInfo& ubo_info) = 0;
    virtual void draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                      uint32_t index) const = 0;
    virtual void destroy(const VkDevice& dev);

    Mesh* test(const MyShell::Context& ctx, const CommandData& cmd_data, const VkPhysicalDeviceMemoryProperties& mem_props,
               std::vector<uint32_t>& queueFamilyIndices, VkCommandBuffer& graphicsCmd
               //,VkCommandBuffer transferCmd
    );

   protected:
    virtual void loadObj() = 0;
    void addVertex(ColorVertex&& v);
    void addVertex(TextureVertex&& v);
    void addVertices(std::vector<ColorVertex>&& vs);
    void addVertices(std::vector<TextureVertex>&& vs);
    virtual Mesh* async_load(const MyShell::Context& ctx, const CommandData& cmd_data,
                             const VkPhysicalDeviceMemoryProperties& mem_props, std::vector<uint32_t>& queueFamilyIndices,
                             VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) = 0;
    void loadModel(const VkDevice& dev, const VkPhysicalDeviceMemoryProperties& mem_props, const VkCommandBuffer& transferCmd,
                   std::vector<StagingBufferResource>& stgResources);
    void loadSubmit(const MyShell::Context& ctx, const CommandData& cmd_data, const VkPhysicalDeviceMemoryProperties& mem_props,
                    std::vector<uint32_t>& queueFamilyIndices, const VkCommandBuffer& graphicsCmd,
                    const VkCommandBuffer& transferCmd, std::vector<StagingBufferResource>& stgResources);

    inline Vertex* getVertexData() { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(vertices_[0]) * vertices_.size();
        return p_bufferSize;
    }
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexSize() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    bool ready_;
    std::string modelPath_;
    std::vector<Vertex> vertices_;
    std::vector<VB_INDEX_TYPE> indices_;
    std::vector<VkDescriptorSet> desc_sets_;
    BufferResource vertex_res_;
    BufferResource index_res_;

   private:
    void createVertexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, const VkPhysicalDeviceMemoryProperties& mem_props,
                                StagingBufferResource& stg_res);
    void createIndexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, const VkPhysicalDeviceMemoryProperties& mem_props,
                               StagingBufferResource& stg_res);

    virtual void Mesh::update_descriptor_set(const MyShell::Context& ctx, const std::vector<VkDescriptorSetLayout>& layouts,
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
    void prepare(const MyShell::Context& ctx, const std::vector<VkDescriptorSetLayout>& layouts, const VkDescriptorPool& desc_pool,
                 const VkDescriptorBufferInfo& ubo_info) override;

   protected:
    void loadObj() override;
    void update_descriptor_set(const MyShell::Context& ctx, const std::vector<VkDescriptorSetLayout>& layouts,
                               const VkDescriptorPool& desc_pool, const VkDescriptorBufferInfo& ubo_info) override;
    Mesh* async_load(const MyShell::Context& ctx, const CommandData& cmd_data, const VkPhysicalDeviceMemoryProperties& mem_props,
                     std::vector<uint32_t>& queueFamilyIndices, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) override;

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
    void prepare(const MyShell::Context& ctx, const std::vector<VkDescriptorSetLayout>& layouts, const VkDescriptorPool& desc_pool,
                 const VkDescriptorBufferInfo& ubo_info) override;
    virtual void destroy(const VkDevice& dev) override;

   protected:
    void loadObj() override;
    void update_descriptor_set(const MyShell::Context& ctx, const std::vector<VkDescriptorSetLayout>& layouts,
                               const VkDescriptorPool& desc_pool, const VkDescriptorBufferInfo& ubo_info) override;
    void loadTexture(const MyShell::Context& ctx, const VkPhysicalDeviceMemoryProperties& mem_props,
                     std::vector<uint32_t>& queueFamilyIndices, const VkCommandBuffer& graphicsCmd,
                     const VkCommandBuffer& transferCmd, std::vector<StagingBufferResource>& stgResources);
    Mesh* async_load(const MyShell::Context& ctx, const CommandData& cmd_data, const VkPhysicalDeviceMemoryProperties& mem_props,
                     std::vector<uint32_t>& queueFamilyIndices, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) override;

   private:
    std::string texturePath_;
    Texture::TextureData texture_;
};

#endif  // !MESH_H
