/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MESH_H
#define MESH_H

#include <future>
#include <glm/glm.hpp>
#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <Common/Helpers.h>

#include "ConstantsAll.h"
#include "Geometry.h"
#include "Handlee.h"
#include "Instance.h"
#include "Material.h"
#include "Obj3dDrawInst.h"
#include "Shell.h"
#include "Texture.h"

// clang-format off
class Face;
namespace Loading { struct Resource; }
namespace Scene   { class Handler; }
// clang-format on

namespace Mesh {

struct GenericCreateInfo;
class Handler;

// BASE

class Base : public NonCopyable, public Handlee<Mesh::Handler>, public Obj3d::InstanceDraw {
    friend class Mesh::Handler;
    friend class Descriptor::Handler;  // Reference (TODO: get rid of this)

   public:
    const bool MAPPABLE;
    const std::string NAME;
    const std::set<PASS> PASS_TYPES;
    const PIPELINE PIPELINE_TYPE;
    const Settings SETTINGS;
    const MESH TYPE;
    const VERTEX VERTEX_TYPE;

    constexpr const auto& getOffset() const { return offset_; }
    constexpr const auto& getStatus() const { return status_; }

    // MATERIAL
    inline auto& getMaterial() const { return pMaterial_; }
    inline bool hasNormalMap() const {
        return pMaterial_->hasTexture() && pMaterial_->getTexture()->flags & ::Sampler::USAGE::NORMAL;
    }

    inline void setStatus(const STATUS&& status) {
        switch (status) {
            case STATUS::PENDING_BUFFERS:
                assert(status_ == STATUS::PENDING_VERTICES);
                status_ = STATUS::PENDING_BUFFERS;
                break;
            default:
                assert(false && "Unhandled case for mesh status");
                break;
        }
    }

    // LOADING
    void prepare();

    // VERTEX
    virtual Vertex::Complete getVertexComplete(size_t index) const = 0;
    virtual void addVertex(const Vertex::Complete& v, int32_t index = -1) = 0;
    virtual void addVertex(const Face& face);
    virtual inline uint32_t getVertexCount() const = 0;  // TODO: this shouldn't be public
    virtual const glm::vec3& getVertexPositionAtOffset(size_t offset) const = 0;
    void updateBuffers();
    inline vk::Buffer& getVertexBuffer() { return vertexRes_.buffer; }

    // INDEX
    uint32_t getFaceCount() const;
    Face getFace(size_t faceIndex);
    inline void addIndices(std::vector<IndexBufferType>& is) {
        for (auto i : is) indices_.push_back(i);
    }
    inline void addIndex(IndexBufferType index) { indices_.push_back(index); }

    // FACE
    inline bool isSelectable() { return selectable_; }
    void selectFace(const Ray& ray, float& tMin, Face& face, size_t offset) const;
    void updateTangentSpaceData();

    // DRAWING
    bool shouldDraw(const PASS& passTypeComp, const PIPELINE& pipelineType) const;
    void draw(const RENDER_PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const vk::CommandBuffer& cmd, const uint8_t frameIndex) const;
    void draw(const RENDER_PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
              const uint8_t frameIndex) const;

    virtual void destroy();

   protected:
    Base(Mesh::Handler& handler, const index&& offset, const MESH&& type, const VERTEX&& vertexType,
         const std::string&& name, const CreateInfo* pCreateInfo, std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData,
         std::shared_ptr<Material::Base>& pMaterial);
    Base() = delete;
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTRs OF THIS CLASS.
        IF THIS IS REMOVED THE DESTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Base();

    // VERTEX
    void loadBuffers();
    virtual inline const void* getVertexData() const = 0;
    virtual inline vk::DeviceSize getVertexBufferSize(bool assert = false) const = 0;

    // INDEX
    inline IndexBufferType* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexCount() const { return static_cast<uint32_t>(indices_.size()); }
    inline vk::DeviceSize getIndexBufferSize(bool assert = false) const {
        vk::DeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();
        if (assert) assert(bufferSize == indexRes_.memoryRequirements.size);
        return bufferSize;
    }
    // INDEX (ADJACENCY)
    virtual void makeAdjacenyList();
    inline vk::DeviceSize getIndexBufferAdjSize(bool assert = false) const {
        vk::DeviceSize bufferSize = sizeof(indicesAdjaceny_[0]) * indicesAdjaceny_.size();
        if (assert) assert(bufferSize == indexAdjacencyRes_.memoryRequirements.size);
        return bufferSize;
    }

    FlagBits status_;

    // INFO
    bool selectable_;

    // DSECRIPTOR
    const Descriptor::Set::BindData& getDescriptorSetBindData(const PASS& passType) const;
    Descriptor::Set::bindDataMap descSetBindDataMap_;

    BufferResource vertexRes_;
    std::vector<IndexBufferType> indices_;
    BufferResource indexRes_;
    std::vector<IndexBufferType> indicesAdjaceny_;
    BufferResource indexAdjacencyRes_;
    std::unique_ptr<LoadingResource> pLdgRes_;
    std::shared_ptr<Material::Base> pMaterial_;

   private:
    void createBufferData(const vk::CommandBuffer& cmd, BufferResource& stgRes, vk::DeviceSize bufferSize, const void* data,
                          BufferResource& res, vk::BufferUsageFlagBits usage, std::string bufferType);

    Mesh::index offset_;
};

// COLOR

class Color : public Base {
    friend class Mesh::Handler;

   public:
    ~Color();

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) const override {
        assert(index < vertices_.size());
        return {vertices_[index]};
    }
    inline void addVertex(const Vertex::Complete& v, int32_t index = -1) override {
        if (index == -1) {
            vertices_.push_back(v.getVertex<Vertex::Color>());
        } else {
            assert(index < vertices_.size());
            vertices_[index] = v.getVertex<Vertex::Color>();
        }
        pInstObj3d_->updateBoundingBox(vertices_.back());
    }
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const override { return vertices_.size(); }
    inline vk::DeviceSize getVertexBufferSize(bool assert = false) const override {
        vk::DeviceSize bufferSize = sizeof(Vertex::Color) * vertices_.size();
        if (assert) assert(bufferSize == vertexRes_.memoryRequirements.size);
        return bufferSize;
    }
    const glm::vec3& getVertexPositionAtOffset(size_t offset) const override { return vertices_[offset].position; }

   protected:
    // This is the generic constructor...
    Color(Mesh::Handler& handler, const index&& offset, const GenericCreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
    Color(Mesh::Handler& handler, const index&& offset, const std::string&& name, const CreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
          const MESH&& type = MESH::COLOR);

    std::vector<Vertex::Color> vertices_;
};

// **********************
//      Line
// **********************

class Line : public Color {
    friend class Mesh::Handler;

   public:
    ~Line();

   protected:
    Line(Mesh::Handler& handler, const index&& offset, const std::string&& name, const CreateInfo* pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

// TEXTURE

class Texture : public Base {
    friend class Mesh::Handler;

   public:
    ~Texture();

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) const override {
        assert(index < vertices_.size());
        return {vertices_.at(index)};
    }
    inline void addVertex(const Vertex::Complete& v, int32_t index = -1) override {
        if (index == -1) {
            vertices_.push_back(v.getVertex<Vertex::Texture>());
        } else {
            assert(index < vertices_.size());
            vertices_[index] = v.getVertex<Vertex::Texture>();
        }
        pInstObj3d_->updateBoundingBox(vertices_.back());
    }
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const override { return vertices_.size(); }
    inline vk::DeviceSize getVertexBufferSize(bool assert = false) const override {
        vk::DeviceSize bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        if (assert) assert(bufferSize == vertexRes_.memoryRequirements.size);
        return bufferSize;
    }
    const glm::vec3& getVertexPositionAtOffset(size_t offset) const override { return vertices_[offset].position; }

   protected:
    Texture(Mesh::Handler& handler, const index&& offset, const std::string&& name, const CreateInfo* pCreateInfo,
            std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);

    std::vector<Vertex::Texture> vertices_;
};

}  // namespace Mesh

#endif  // !MESH_H
