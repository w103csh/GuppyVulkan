
#include <unordered_map>

#include "CmdBufHandler.h"
#include "FileLoader.h"
#include "Mesh.h"

#include "TextureHandler.h"

// **********************
// Mesh
// **********************

Mesh::Mesh(MeshCreateInfo* pCreateInfo)
    : Object3d(pCreateInfo->model),
      markerName_(pCreateInfo->markerName),
      offset_(pCreateInfo->offset),
      pickable_(pCreateInfo->pickable),
      pMaterial_(pCreateInfo->pMaterial == nullptr ? std::make_unique<Material>() : std::move(pCreateInfo->pMaterial)),
      status_(STATUS::PENDING),
      vertex_res_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      index_res_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      pLdgRes_(LoadingResourceHandler::createLoadingResources()),
      vertexType_(),
      pipelineType_() {}

void Mesh::setSceneData(size_t offset) { offset_ = offset; }

std::future<Mesh*> Mesh::load(const Shell::Context& ctx, std::function<void(Mesh*)> callback) {
    return std::async(std::launch::async, &Mesh::async_load, this, ctx, callback);
}

Mesh* Mesh::async_load(const Shell::Context& ctx, std::function<void(Mesh*)> callback) {
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
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_res_.buffer, vertex_res_.memory);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stg_res.buffer, vertex_res_.buffer, memReqsSize);

    // Name the buffers for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)vertex_res_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                  "Mesh vertex buffer");
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

bool Mesh::selectFace(const Ray& ray, Face& face, size_t offset) const {
    if (!pickable_) return false;

    // Declare some variables that will be reused
    float a, b, c;
    float d, e, f;
    // float g, h, i;
    float j, k, l;

    float ei_hf;
    float gf_di;
    float dh_eg;
    float ak_jb;
    float jc_al;
    float bl_kc;

    float beta, gamma, t, M;

    std::array<glm::vec4, 2> localRay = {glm::vec4(ray.e, 1.0f), glm::vec4(ray.d, 1.0f)};
    worldToLocal(localRay);

    float t0 = 0.0f, t1 = glm::distance(localRay[0], localRay[1]);  // TODO: like this???

    bool faceFound = false;
    for (size_t n = 0; n < indices_.size(); n += 3) {
        // face indices
        const auto& idx0 = indices_[n];
        const auto& idx1 = indices_[n + 1];
        const auto& idx2 = indices_[n + 2];
        // face vertex positions (faces are only triangles for now)
        const auto& pa = getVertexPositionAtOffset(idx0);
        const auto& pb = getVertexPositionAtOffset(idx1);
        const auto& pc = getVertexPositionAtOffset(idx2);

        /*  This is solved using barycentric coordinates, and
            Cramer's rule.

            e + td = a + beta(b-a) + gamma(c-a)

            |xa-xb   xa-xc   xd| |b| = |xa-xe|
            |ya-yb   ya-yc   yd| |g| = |ya-ye|
            |za-zb   za-zc   zd| |t| = |za-ze|

            |a   d   g| |beta | = |j|
            |b   e   h| |gamma| = |k|
            |c   f   i| |t    | = |l|

            M =       a(ei-hf) + b(gf-di) + c(dh-eg)
            t =     (f(ak-jb) + e(jc-al) + d(bl-kc)) / -M
            gamma = (i(ak-jb) + h(jc-al) + g(bl-kc)) /  M
            beta =  (j(ei-hf) + k(gf-di) + l(dh-eg)) /  M
        */

        a = pa.x - pb.x;
        b = pa.y - pb.y;
        c = pa.z - pb.z;
        d = pa.x - pc.x;
        e = pa.y - pc.y;
        f = pa.z - pc.z;
        const auto& g = localRay[1].x;
        const auto& h = localRay[1].y;
        const auto& i = localRay[1].z;
        j = pa.x - localRay[0].x;
        k = pa.y - localRay[0].y;
        l = pa.z - localRay[0].z;

        ei_hf = (e * i) - (h * f);
        gf_di = (g * f) - (d * i);
        dh_eg = (d * h) - (e * g);
        ak_jb = (a * k) - (j * b);
        jc_al = (j * c) - (a * l);
        bl_kc = (b * l) - (k * c);

        glm::vec3 c1 = {a, b, c};
        glm::vec3 c2 = {d, e, f};
        glm::vec3 c3 = {g, h, i};
        glm::vec3 c4 = {j, k, l};

        glm::mat3 A(c1, c2, c3);
        auto M_ = glm::determinant(A);
        glm::mat3 BETA(c4, c2, c3);
        auto beta_ = glm::determinant(BETA) / M_;
        glm::mat3 GAMMA(c1, c4, c3);
        auto gamma_ = glm::determinant(GAMMA) / M_;
        glm::mat3 T(c1, c2, c4);
        auto t_ = glm::determinant(T) / M_;

        auto t_test = t_ < t0 || t_ > t1;
        auto gamma_test = gamma_ < 0 || gamma_ > 1;
        auto beta_test = beta_ < 0 || beta_ > (1 - gamma_);

        M = (a * ei_hf) + (b * gf_di) + (c * dh_eg);
        // assert(glm::epsilonEqual(M, M_, glm::epsilon<float>()));

        t = ((f * ak_jb) + (e * jc_al) + (d * bl_kc)) / -M;
        // assert(glm::epsilonEqual(t, t_, glm::epsilon<float>()));
        // test t
        if (t < t0 || t > t1) continue;

        gamma = ((i * ak_jb) + (h * jc_al) + (g * bl_kc)) / M;
        // assert(glm::epsilonEqual(gamma, gamma_, glm::epsilon<float>()));
        // test gamma
        if (gamma < 0 || gamma > 1) continue;

        beta = ((j * ei_hf) + (k * gf_di) + (l * dh_eg)) / M;
        // assert(glm::epsilonEqual(beta, beta_, glm::epsilon<float>()));
        // test beta
        if (beta < 0 || beta > (1 - gamma)) continue;

        auto vc_a = getVertexComplete(idx0);
        auto vc_b = getVertexComplete(idx1);
        auto vc_c = getVertexComplete(idx2);

        // test face
        if (!faceFound) {
            faceFound = true;
            face = {vc_a, vc_b, vc_c, idx0, idx1, idx2, offset};
        } else {
            Face tmp = {vc_a, vc_b, vc_c, idx0, idx1, idx2, offset};
            if (face.compareCentroids(localRay[0], tmp)) {
                face = tmp;
            }
        }
    }

    return faceFound;
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
        vkCmdDraw(
            cmd,
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

ColorMesh::ColorMesh(MeshCreateInfo* pCreateInfo) : Mesh(pCreateInfo) {
    flags_ = FLAGS::POLY;
    pipelineType_ = PIPELINE_TYPE::TRI_LIST_COLOR;
    vertexType_ = Vertex::TYPE::COLOR;
}

// **********************
// ColorMesh
// **********************

LineMesh::LineMesh(MeshCreateInfo* pCreateInfo) : ColorMesh(pCreateInfo) {
    flags_ = FLAGS::LINE;
    pipelineType_ = PIPELINE_TYPE::LINE;
    vertexType_ = Vertex::TYPE::COLOR;
}

// **********************
// TextureMesh
// **********************

TextureMesh::TextureMesh(MeshCreateInfo* pCreateInfo) : Mesh(pCreateInfo) {
    assert(pMaterial_->hasTexture());
    flags_ = FLAGS::POLY;
    pipelineType_ = PIPELINE_TYPE::TRI_LIST_TEX;
    vertexType_ = Vertex::TYPE::TEXTURE;
};

void TextureMesh::setSceneData(const Shell::Context& ctx, size_t offset) {
    // Only texture meshes are draw on a secondary command buffer atm...
    secCmds_.resize(ctx.image_count);
    CmdBufHandler::createCmdBuffers(CmdBufHandler::graphics_cmd_pool(), secCmds_.data(), VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                    ctx.image_count);
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
                                const VkDescriptorSet& descSet, const std::array<uint32_t, 1>& dynUboOffsets,
                                size_t frameIndex, const VkCommandBufferInheritanceInfo& inheritanceInfo,
                                const VkViewport& viewport, const VkRect2D& scissor) const {
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
        PipelineHandler::createTextureDescriptorSets(pMaterial_->getTexture().imgDescInfo, pMaterial_->getTexture().offset,
                                                     pRes);
    }
    status_ = pMaterial_->getStatus();
}

void TextureMesh::destroy(const VkDevice& dev) {
    vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), static_cast<uint32_t>(secCmds_.size()), secCmds_.data());
    Mesh::destroy(dev);
}
