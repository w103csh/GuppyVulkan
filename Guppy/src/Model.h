
#ifndef MODEL_H
#define MODEL_H

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Material.h"

class Scene;

class Model {
   public:
    // texture mesh
    // Model(std::unique_ptr<Scene> &pScene, std::string modelPath, std::string texPath = "", std::string normPath = "",
    //      std::string specPath = "", std::string materialPath = "");

    // color mesh
    Model(std::unique_ptr<Scene> &pScene, std::string modelPath, Material material = {}, glm::mat4 model = glm::mat4(1.0f),
          bool async = true, std::function<void(std::vector<Mesh *>)> callback = nullptr);

    STATUS status;

   private:
    std::string modelPath_;
    size_t sceneOffset_;
    std::vector<size_t> colorOffsets_;
    std::vector<size_t> texOffsets_;
    std::vector<size_t> lineOffsets_;
};

#endif  // !MODEL_H