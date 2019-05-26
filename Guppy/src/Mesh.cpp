
#include <unordered_map>

#include "Mesh.h"

#include "DescriptorHandler.h"
#include "Face.h"
#include "FileLoader.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "PBR.h"  // TODO: this is bad
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

// **********************
// Mesh
// **********************

Mesh::Base::Base(Mesh::Handler& handler, const MESH&& type, const VERTEX&& vertexType, const FLAG&& flags,
                 const std::string&& name, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                 std::shared_ptr<Material::Base>& pMaterial)
    : Handlee(handler),
      ObjDrawInst3d(pInstanceData),
      FLAGS(flags),
      MAPPABLE(pCreateInfo->mappable),
      NAME(name),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      TYPE(type),
      VERTEX_TYPE(vertexType),
      status_(STATUS::PENDING),
      // INFO
      isIndexed_(pCreateInfo->isIndexed),
      selectable_(pCreateInfo->selectable),
      // REFERENCE
      pipelineReference_{},
      descriptorReference_{},
      //
      vertexRes_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      indexRes_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      pLdgRes_(nullptr),
      pMaterial_(pMaterial),
      offset_(BAD_OFFSET)
//
{
    assert(helpers::checkVertexPipelineMap(VERTEX_TYPE, PIPELINE_TYPE));
    assert(pInstanceData_ != nullptr);
    assert(pMaterial_ != nullptr);
}

Mesh::Base::~Base() = default;

void Mesh::Base::prepare() {
    assert(status_ ^ STATUS::READY);
    assert(getOffset() != BAD_OFFSET);

    if (status_ == STATUS::PENDING_BUFFERS) {
        loadBuffers();
        // Submit vertex loading commands...
        handler().loadingHandler().loadSubmit(std::move(pLdgRes_));
        status_ = STATUS::PENDING_MATERIAL | STATUS::PENDING_PIPELINE;
    }

    if (status_ & STATUS::PENDING_MATERIAL) {
        if (pMaterial_->getStatus() == STATUS::READY) {
            status_ ^= STATUS::PENDING_MATERIAL;
        }
    }

    if (status_ & STATUS::PENDING_PIPELINE) {
        auto& pPipeline = handler().pipelineHandler().getPipeline(PIPELINE_TYPE);
        if (pPipeline->getStatus() == STATUS::READY)
            status_ ^= STATUS::PENDING_PIPELINE;
        else
            pPipeline->updateStatus();
    }

    /*  TODO: apparently you can allocate the descriptor sets immediately, and then
        you just need to wait to use the sets until they have been updated with valid
        resources... I should implement this. As of now just wait until we
        can allocate and update all at once.
    */
    if (status_ == STATUS::READY) {
        handler().pipelineHandler().getReference(std::ref(*this));
        handler().descriptorHandler().getReference(std::ref(*this));
    } else {
        handler().ldgOffsets_.insert({TYPE, getOffset()});
    }
}

// thread sync
void Mesh::Base::loadBuffers() {
    assert(getVertexCount());

    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // Vertex buffer
    BufferResource stgRes = {};
    createBufferData(
        pLdgRes_->transferCmd, stgRes, getVertexBufferSize(), getVertexData(), vertexRes_,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), "vertex");
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // Index buffer
    if (isIndexed_) {
        assert(!indices_.empty());
        stgRes = {};
        createBufferData(
            pLdgRes_->transferCmd, stgRes, getIndexBufferSize(), getIndexData(), indexRes_,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
            "index");
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    } else {
        assert(indices_.empty());
    }
}

void Mesh::Base::createBufferData(const VkCommandBuffer& cmd, BufferResource& stgRes, VkDeviceSize bufferSize,
                                  const void* data, BufferResource& res, VkBufferUsageFlagBits usage,
                                  std::string bufferType) {
    const auto& ctx = handler().shell().context();

    // STAGING RESOURCE
    res.memoryRequirements.size =
        helpers::createBuffer(ctx.dev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ctx.mem_props,
                              stgRes.buffer, stgRes.memory);

    // FILL STAGING BUFFER ON DEVICE
    void* pData;
    vk::assert_success(vkMapMemory(ctx.dev, stgRes.memory, 0, res.memoryRequirements.size, 0, &pData));
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
    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (MAPPABLE) memProps |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    helpers::createBuffer(ctx.dev, bufferSize,
                          // TODO: probably don't need to check memory requirements again
                          usage, memProps, ctx.mem_props, res.buffer, res.memory);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stgRes.buffer, res.buffer, res.memoryRequirements.size);

    // Name the buffers for debugging
    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + " " + bufferType + " mesh buffer";
        ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)res.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      markerName.c_str());
    }
}

// TODO: I hate this...
void Mesh::Base::bindPushConstants(VkCommandBuffer cmd) const {
    if (pipelineReference_.pushConstantTypes.empty()) return;
    assert(pipelineReference_.pushConstantTypes.size() == 1 && "Cannot handle multiple push constants");

    // TODO: MATERIAL INFO SHOULD PROBABLY BE A DYNAMIC UNIFORM (since it doesn't change constantly)

    assert(false);
    // switch (pipelineReference_.pushConstantTypes.front()) {
    //    case PUSH_CONSTANT::DEFAULT: {
    //        Pipeline::Default::PushConstant pushConstant = {model_};
    //        vkCmdPushConstants(cmd, pipelineReference_.layout, pipelineReference_.pushConstantStages, 0,
    //                           static_cast<uint32_t>(sizeof Pipeline::Default::PushConstant), &pushConstant);
    //    } break;
    //    default: { assert(false && "Add new push constant"); } break;
    //}
}

void Mesh::Base::addVertex(const Face& face) {
    // currently not used
    for (uint8_t i = 0; i < Face::NUM_VERTICES; i++) {
        auto face = getFace(i);
        face.calculateTangentSpaceVectors();
        addVertex(face[i], face.getIndex(i));
    }
}

void Mesh::Base::updateBuffers() {
    auto& dev = handler().shell().context().dev;
    VkDeviceSize bufferSize;
    void* pData;

    // VERTEX BUFFER
    bufferSize = getVertexBufferSize(true);
    vk::assert_success(vkMapMemory(dev, vertexRes_.memory, 0, VK_WHOLE_SIZE, 0, &pData));
    memcpy(pData, getVertexData(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(dev, vertexRes_.memory);

    // INDEX BUFFER
    if (isIndexed_) {
        bufferSize = getIndexBufferSize(true);
        vk::assert_success(vkMapMemory(dev, indexRes_.memory, 0, indexRes_.memoryRequirements.size, 0, &pData));
        memcpy(pData, getVertexData(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(dev, indexRes_.memory);
    }
}

uint32_t Mesh::Base::getFaceCount() const {
    assert(indices_.size() % Face::NUM_VERTICES == 0);
    return static_cast<uint32_t>(indices_.size()) / Face::NUM_VERTICES;
}

Face Mesh::Base::getFace(size_t faceIndex) {
    VB_INDEX_TYPE idx0 = indices_[faceIndex + 0];
    VB_INDEX_TYPE idx1 = indices_[faceIndex + 1];
    VB_INDEX_TYPE idx2 = indices_[faceIndex + 2];
    return {getVertexComplete(idx0), getVertexComplete(idx1), getVertexComplete(idx2), idx0, idx1, idx2, 0};
}

void Mesh::Base::selectFace(const Ray& ray, float& tMin, Face& face, size_t offset) const {
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

void Mesh::Base::updateTangentSpaceData() {
    // currently not used
    for (size_t i = 0; i < getFaceCount(); i++) {
        auto face = getFace(i);
        face.calculateTangentSpaceVectors();
        addVertex(face);
    }
}

void Mesh::Base::draw(const VkCommandBuffer& cmd, const uint8_t& frameIndex) const {
    vkCmdBindPipeline(cmd, pipelineReference_.bindPoint, pipelineReference_.pipeline);

    // bindPushConstants(cmd);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineReference_.layout, descriptorReference_.firstSet,
                            static_cast<uint32_t>(descriptorReference_.descriptorSets[frameIndex].size()),
                            descriptorReference_.descriptorSets[frameIndex].data(),
                            static_cast<uint32_t>(descriptorReference_.dynamicOffsets.size()),
                            descriptorReference_.dynamicOffsets.data());

    // VERTEX
    VkBuffer vertexBuffers[] = {vertexRes_.buffer};
    VkDeviceSize vertexOffsets[] = {0};
    VkDeviceSize test = 0;
    vkCmdBindVertexBuffers(cmd, Vertex::BINDING, 1, &vertexRes_.buffer, &test);

    // INSTANCE (as of now there will always be at least one instance binding)
    vkCmdBindVertexBuffers(         //
        cmd,                        // VkCommandBuffer commandBuffer
        getInstanceFirstBinding(),  // uint32_t firstBinding
        getInstanceBindingCount(),  // uint32_t bindingCount
        getInstanceBuffers(),       // const VkBuffer* pBuffers
        getInstanceOffsets()        // const VkDeviceSize* pOffsets
    );

    // TODO: clean these up!!
    if (indices_.size()) {
        // TODO: Make index type value dynamic.
        vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(               //
            cmd,                        // VkCommandBuffer commandBuffer
            getIndexCount(),            // uint32_t indexCount
            getInstanceCount(),         // uint32_t instanceCount
            0,                          // uint32_t firstIndex
            0,                          // int32_t vertexOffset
            getInstanceFirstInstance()  // uint32_t firstInstance
        );
    } else {
        vkCmdDraw(                      //
            cmd,                        // VkCommandBuffer commandBuffer
            getVertexCount(),           // uint32_t vertexCount
            getInstanceCount(),         // uint32_t instanceCount
            0,                          // uint32_t firstVertex
            getInstanceFirstInstance()  // uint32_t firstInstance
        );
    }
}

void Mesh::Base::updatePipelineReferences(const PIPELINE& type, const VkPipeline& pipeline) {
    if (type == PIPELINE_TYPE) {
        pipelineReference_.pipeline = pipeline;
    }
}

void Mesh::Base::destroy() {
    auto& dev = handler().shell().context().dev;
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
//      Color
// **********************

Mesh::Color::Color(Mesh::Handler& handler, const std::string&& name, CreateInfo* pCreateInfo,
                   std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
                   const MESH&& type)
    : Base{handler,                                //
           std::forward<const MESH>(type),         //
           VERTEX::COLOR,                          //
           FLAG::POLY,                             //
           std::forward<const std::string>(name),  //
           pCreateInfo,                            //
           pInstanceData,                          //
           pMaterial} {}

Mesh::Color::Color(Mesh::Handler& handler, const FLAG&& flags, const std::string&& name, CreateInfo* pCreateInfo,
                   std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
                   const MESH&& type)
    : Base{handler,                                //
           std::forward<const MESH>(type),         //
           VERTEX::COLOR,                          //
           std::forward<const FLAG>(flags),        //
           std::forward<const std::string>(name),  //
           pCreateInfo,                            //
           pInstanceData,                          //
           pMaterial} {}

Mesh::Color::~Color() = default;

// **********************
//      Line
// **********************

Mesh::Line::Line(Mesh::Handler& handler, const std::string&& name, CreateInfo* pCreateInfo,
                 std::shared_ptr<Instance::Base>& pInstanceData,
                 std::shared_ptr<Material::Base>& pMaterial)
    : Color{handler,                                //
            FLAG::LINE,                             //
            std::forward<const std::string>(name),  //
            pCreateInfo,                            //
            pInstanceData,                          //
            pMaterial,
            MESH::LINE} {}

Mesh::Line::~Line() = default;

// **********************
//      Texture
// **********************

Mesh::Texture::Texture(Mesh::Handler& handler, const std::string&& name, CreateInfo* pCreateInfo,
                       std::shared_ptr<Instance::Base>& pInstanceData,
                       std::shared_ptr<Material::Base>& pMaterial)
    : Base{handler,                                //
           MESH::TEXTURE,                          //
           VERTEX::TEXTURE,                        //
           FLAG::POLY,                             //
           std::forward<const std::string>(name),  //
           pCreateInfo,                            //
           pInstanceData,                          //
           pMaterial} {
    assert(pMaterial_->hasTexture());
}

Mesh::Texture::~Texture() = default;
