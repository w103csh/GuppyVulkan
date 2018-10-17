
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

#include "CmdBufHandler.h"
#include "Mesh.h"

// **********************
// Mesh
// **********************

Mesh::Mesh() : modelPath_(""), status_(STATUS::PENDING), markerName_(""), pLdgRes_(nullptr) {}

Mesh::Mesh(std::string modelPath) : modelPath_(modelPath), status_(STATUS::PENDING), markerName_(""), pLdgRes_(nullptr) {}

void Mesh::setSceneData(const MyShell::Context& ctx, size_t offset) { offset_ = offset; }

std::future<Mesh*> Mesh::load(const MyShell::Context& ctx) {
    pLdgRes_ = LoadingResourceHandler::createLoadingResources();
    return std::async(std::launch::async, &Mesh::async_load, this, ctx);
}

Mesh* Mesh::async_load(const MyShell::Context& ctx) {
    if (!modelPath_.empty()) {
        switch (helpers::getModelFileType(modelPath_)) {
            case MODEL_FILE_TYPE::OBJ: {
                loadObj();
            } break;
            default: { throw std::runtime_error("file type not handled!"); } break;
        }
    } else {
        // Vertices should have been created elsewhere then ...
        assert(getVertexCount());
    }
    status_ = STATUS::VERTICES_LOADED;
    return this;
}

void Mesh::prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) {
    loadVertexBuffers(dev);
    LoadingResourceHandler::loadSubmit(std::move(pLdgRes_));
    status_ = STATUS::READY;
}

void Mesh::loadVertexBuffers(const VkDevice& dev) {
    assert(getVertexCount());

    // Vertex buffer
    BufferResource vertexStgRes = {};
    createVertexBufferData(dev, pLdgRes_->transferCmd, vertexStgRes);
    pLdgRes_->stgResources.push_back(std::move(vertexStgRes));

    // Index buffer
    BufferResource indexStgRes = {};
    createIndexBufferData(dev, pLdgRes_->transferCmd, indexStgRes);
    pLdgRes_->stgResources.push_back(std::move(indexStgRes));
}

void Mesh::createVertexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res) {
    // STAGING BUFFER
    VkDeviceSize bufferSize = getVertexBufferSize();

    auto memReqsSize = helpers::create_buffer(dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                              stg_res.buffer, stg_res.memory);

    // FILL STAGING BUFFER ON DEVICE
    void* pData;
    vkMapMemory(dev, stg_res.memory, 0, memReqsSize, 0, &pData);
    /*
        You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory.
        Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
        of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
        are two ways to deal with that problem:
            - Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            - Call vkFlushMappedMemoryRanges to after writing to the mapped memory, and call
              vkInvalidateMappedMemoryRanges before reading from the mapped memory
        We went for the first approach, which ensures that the mapped memory always matches the contents of the
        allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
        but we'll see why that doesn't matter in the next chapter.
    */
    memcpy(pData, getVertexData(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(dev, stg_res.memory);

    // FAST VERTEX BUFFER
    helpers::create_buffer(dev,
                           bufferSize,  // TODO: probably don't need to check memory requirements again
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_res_.buffer, vertex_res_.memory);

    // COPY FROM STAGING TO FAST
    helpers::copy_buffer(cmd, stg_res.buffer, vertex_res_.buffer, memReqsSize);

    // Name the buffers for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)vertex_res_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Mesh vertex buffer");
}

void Mesh::createIndexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res) {
    VkDeviceSize bufferSize = getIndexBufferSize();

    // TODO: more checking around this scenario...
    if (bufferSize) {
        auto memReqsSize = helpers::create_buffer(dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                  stg_res.buffer, stg_res.memory);

        void* pData;
        vkMapMemory(dev, stg_res.memory, 0, memReqsSize, 0, &pData);
        memcpy(pData, getIndexData(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(dev, stg_res.memory);

        helpers::create_buffer(dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_res_.buffer, index_res_.memory);

        helpers::copy_buffer(cmd, stg_res.buffer, index_res_.buffer, memReqsSize);

        // Name the buffers for debugging
        ext::DebugMarkerSetObjectName(dev, (uint64_t)index_res_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      "Mesh index buffer");
    }
}

void Mesh::drawInline(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                      const VkDescriptorSet& descSet) const {
    assert(status_ == STATUS::READY);

    VkBuffer vertex_buffers[] = {vertex_res_.buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);

    // TODO: clean these up!!
    if (indices_.size()) {
        vkCmdBindIndexBuffer(cmd, index_res_.buffer, 0,
                             VK_INDEX_TYPE_UINT32  // TODO: Figure out how to make the type dynamic
        );
        vkCmdDrawIndexed(cmd, getIndexSize(), 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd,
                  getVertexCount(),  // vertexCount
                  1,                 // instanceCount - Used for instanced rendering, use 1 if you're not doing that.
                  0,  // firstVertex - Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
                  0   // firstInstance - Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        );
    }
}

void TextureMesh::drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                const VkDescriptorSet& descSet, size_t frameIndex,
                                const VkCommandBufferInheritanceInfo& inheritanceInfo, const VkViewport& viewport,
                                const VkRect2D& scissor) const {
    assert(status_ == STATUS::READY);
    auto& secCmd = secCmds_[frameIndex];

    CmdBufHandler::beginCmd(secCmd, &inheritanceInfo);

    vkCmdSetViewport(secCmd, 0, 1, &viewport);
    vkCmdSetScissor(secCmd, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {vertex_res_.buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descSet, 0, nullptr);
    vkCmdBindVertexBuffers(secCmd, 0, 1, vertex_buffers, offsets);

    if (indices_.size()) {
        vkCmdBindIndexBuffer(secCmd, index_res_.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(secCmd, getIndexSize(), 1, 0, 0, 0);
    } else {
        vkCmdDraw(secCmd, getVertexCount(), 1, 0, 0);
    }

    CmdBufHandler::endCmd(secCmd);
    vkCmdExecuteCommands(cmd, 1, &secCmd);
}

void Mesh::destroy(const VkDevice& dev) {
    // vertex
    vkDestroyBuffer(dev, vertex_res_.buffer, nullptr);
    vkFreeMemory(dev, vertex_res_.memory, nullptr);
    // index
    vkDestroyBuffer(dev, index_res_.buffer, nullptr);
    vkFreeMemory(dev, index_res_.memory, nullptr);
}

// **********************
// ColorMesh
// **********************

ColorMesh::ColorMesh() {
    vertexType_ = Vertex::TYPE::COLOR;
    topoType_ = PipelineHandler::TOPOLOGY::TRI_LIST_COLOR;
}

void ColorMesh::loadObj() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath_.c_str())) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex::Color, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            // Create the vertex ...
            Vertex::Color vertex = {};
            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};
            vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());

                vertices_.push_back(std::move(vertex));
            }

            indices_.push_back(uniqueVertices[vertex]);
        }
    }
}

// **********************
// TextureMesh
// **********************

TextureMesh::TextureMesh(std::shared_ptr<Texture::TextureData> pTex) : pTex_(pTex) {
    assert(pTex);
    vertexType_ = Vertex::TYPE::TEXTURE;
    topoType_ = PipelineHandler::TOPOLOGY::TRI_LIST_TEX;
};

TextureMesh::TextureMesh(std::shared_ptr<Texture::TextureData> pTex, std::string modelPath) : Mesh(modelPath), pTex_(pTex) {
    assert(pTex);
    vertexType_ = Vertex::TYPE::TEXTURE;
    topoType_ = PipelineHandler::TOPOLOGY::TRI_LIST_TEX;
};

void TextureMesh::setSceneData(const MyShell::Context& ctx, size_t offset) {
    // Only texture meshes are draw on a secondary command buffer atm...
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    alloc_info.commandBufferCount = ctx.image_count;

    secCmds_.resize(ctx.image_count);
    vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, secCmds_.data()));

    // call override
    Mesh::setSceneData(ctx, offset);
}

void TextureMesh::loadObj() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath_.c_str())) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex::Texture, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            // Create the vertex ...
            Vertex::Texture vertex = {};
            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};
            vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                               1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());

                vertices_.push_back(std::move(vertex));
            }

            indices_.push_back(uniqueVertices[vertex]);
        }
    }
}

void TextureMesh::prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) {
    loadVertexBuffers(dev);
    LoadingResourceHandler::loadSubmit(std::move(pLdgRes_));
    if (pTex_->status == Texture::STATUS::READY) {
        PipelineHandler::createTextureDescriptorSets(pTex_->imgDescInfo, pTex_->offset, pRes);
        status_ = STATUS::READY;
    } else {
        status_ = STATUS::PENDING_TEXTURE;
    }
}

void TextureMesh::destroy(const VkDevice& dev) {
    vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), static_cast<uint32_t>(secCmds_.size()), secCmds_.data());
    Mesh::destroy(dev);
}
