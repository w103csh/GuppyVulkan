
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
class Mesh;
class MyShell;
class Scene;
class TextureMesh;

class Model : public Object3d {
   public:
    typedef uint32_t IDX;
    typedef std::function<void(Model *)> CALLBK;

    Model::Model(Model::IDX handlerOffset, std::unique_ptr<Scene> &pScene, std::string modelPath,
                 glm::mat4 model = glm::mat4(1.0f));

    inline Model::IDX getHandlerOffset() const { return handlerOffset_; }
    inline Model::IDX getSceneOffset() const { return sceneOffset_; }

    std::vector<ColorMesh *> loadColor(MyShell *sh, Material material);
    std::vector<TextureMesh *> loadTexture(MyShell *sh, Material material, std::shared_ptr<Texture::Data> pTexture);

    void postLoad(Model::CALLBK callback);

    virtual inline void transform(const glm::mat4 t) override;
    // void updateAggregateBoundingBox(std::unique_ptr<Scene> &pScene);

    STATUS status;

    void addOffset(std::unique_ptr<ColorMesh> &pMesh);
    // void addOffset(std::unique_ptr<LineMesh> &pMesh);
    void addOffset(std::unique_ptr<TextureMesh> &pMesh);

   private:
    void allMeshAction(std::function<void(Mesh *)> action);
    void updateAggregateBoundingBox(Mesh *pMesh);

    Model::IDX handlerOffset_;
    std::string modelPath_;
    Model::IDX sceneOffset_;
    std::vector<Model::IDX> colorOffsets_;
    std::vector<Model::IDX> lineOffsets_;
    std::vector<Model::IDX> texOffsets_;
};

#endif  // !MODEL_H