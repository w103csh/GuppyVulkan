
#ifndef MODEL_H
#define MODEL_H

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "Handlee.h"
#include "Helpers.h"
#include "Material.h"
#include "Mesh.h"
#include "Object3d.h"

// clang-format off
namespace Mesh { class Handler; }
// clang-format on

namespace Model {

class Base;
class Handler;

typedef uint32_t INDEX;
constexpr auto INDEX_MAX = UINT32_MAX;
typedef std::function<void(Model::Base *)> CBACK;

struct CreateInfo : public Mesh::CreateInfo {
    bool async = false;
    bool smoothNormals = false;
    bool visualHelper = false;
    float visualHelperLineSize = 0.1f;
    Model::CBACK callback = nullptr;
    Model::INDEX handlerOffset = Model::INDEX_MAX;  // TODO: this should only be set by the handler...
    std::string modelPath = "";
};

class Base : public Object3d, public Handlee<Model::Handler> {
    friend class Model::Handler;

   public:
    Base(Model::Handler &handler, Model::CreateInfo *pCreateInfo, const std::vector<glm::mat4> &&models);
    virtual ~Base();

    const PIPELINE PIPELINE_TYPE;

    inline Model::INDEX getOffset() const { return offset_; }

    void postLoad(Model::CBACK callback);

    // OBJECT3D
    virtual uint32_t getModelCount() { return static_cast<uint32_t>(models_.size()); }
    virtual inline void transform(const glm::mat4 t, uint32_t index = 0) override;

    // TODO: use a conditional template argument
    Model::INDEX getMeshOffset(MESH type, uint8_t offset);

    inline Model::CreateInfo getMeshCreateInfo() {
        Model::CreateInfo createInfo = {};
        createInfo.pipelineType = PIPELINE_TYPE;
        return createInfo;
    }

    void allMeshAction(std::function<void(Mesh::Base *)> action);

    // STATUS status;

   private:
    // OBJECT3D
    inline const glm::mat4 &model(uint32_t index = 0) const override { return models_[index]; }
    std::vector<glm::mat4> models_;

    void updateAggregateBoundingBox(Mesh::Base *pMesh);

    Model::INDEX offset_;
    std::string modelPath_;
    bool smoothNormals_;
    bool visualHelper_;
    float visualHelperLineSize_;

    void addOffset(std::unique_ptr<Mesh::Color> &pMesh);
    void addOffset(std::unique_ptr<Mesh::Line> &pMesh);
    void addOffset(std::unique_ptr<Mesh::Texture> &pMesh);

    std::vector<Model::INDEX> colorOffsets_;
    std::vector<Model::INDEX> lineOffsets_;
    std::vector<Model::INDEX> texOffsets_;
};

}  // namespace Model

#endif  // !MODEL_H