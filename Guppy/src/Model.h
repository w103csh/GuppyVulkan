
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
class MyShell;
class Scene;
class TextureMesh;

typedef uint32_t mdl_idx;

class Model : Object3d {
   public:
    // texture mesh
    // Model(std::unique_ptr<Scene> &pScene, std::string modelPath, std::string texPath = "", std::string normPath = "",
    //      std::string specPath = "", std::string materialPath = "");

    // color mesh
    Model::Model(mdl_idx handlerOffset, std::unique_ptr<Scene> &pScene, std::string modelPath, glm::mat4 model = glm::mat4(1.0f));

    inline mdl_idx getHandlerOffset() const { return handlerOffset_; }
    inline mdl_idx getSceneOffset() const { return sceneOffset_; }

    std::vector<ColorMesh *> load(MyShell *sh, Material material, std::function<void(Mesh *)> callback);

    STATUS status;

    void addOffset(std::unique_ptr<ColorMesh> &pMesh);
    //void addOffset(std::unique_ptr<LineMesh> &pMesh);
    void addOffset(std::unique_ptr<TextureMesh> &pMesh);

   private:
    mdl_idx handlerOffset_;
    std::string modelPath_;
    mdl_idx sceneOffset_;
    std::vector<mdl_idx> colorOffsets_;
    std::vector<mdl_idx> lineOffsets_;
    std::vector<mdl_idx> texOffsets_;
};

#endif  // !MODEL_H