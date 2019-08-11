
#ifndef MODEL_H
#define MODEL_H

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "Handlee.h"
#include "Helpers.h"
#include "Instance.h"
#include "Material.h"
#include "Mesh.h"
#include "Obj3d.h"

// clang-format off
namespace Mesh { class Handler; }
// clang-format on

namespace Model {

class Base;
class Handler;

using index = uint32_t;
constexpr auto INDEX_MAX = UINT32_MAX;
using cback = std::function<void(Model::Base *)>;

struct CreateInfo : public Mesh::CreateInfo {
    bool async = false;
    bool smoothNormals = false;
    bool visualHelper = false;
    float visualHelperLineSize = 0.1f;
    Model::cback callback = nullptr;
    Model::index handlerOffset = Model::INDEX_MAX;  // TODO: this should only be set by the handler...
    std::string modelPath = "";
};

class Base : public NonCopyable, public Handlee<Model::Handler>, public ObjInst3d {
    friend class Model::Handler;

   public:
    Base(Model::Handler &handler, Model::CreateInfo *pCreateInfo, std::shared_ptr<Instance::Base> &pInstanceData);
    virtual ~Base();

    const PIPELINE PIPELINE_TYPE;

    inline Model::index getOffset() const { return offset_; }

    void postLoad(Model::cback callback);

    const std::vector<Model::index> &getMeshOffsets(MESH type);

    inline Model::CreateInfo getMeshCreateInfo() {
        Model::CreateInfo createInfo = {};
        createInfo.pipelineType = PIPELINE_TYPE;
        return createInfo;
    }

    void allMeshAction(std::function<void(Mesh::Base *)> action);

    // STATUS status;

   private:
    void updateAggregateBoundingBox(Mesh::Base *pMesh);

    Model::index offset_;
    std::string modelPath_;
    bool smoothNormals_;
    bool visualHelper_;
    float visualHelperLineSize_;

    void addOffset(std::unique_ptr<Mesh::Color> &pMesh);
    void addOffset(std::unique_ptr<Mesh::Line> &pMesh);
    void addOffset(std::unique_ptr<Mesh::Texture> &pMesh);

    std::vector<Model::index> colorOffsets_;
    std::vector<Model::index> lineOffsets_;
    std::vector<Model::index> texOffsets_;
};

}  // namespace Model

#endif  // !MODEL_H