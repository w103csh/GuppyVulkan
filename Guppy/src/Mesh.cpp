

#include <future>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <type_traits>
#include <unordered_map>

#include "CmdBufHandler.h"
#include "Mesh.h"
#include "PipelineHandler.h"

// **********************
// Mesh
// **********************

Mesh::Mesh() : modelPath_(""), status_(STATUS::PENDING), markerName_("") {}

Mesh::Mesh(std::string modelPath) : modelPath_(modelPath), status_(STATUS::PENDING), markerName_("") {}

void Mesh::setSceneData(const MyShell::Context& ctx, size_t offset) {
    offset_ = offset;

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    alloc_info.commandBufferCount = ctx.image_count;

    cmds_.resize(ctx.image_count);
    vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, cmds_.data()));
}

std::future<Mesh*> Mesh::load(const MyShell::Context& ctx) {
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

    VkCommandBuffer graphicsCmd = nullptr, transferCmd = nullptr;

    // There should always be at least a graphics queue...
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, &graphicsCmd));
    // being recording
    CmdBufHandler::beginCmd(graphicsCmd);

    // There might not be a dedicated transfer queue...
    if (CmdBufHandler::graphics_index() != CmdBufHandler::transfer_index()) {
        alloc_info.commandPool = CmdBufHandler::transfer_cmd_pool();
        vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, &transferCmd));
        // being recording
        CmdBufHandler::beginCmd(transferCmd);
    }

    return std::async(std::launch::async, &Mesh::async_load, this, ctx, std::move(graphicsCmd), std::move(transferCmd));
}

void Mesh::prepare(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, DescriptorResources& desc_res) {
    status_ = STATUS::READY;
}

void Mesh::loadModel(const VkDevice& dev, const VkCommandBuffer& transferCmd, std::vector<BufferResource>& stgResources) {
    assert(getVertexCount());

    // Vertex buffer
    BufferResource vertexStgRes = {};
    createVertexBufferData(dev, transferCmd, vertexStgRes);
    stgResources.push_back(std::move(vertexStgRes));

    // Index buffer
    BufferResource indexStgRes = {};
    createIndexBufferData(dev, transferCmd, indexStgRes);
    stgResources.push_back(std::move(indexStgRes));
}

void Mesh::loadSubmit(const MyShell::Context& ctx, const VkCommandBuffer& graphicsCmd, const VkCommandBuffer& transferCmd,
                      std::vector<BufferResource>& stgResources) {
    auto queueFamilyIndices = CmdBufHandler::getUniqueQueueFamilies(true, false, true);

    bool should_wait = queueFamilyIndices.size() > 1;

    // End buffer recording
    CmdBufHandler::endCmd(graphicsCmd);
    if (should_wait) CmdBufHandler::endCmd(transferCmd);

    // Fences for cleanup ...
    std::vector<VkFence> fences;
    for (int i = 0; i < queueFamilyIndices.size(); i++) {
        VkFence fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vk::assert_success(vkCreateFence(ctx.dev, &fence_info, nullptr, &fence));
        fences.push_back(fence);
    }

    // Semaphore
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vk::assert_success(vkCreateSemaphore(ctx.dev, &semaphore_info, nullptr, &semaphore));

    // Wait stages
    VkPipelineStageFlags wait_stages[] = {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};

    // Staging submit info
    VkSubmitInfo stag_sub_info = {};
    stag_sub_info.pNext = nullptr;
    stag_sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // Wait
    stag_sub_info.waitSemaphoreCount = 0;
    stag_sub_info.pWaitSemaphores = nullptr;
    stag_sub_info.pWaitDstStageMask = 0;
    // Command buffers
    stag_sub_info.commandBufferCount = 1;
    stag_sub_info.pCommandBuffers = &transferCmd;
    // Signal
    stag_sub_info.signalSemaphoreCount = 1;
    stag_sub_info.pSignalSemaphores = &semaphore;

    VkSubmitInfo wait_sub_info = {};
    if (should_wait) {
        // Wait submit info
        wait_sub_info = {};
        wait_sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        wait_sub_info.pNext = nullptr;
        // Wait
        wait_sub_info.waitSemaphoreCount = 1;
        wait_sub_info.pWaitSemaphores = &semaphore;
        wait_sub_info.pWaitDstStageMask = wait_stages;
        // Command buffers
        wait_sub_info.commandBufferCount = 1;
        wait_sub_info.pCommandBuffers = &graphicsCmd;
        // Signal
        wait_sub_info.signalSemaphoreCount = 0;
        wait_sub_info.pSignalSemaphores = nullptr;
    }

    // Sumbit ...
    if (should_wait) {
        vk::assert_success(vkQueueSubmit(CmdBufHandler::transfer_queue(), 1, &stag_sub_info, fences[0]));
    }
    vk::assert_success(vkQueueSubmit(CmdBufHandler::graphics_queue(), 1, &wait_sub_info, fences[1]));

    // **************************
    // Cleanup
    // **************************

    // Wait for fences
    vk::assert_success(vkWaitForFences(ctx.dev, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, UINT64_MAX));

    // Free stating resources
    for (auto& res : stgResources) {
        vkDestroyBuffer(ctx.dev, res.buffer, nullptr);
        vkFreeMemory(ctx.dev, res.memory, nullptr);
    }
    stgResources.clear();

    // Free fences
    for (auto& fence : fences) vkDestroyFence(ctx.dev, fence, nullptr);

    // Free semaphores
    vkDestroySemaphore(ctx.dev, semaphore, nullptr);

    // Free command buffers
    // switch (type) {
    //    case END_TYPE::DESTROY: {
    vkFreeCommandBuffers(ctx.dev, CmdBufHandler::graphics_cmd_pool(), 1, &graphicsCmd);
    if (should_wait) {
        vkFreeCommandBuffers(ctx.dev, CmdBufHandler::transfer_cmd_pool(), 1, &transferCmd);
    }
    //    } break;
    //    case END_TYPE::RESET: {
    //        vkResetCommandBuffer(cmd_data_->cmds[cmd_data_->transfer_queue_family], 0);
    //        helpers::execute_begin_command_buffer(cmd_data_->cmds[cmd_data_->transfer_queue_family]);

    //        vkResetCommandBuffer(cmd_data_->cmds[cmd_res->wait_queue_family], 0);
    //        helpers::execute_begin_command_buffer(cmd_data_->cmds[cmd_res->wait_queue_family]);
    //    } break;
    //}
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

Mesh* ColorMesh::async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) {
    std::vector<BufferResource> stgResources;
    loadModel(ctx.dev, transferCmd, stgResources);
    loadSubmit(ctx, graphicsCmd, transferCmd, stgResources);
    return this;
}

const VkCommandBuffer& ColorMesh::draw(const VkDevice& dev, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                       const VkDescriptorSet& descSet, size_t frameIndex,
                                       const VkCommandBufferInheritanceInfo& inheritanceInfo) const {
    assert(status_ == STATUS::READY);

    auto& cmd = cmds_[frameIndex];
    CmdBufHandler::beginCmd(cmd);

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

        // Add debug marker for mesh name
        ext::DebugMarkerInsert(cmd, "Draw (insert name here)", glm::vec4(0.0f));

        vkCmdDrawIndexed(cmd, getIndexSize(), 1, 0, 0, 0);
    } else {
        // Add debug marker for mesh name
        ext::DebugMarkerInsert(cmd, "Draw (insert name here)", glm::vec4(0.0f));

        vkCmdDraw(cmd,
                  getVertexCount(),  // vertexCount
                  1,                 // instanceCount - Used for instanced rendering, use 1 if you're not doing that.
                  0,  // firstVertex - Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
                  0   // firstInstance - Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        );
    }

    CmdBufHandler::endCmd(cmd);
    return cmd;
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

// void ColorMesh::update_descriptor_set(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout,
//                                      const VkDescriptorPool& desc_pool, const VkDescriptorBufferInfo& ubo_info) {
//    std::vector<VkDescriptorSetLayout> layouts(ctx.image_count, layout);
//
//    VkDescriptorSetAllocateInfo alloc_info = {};
//    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    alloc_info.descriptorPool = desc_pool;
//    alloc_info.descriptorSetCount = static_cast<uint32_t>(ctx.image_count);
//    alloc_info.pSetLayouts = layouts.data();
//
//    desc_sets_.resize(ctx.image_count);
//    vk::assert_success(vkAllocateDescriptorSets(ctx.dev, &alloc_info, desc_sets_.data()));
//
//    // TODO: hardcoded ubo 1 here...
//    uint32_t descriptorWriteCount = 1;
//
//    for (size_t i = 0; i < ctx.image_count; i++) {
//        std::vector<VkWriteDescriptorSet> writes(descriptorWriteCount);
//
//        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writes[0].dstSet = desc_sets_[i];
//        writes[0].dstBinding = 0;
//        writes[0].dstArrayElement = 0;
//        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        writes[0].descriptorCount = 1;
//        writes[0].pBufferInfo = &ubo_info;
//
//        vkUpdateDescriptorSets(ctx.dev, descriptorWriteCount, writes.data(), 0, nullptr);
//    }
//}

// **********************
// TextureMesh
// **********************

TextureMesh::TextureMesh(std::string texturePath) : Mesh(), texturePath_(texturePath) {
    vertexType_ = Vertex::TYPE::TEXTURE;
    topoType_ = PipelineHandler::TOPOLOGY::TRI_LIST_TEX;
};

Mesh* TextureMesh::async_load(const MyShell::Context& ctx, VkCommandBuffer graphicsCmd, VkCommandBuffer transferCmd) {
    std::vector<BufferResource> stgResources;
    loadModel(ctx.dev, transferCmd, stgResources);
    loadTexture(ctx, graphicsCmd, transferCmd, stgResources);
    loadSubmit(ctx, graphicsCmd, transferCmd, stgResources);
    return this;
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

void TextureMesh::loadTexture(const MyShell::Context& ctx, const VkCommandBuffer& graphicsCmd, const VkCommandBuffer& transferCmd,
                              std::vector<BufferResource>& stgResources) {
    BufferResource textureStgRes = {};
    texture_ =
        Texture::createTexture(ctx.dev, graphicsCmd, transferCmd, texturePath_, ctx.linear_blitting_supported_, textureStgRes);
    stgResources.push_back(std::move(textureStgRes));
}

void TextureMesh::prepare(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, DescriptorResources& desc_res) {
    createDescriptorSets(ctx, ubo_info, desc_res);
    status_ = STATUS::READY;
}

const VkCommandBuffer& TextureMesh::draw(const VkDevice& dev, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                                         const VkDescriptorSet& descSet, size_t frameIndex,
                                         const VkCommandBufferInheritanceInfo& inheritanceInfo) const {
    assert(status_ == STATUS::READY);

    auto& cmd = cmds_[frameIndex];
    CmdBufHandler::beginCmd(cmd, &inheritanceInfo);

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
        // vkCmdDrawIndexed(cmd, getIndexSize(), 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd,
                  getVertexCount(),  // vertexCount
                  1,                 // instanceCount - Used for instanced rendering, use 1 if you're not doing that.
                  0,  // firstVertex - Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
                  0   // firstInstance - Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        );
    }

    CmdBufHandler::endCmd(cmd);
    return cmd;
}

void TextureMesh::draw2(const VkDevice& dev, const VkCommandBuffer& priCmd, const VkPipelineLayout& layout,
                        const VkPipeline& pipeline, size_t frameIndex, const VkCommandBufferInheritanceInfo& inheritanceInfo,
                        const VkViewport& viewport, const VkRect2D& scissor) const {
    assert(status_ == STATUS::READY);
    auto& secCmd = cmds_[frameIndex];

    CmdBufHandler::beginCmd(secCmd, &inheritanceInfo);

    vkCmdSetViewport(secCmd, 0, 1, &viewport);
    vkCmdSetScissor(secCmd, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {vertex_res_.buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descSets_[frameIndex], 0, nullptr);
    vkCmdBindVertexBuffers(secCmd, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(secCmd, index_res_.buffer, 0,
                         VK_INDEX_TYPE_UINT32  // TODO: Figure out how to make the type dynamic
    );
    vkCmdDrawIndexed(secCmd, getIndexSize(), 1, 0, 0, 0);

    CmdBufHandler::endCmd(secCmd);
    vkCmdExecuteCommands(priCmd, 1, &secCmd);
}

void TextureMesh::createDescriptorSets(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info,
                                       DescriptorResources& desc_res) {
    std::vector<VkDescriptorSetLayout> layouts;
    PipelineHandler::getDescriptorLayouts(ctx.image_count, Vertex::TYPE::TEXTURE, layouts);
    auto setCount = static_cast<uint32_t>(layouts.size());

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = desc_res.pool;
    alloc_info.descriptorSetCount = setCount;
    alloc_info.pSetLayouts = layouts.data();

    descSets_.resize(ctx.image_count);
    vk::assert_success(vkAllocateDescriptorSets(ctx.dev, &alloc_info, descSets_.data()));

    std::vector<VkWriteDescriptorSet> writes;
    for (size_t i = 0; i < ctx.image_count; i++) {
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descSets_[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &ubo_info;
        writes.push_back(write);

        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descSets_[i];
        write.dstBinding = 1;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &texture_.imgDescInfo;
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(ctx.dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

// void TextureMesh::update_descriptor_set(const MyShell::Context& ctx, const VkDescriptorSetLayout& layout,
//                                        const VkDescriptorPool& desc_pool, const VkDescriptorBufferInfo& ubo_info) {
//    std::vector<VkDescriptorSetLayout> layouts(ctx.image_count, layout);
//
//    VkDescriptorSetAllocateInfo alloc_info = {};
//    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    alloc_info.descriptorPool = desc_pool;
//    alloc_info.descriptorSetCount = static_cast<uint32_t>(ctx.image_count);
//    alloc_info.pSetLayouts = layouts.data();
//
//    desc_sets_.resize(ctx.image_count);
//    vk::assert_success(vkAllocateDescriptorSets(ctx.dev, &alloc_info, desc_sets_.data()));
//
//    // TODO: hardcoded ubo 1 here...
//    uint32_t descriptorWriteCount = 2;
//
//    for (size_t i = 0; i < ctx.image_count; i++) {
//        std::vector<VkWriteDescriptorSet> writes(descriptorWriteCount);
//
//        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writes[0].dstSet = desc_sets_[i];
//        writes[0].dstBinding = 0;
//        writes[0].dstArrayElement = 0;
//        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        writes[0].descriptorCount = 1;
//        writes[0].pBufferInfo = &ubo_info;
//
//        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writes[1].dstSet = desc_sets_[i];
//        writes[1].dstBinding = 1;
//        writes[1].dstArrayElement = 0;
//        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        writes[1].descriptorCount = 1;
//        writes[1].pImageInfo = &texture_.imgDescInfo;
//
//        // writes[1].descriptorCount = static_cast<uint32_t>(textures_.size());
//        // std::vector<VkDescriptorImageInfo> imgDescInfos;
//        // for (auto& texture : textures_) imgDescInfos.push_back(texture.imgDescInfo);
//        // writes[1].pImageInfo = imgDescInfos.data();
//
//        vkUpdateDescriptorSets(ctx.dev, descriptorWriteCount, writes.data(), 0, nullptr);
//    }
//}

void TextureMesh::destroy(const VkDevice& dev) {
    Mesh::destroy(dev);
    // texture
    vkDestroySampler(dev, texture_.sampler, nullptr);
    vkDestroyImageView(dev, texture_.view, nullptr);
    vkDestroyImage(dev, texture_.image, nullptr);
    vkFreeMemory(dev, texture_.memory, nullptr);
}
