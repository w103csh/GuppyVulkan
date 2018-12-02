
#include <unordered_map>

#include "CmdBufHandler.h"
#include "FileLoader.h"
#include "Mesh.h"

#include "TextureHandler.h"

// **********************
// Mesh
// **********************

Mesh::Mesh(std::unique_ptr<Material> pMaterial, glm::mat4 model)
    : Object3d(model),
      status_(STATUS::PENDING),
      markerName_(),
      vertexType_(),
      pipelineType_(),
      vertex_res_(),
      index_res_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      offset_(),
      pMaterial_(std::move(pMaterial)),
      pLdgRes_(LoadingResourceHandler::createLoadingResources()) {}

void Mesh::setSceneData(size_t offset) { offset_ = offset; }

std::future<Mesh*> Mesh::load(const MyShell::Context& ctx, std::function<void(Mesh*)> callback) {
    return std::async(std::launch::async, &Mesh::async_load, this, ctx, callback);
}

Mesh* Mesh::async_load(const MyShell::Context& ctx, std::function<void(Mesh*)> callback) {
    // if (!modelPath_.empty()) {
    //    switch (helpers::getModelFileType(modelPath_)) {
    //        case MODEL_FILE_TYPE::OBJ: {
    //            FileLoader::loadObj(this);
    //        } break;
    //        default: { throw std::runtime_error("file type not handled!"); } break;
    //    }
    //} else {
    //    // Vertices should have been created elsewhere then ...
    //    assert(getVertexCount());
    //}
    // status_ = STATUS::VERTICES_LOADED;

    // if (callback != nullptr) callback(this);

    return this;
}

void Mesh::prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) {
    loadVertexBuffers(dev);
    // Submit vertex loading commands...
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

    auto memReqsSize = helpers::createBuffer(dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
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
    helpers::createBuffer(dev,
                          bufferSize,  // TODO: probably don't need to check memory requirements again
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          vertex_res_.buffer, vertex_res_.memory);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stg_res.buffer, vertex_res_.buffer, memReqsSize);

    // Name the buffers for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)vertex_res_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Mesh vertex buffer");
}

void Mesh::createIndexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res) {
    VkDeviceSize bufferSize = getIndexBufferSize();

    // TODO: more checking around this scenario...
    if (bufferSize) {
        auto memReqsSize = helpers::createBuffer(dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                 stg_res.buffer, stg_res.memory);

        void* pData;
        vkMapMemory(dev, stg_res.memory, 0, memReqsSize, 0, &pData);
        memcpy(pData, getIndexData(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(dev, stg_res.memory);

        helpers::createBuffer(dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_res_.buffer, index_res_.memory);

        helpers::copyBuffer(cmd, stg_res.buffer, index_res_.buffer, memReqsSize);

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

    PushConstants pushConstants = {getData().model, pMaterial_->getData()};
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants),
                       &pushConstants);

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

void Mesh::destroy(const VkDevice& dev) {
    // vertex
    vkDestroyBuffer(dev, vertex_res_.buffer, nullptr);
    vkFreeMemory(dev, vertex_res_.memory, nullptr);
    // index
    if (index_res_.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(dev, index_res_.buffer, nullptr);
    }
    if (index_res_.memory != VK_NULL_HANDLE) {
        vkFreeMemory(dev, index_res_.memory, nullptr);
    }
}

// **********************
// ColorMesh
// **********************

ColorMesh::ColorMesh(std::unique_ptr<Material> pMaterial, glm::mat4 model) : Mesh(std::move(pMaterial), model) {
    vertexType_ = Vertex::TYPE::COLOR;
    pipelineType_ = PIPELINE_TYPE::TRI_LIST_COLOR;
    flags_ = FLAGS::POLY;
}

// **********************
// ColorMesh
// **********************

LineMesh::LineMesh() : ColorMesh(std::make_unique<Material>()) {
    vertexType_ = Vertex::TYPE::COLOR;
    pipelineType_ = PIPELINE_TYPE::LINE;
    flags_ = FLAGS::LINE;
}

// **********************
// TextureMesh
// **********************

TextureMesh::TextureMesh(std::unique_ptr<Material> pMaterial, glm::mat4 model) : Mesh(std::move(pMaterial), model) {
    assert(pMaterial_->hasTexture());
    vertexType_ = Vertex::TYPE::TEXTURE;
    pipelineType_ = PIPELINE_TYPE::TRI_LIST_TEX;
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
    Mesh::setSceneData(offset);
}

void TextureMesh::prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) {
    loadVertexBuffers(dev);
    // Submit vertex loading commands...
    LoadingResourceHandler::loadSubmit(std::move(pLdgRes_));
    // See if the descriptor sets for the texture(s) can be made...
    tryCreateDescriptorSets(pRes);
}

void TextureMesh::drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                const VkDescriptorSet& descSet, const std::array<uint32_t, 1>& dynUboOffsets, size_t frameIndex,
                                const VkCommandBufferInheritanceInfo& inheritanceInfo, const VkViewport& viewport,
                                const VkRect2D& scissor) const {
    assert(status_ == STATUS::READY);
    auto& secCmd = secCmds_[frameIndex];

    CmdBufHandler::beginCmd(secCmd, &inheritanceInfo);

    vkCmdSetViewport(secCmd, 0, 1, &viewport);
    vkCmdSetScissor(secCmd, 0, 1, &scissor);

    vkCmdBindPipeline(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    PushConstants pushConstants = {getData().model, pMaterial_->getData()};
    vkCmdPushConstants(secCmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants),
                       &pushConstants);

    vkCmdBindDescriptorSets(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descSet,
                            static_cast<uint32_t>(dynUboOffsets.size()), dynUboOffsets.data());

    VkBuffer vertexBuffs[] = {vertex_res_.buffer};
    VkDeviceSize vertexOffs[] = {0};
    vkCmdBindVertexBuffers(secCmd, 0, 1, vertexBuffs, vertexOffs);

    if (indices_.size()) {
        vkCmdBindIndexBuffer(secCmd, index_res_.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(secCmd, getIndexSize(), 1, 0, 0, 0);
    } else {
        vkCmdDraw(secCmd, getVertexCount(), 1, 0, 0);
    }

    CmdBufHandler::endCmd(secCmd);
    vkCmdExecuteCommands(cmd, 1, &secCmd);
}

void TextureMesh::tryCreateDescriptorSets(std::unique_ptr<DescriptorResources>& pRes) {
    if (pMaterial_->getStatus() == STATUS::READY) {
        PipelineHandler::createTextureDescriptorSets(pMaterial_->getTexture().imgDescInfo, pMaterial_->getTexture().offset, pRes);
    }
    status_ = pMaterial_->getStatus();
}

void TextureMesh::destroy(const VkDevice& dev) {
    vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), static_cast<uint32_t>(secCmds_.size()), secCmds_.data());
    Mesh::destroy(dev);
}
