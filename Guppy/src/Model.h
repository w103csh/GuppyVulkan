
#ifndef MODEL_H
#define MODEL_H

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "FileLoader.h"
#include "Helpers.h"
#include "Material.h"
#include "Object3d.h"

class ColorMesh;
class LineMesh;
class Mesh;
class Model;
class Scene;
class Shell;
class TextureMesh;

typedef uint32_t MODEL_INDEX;
#define MODEL_INDEX_MAX UINT32_MAX
typedef std::function<void(Model *)> MODEL_CALLBACK;

typedef struct ModelCreateInfo {
    bool async = false;
    bool needsTexture = false;
    bool smoothNormals = false;
    bool visualHelper = false;
    MODEL_CALLBACK callback = nullptr;
    MODEL_INDEX handlerOffset = MODEL_INDEX_MAX;  // TODO: this should only be set by the handler...
    Material::Info materialInfo = {};
    glm::mat4 model = glm::mat4(1.0f);
    std::string modelPath = "";
    PIPELINE pipelineType = PIPELINE::DONT_CARE;
} ModelCreateInfo;

class Model : public Object3d {
    friend class ModelHandler;

   public:
    Model::Model(ModelCreateInfo *pCreateInfo, MODEL_INDEX sceneOffset);

    const PIPELINE PIPELINE_TYPE;

    inline MODEL_INDEX getHandlerOffset() const { return handlerOffset_; }
    inline MODEL_INDEX getSceneOffset() const { return sceneOffset_; }

    std::vector<ColorMesh *> loadColor(Shell *sh, const Material::Info &materialInfo);
    std::vector<TextureMesh *> loadTexture(Shell *sh, const Material::Info &materialInfo);

    void postLoad(MODEL_CALLBACK callback);

    virtual inline void transform(const glm::mat4 t) override;
    // void updateAggregateBoundingBox(std::unique_ptr<Scene> &pScene);

    // TODO: use a conditional template argument
    MODEL_INDEX getMeshOffset(MESH type, uint8_t offset);

    STATUS status;

   private:
    void allMeshAction(std::function<void(Mesh *)> action);
    void updateAggregateBoundingBox(Mesh *pMesh);

    MODEL_INDEX handlerOffset_;
    std::string modelPath_;
    MODEL_INDEX sceneOffset_;
    bool smoothNormals_;
    bool visualHelper_;

    void addOffset(std::unique_ptr<ColorMesh> &pMesh);
    void addOffset(std::unique_ptr<LineMesh> &pMesh);
    void addOffset(std::unique_ptr<TextureMesh> &pMesh);

    std::vector<MODEL_INDEX> colorOffsets_;
    std::vector<MODEL_INDEX> lineOffsets_;
    std::vector<MODEL_INDEX> texOffsets_;
};

#endif  // !MODEL_H