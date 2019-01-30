
#include <unordered_map>

#include "Mesh.h"

#include "FileLoader.h"
#include "PBR.h"  // TODO: this is bad
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

// **********************
// Mesh
// **********************

Mesh::Mesh(const MESH&& type, const VERTEX&& vertexType, const FLAG&& flags, const std::string& name,
           MeshCreateInfo* pCreateInfo)
    : Object3d(pCreateInfo->model),
      TYPE(type),
      VERTEX_TYPE(vertexType),
      FLAGS(flags),
      NAME(name),
      isIndexed_(pCreateInfo->isIndexed),
      offset_(pCreateInfo->offset),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      pMaterial_(std::make_unique<Material::Base>(&pCreateInfo->materialInfo)),
      selectable_(pCreateInfo->selectable),
      status_(STATUS::PENDING),
      vertexRes_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      indexRes_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      pLdgRes_(nullptr) {
    // PIPLINE
    assert(helpers::checkVertexPipelineMap(VERTEX_TYPE, PIPELINE_TYPE));  // checks the vertex to pipeline type map...
    // MATERIAL
    if (VERTEX_TYPE == VERTEX::TEXTURE) assert(pMaterial_->hasTexture());
}

void Mesh::setSceneData(size_t offset) { offset_ = offset; }

void Mesh::prepare(const Scene::Handler& sceneHandler, bool load) {
    if (load) {  // TODO: THIS AIN'T GREAT
        loadBuffers(sceneHandler);
        // Submit vertex loading commands...
        sceneHandler.loadingHandler().loadSubmit(std::move(pLdgRes_));
    }

    // Get descriptor sets for the drawing command...
    /*  TODO: apparently you can allocate the descriptor sets immediately, and then
        you just need to wait to use the sets until they have been updated with valid
        resources... I should implement this. As of now just wait until we
        can allocate and update all at once.
    */
    if (pMaterial_->getStatus() == STATUS::READY) {
        // Pipeline::Handler::makeDescriptorSets(PIPELINE_TYPE, descReference_, pMaterial_->getTexture());
    }

    status_ = pMaterial_->getStatus();
}

// thread sync
void Mesh::loadBuffers(const Scene::Handler& sceneHandler) {
    assert(getVertexCount());

    pLdgRes_ = sceneHandler.loadingHandler().createLoadingResources();

    // Vertex buffer
    BufferResource stgRes = {};
    createBufferData(
        sceneHandler.shell().context(), sceneHandler.settings(), pLdgRes_->transferCmd, stgRes, getVertexBufferSize(),
        getVertexData(), vertexRes_,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), "vertex");
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // Index buffer
    if (isIndexed_) {
        assert(!indices_.empty());
        stgRes = {};
        createBufferData(
            sceneHandler.shell().context(), sceneHandler.settings(), pLdgRes_->transferCmd, stgRes, getIndexBufferSize(),
            getIndexData(), indexRes_,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
            "index");
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    } else {
        assert(indices_.empty());
    }
}

void Mesh::createBufferData(const Shell::Context& ctx, const Game::Settings& settings, const VkCommandBuffer& cmd,
                            BufferResource& stgRes, VkDeviceSize bufferSize, const void* data, BufferResource& res,
                            VkBufferUsageFlagBits usage, std::string bufferType) {
    // STAGING RESOURCE
    res.memoryRequirementsSize =
        helpers::createBuffer(ctx.dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ctx.mem_props,
                              stgRes.buffer, stgRes.memory);

    // FILL STAGING BUFFER ON DEVICE
    void* pData;
    vkMapMemory(ctx.dev, stgRes.memory, 0, res.memoryRequirementsSize, 0, &pData);
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
    memcpy(pData, data, static_cast<size_t>(bufferSize));
    vkUnmapMemory(ctx.dev, stgRes.memory);

    // FAST VERTEX BUFFER
    helpers::createBuffer(ctx.dev, bufferSize,
                          // TODO: probably don't need to check memory requirements again
                          usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctx.mem_props, res.buffer, res.memory);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stgRes.buffer, res.buffer, res.memoryRequirementsSize);

    // Name the buffers for debugging
    if (settings.enable_debug_markers) {
        std::string markerName = NAME + " " + bufferType + " mesh buffer";
        ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)res.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      markerName.c_str());
    }
}

// TODO: I hate this...
void Mesh::bindPushConstants(VkCommandBuffer cmd) const {
    if (descReference_.pushConstantTypes.empty()) return;
    assert(descReference_.pushConstantTypes.size() == 1 && "Cannot handle multiple push constants");

    // TODO: MATERIAL INFO SHOULD PROBABLY BE A DYNAMIC UNIFORM (since it doesn't change constantly)

    switch (descReference_.pushConstantTypes[0]) {
        case PUSH_CONSTANT::DEFAULT: {
            // Pipeline::Default::PushConstant pushConstant = {getData(), Material::Base::getData(pMaterial_)};
            // vkCmdPushConstants(cmd, descReference_.layout, descReference_.pushConstantStages, 0,
            //                   static_cast<uint32_t>(sizeof Pipeline::Default::PushConstant), &pushConstant);
        } break;
        case PUSH_CONSTANT::PBR: {
            // Pipeline::PBR::PushConstant pushConstant = {getData(), Material::PBR::getData(pMaterial_)};
            // vkCmdPushConstants(cmd, descReference_.layout, descReference_.pushConstantStages, 0,
            //                   static_cast<uint32_t>(sizeof Pipeline::PBR::PushConstant), &pushConstant);
        } break;
        default: { assert(false && "Add new push constant"); } break;
    }
}

inline void Mesh::addVertex(const Face& face) {
    // currently not used
    for (uint8_t i = 0; i < Face::NUM_VERTICES; i++) {
        auto face = getFace(i);
        face.calculateTangentSpaceVectors();
        addVertex(face[i], face.getIndex(i));
    }
}

void Mesh::updateBuffers(const VkDevice& dev) {
    VkDeviceSize bufferSize;
    void* pData;

    // VERTEX BUFFER
    // TODO: if this assert fails more work needs to be done.
    assert(getVertexBufferSize() == vertexRes_.memoryRequirementsSize);
    pData = nullptr;
    bufferSize = getVertexBufferSize();
    vkMapMemory(dev, vertexRes_.memory, 0, vertexRes_.memoryRequirementsSize, 0, &pData);
    memcpy(pData, getVertexData(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(dev, vertexRes_.memory);

    // INDEX BUFFER
    if (isIndexed_) {
        // TODO: if this assert fails more work needs to be done.
        assert(getIndexBufferSize() == indexRes_.memoryRequirementsSize);
        pData = nullptr;
        vkMapMemory(dev, indexRes_.memory, 0, indexRes_.memoryRequirementsSize, 0, &pData);
        memcpy(pData, getVertexData(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(dev, indexRes_.memory);
    }
}

void Mesh::selectFace(const Ray& ray, float& tMin, Face& face, size_t offset) const {
    bool hit = false;
    VB_INDEX_TYPE idx0_hit = 0, idx1_hit = 0, idx2_hit = 0;

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

    float t0 = 0.0f;  //, t1 = glm::distance(localRay[0], localRay[1]);  // TODO: like this???

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

        // glm::vec3 c1 = {a, b, c};
        // glm::vec3 c2 = {d, e, f};
        // glm::vec3 c3 = {g, h, i};
        // glm::vec3 c4 = {j, k, l};

        // glm::mat3 A(c1, c2, c3);
        // auto M_ = glm::determinant(A);
        // glm::mat3 BETA(c4, c2, c3);
        // auto beta_ = glm::determinant(BETA) / M_;
        // glm::mat3 GAMMA(c1, c4, c3);
        // auto gamma_ = glm::determinant(GAMMA) / M_;
        // glm::mat3 T(c1, c2, c4);
        // auto t_ = glm::determinant(T) / M_;

        // auto t_test = t_ < t0 || t_ > tMin;
        // auto gamma_test = gamma_ < 0 || gamma_ > 1;
        // auto beta_test = beta_ < 0 || beta_ > (1 - gamma_);

        M = (a * ei_hf) + (b * gf_di) + (c * dh_eg);
        // assert(glm::epsilonEqual(M, M_, glm::epsilon<float>()));

        t = ((f * ak_jb) + (e * jc_al) + (d * bl_kc)) / -M;
        // assert(glm::epsilonEqual(t, t_, glm::epsilon<float>()));
        // test t
        if (t < t0 || t > tMin) continue;

        gamma = ((i * ak_jb) + (h * jc_al) + (g * bl_kc)) / M;
        // assert(glm::epsilonEqual(gamma, gamma_, glm::epsilon<float>()));
        // test gamma
        if (gamma < 0 || gamma > 1) continue;

        beta = ((j * ei_hf) + (k * gf_di) + (l * dh_eg)) / M;
        // assert(glm::epsilonEqual(beta, beta_, glm::epsilon<float>()));
        // test beta
        if (beta < 0 || beta > (1 - gamma)) continue;

        tMin = t;

        // TODO: below could be cleaner...
        hit = true;
        idx0_hit = idx0;
        idx1_hit = idx1;
        idx2_hit = idx2;
    }

    if (hit) {
        auto v0 = getVertexComplete(idx0_hit);
        auto v1 = getVertexComplete(idx1_hit);
        auto v2 = getVertexComplete(idx2_hit);
        v0.position = getWorldSpacePosition(v0.position);
        v1.position = getWorldSpacePosition(v1.position);
        v2.position = getWorldSpacePosition(v2.position);
        face = {v0, v1, v2, idx0_hit, idx1_hit, idx2_hit, offset};
    }
}

void Mesh::updateTangentSpaceData() {
    // currently not used
    for (size_t i = 0; i < getFaceCount(); i++) {
        auto face = getFace(i);
        face.calculateTangentSpaceVectors();
        addVertex(face);
    }
}

void Mesh::draw(const VkCommandBuffer& cmd, const uint8_t& frameIndex) const {
    VkBuffer vertex_buffers[] = {vertexRes_.buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(cmd, descReference_.bindPoint, descReference_.pipeline);

    bindPushConstants(cmd);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, descReference_.layout, descReference_.firstSet,
                            descReference_.descriptorSetCount, &descReference_.pDescriptorSets[frameIndex],
                            static_cast<uint32_t>(descReference_.dynamicOffsets.size()),
                            descReference_.dynamicOffsets.data());

    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);

    // TODO: clean these up!!
    if (indices_.size()) {
        vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0,
                             VK_INDEX_TYPE_UINT32  // TODO: Figure out how to make the type dynamic
        );
        vkCmdDrawIndexed(cmd, getIndexCount(), getInstanceCount(), 0, 0, 0);
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

void Mesh::updatePipelineReferences(const PIPELINE& type, const VkPipeline& pipeline) {
    if (type == PIPELINE_TYPE) {
        descReference_.pipeline = pipeline;
    }
}

void Mesh::destroy(const VkDevice& dev) {
    // vertex
    vkDestroyBuffer(dev, vertexRes_.buffer, nullptr);
    vkFreeMemory(dev, vertexRes_.memory, nullptr);
    // index
    if (indexRes_.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(dev, indexRes_.buffer, nullptr);
    }
    if (indexRes_.memory != VK_NULL_HANDLE) {
        vkFreeMemory(dev, indexRes_.memory, nullptr);
    }
}

// **********************
//      ColorMesh
// **********************

// **********************
//      LineMesh
// **********************

// **********************
//      TextureMesh
// **********************

void TextureMesh::prepare(const Scene::Handler& sceneHandler, bool load) {
    if (load) {  // TODO: THIS AIN'T GREAT
        loadBuffers(sceneHandler);
        // Submit vertex loading commands...
        sceneHandler.loadingHandler().loadSubmit(std::move(pLdgRes_));
    }

    // Get descriptor sets for the drawing command...
    /*  TODO: apparently you can allocate the descriptor sets immediately, and then
        you just need to not use the sets until they have updated with valid
        resources... I should implement this. As of now we will just wait until we
        can allocate and update all at once.
    */
    if (pMaterial_->getStatus() == STATUS::READY) {
        // Pipeline::Handler::makeDescriptorSets(PIPELINE_TYPE, descReference_, pMaterial_->getTexture());
        // descReference_.dynamicOffsets[0] *= pMaterial_->getTexture()->offset;
    }

    status_ = pMaterial_->getStatus();
}
