
#ifndef MODEL_H
#define MODEL_H

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "FileLoader.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Material.h"
#include "Object3d.h"

class ColorMesh;
class LineMesh;
class Mesh;
class TextureMesh;

namespace Model {

class Base;
class Handler;

typedef uint32_t INDEX;
constexpr auto INDEX_MAX = UINT32_MAX;
typedef std::function<void(Model::Base *)> CBACK;

typedef struct CreateInfo {
    bool async = false;
    bool needsTexture = false;
    bool smoothNormals = false;
    bool visualHelper = false;
    Model::CBACK callback = nullptr;
    Model::INDEX handlerOffset = Model::INDEX_MAX;  // TODO: this should only be set by the handler...
    Material::Info materialInfo = {};
    glm::mat4 model = glm::mat4(1.0f);
    std::string modelPath = "";
    PIPELINE pipelineType = PIPELINE::DONT_CARE;
} CreateInfo;

class Base : public Object3d, public Handlee<Model::Handler> {
    friend class Model::Handler;

   public:
    Base(const Model::Handler &handler, Model::CreateInfo *pCreateInfo, Model::INDEX sceneOffset);
    virtual ~Base();

    const PIPELINE PIPELINE_TYPE;

    inline Model::INDEX getHandlerOffset() const { return handlerOffset_; }
    inline Model::INDEX getSceneOffset() const { return sceneOffset_; }

    void postLoad(Model::CBACK callback);

    virtual inline void transform(const glm::mat4 t) override;
    // void updateAggregateBoundingBox(std::unique_ptr<Scene> &pScene);

    // TODO: use a conditional template argument
    Model::INDEX getMeshOffset(MESH type, uint8_t offset);

    STATUS status;

   private:
    void allMeshAction(std::function<void(Mesh *)> action);
    void updateAggregateBoundingBox(Mesh *pMesh);

    Model::INDEX handlerOffset_;
    std::string modelPath_;
    Model::INDEX sceneOffset_;
    bool smoothNormals_;
    bool visualHelper_;

    void addOffset(std::unique_ptr<ColorMesh> &pMesh);
    void addOffset(std::unique_ptr<LineMesh> &pMesh);
    void addOffset(std::unique_ptr<TextureMesh> &pMesh);

    std::vector<Model::INDEX> colorOffsets_;
    std::vector<Model::INDEX> lineOffsets_;
    std::vector<Model::INDEX> texOffsets_;
};

}  // namespace Model

#endif  // !MODEL_H