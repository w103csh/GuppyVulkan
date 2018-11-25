
#include "FileLoader.h"
#include "Material.h"
#include "Mesh.h"
#include "ModelHandler.h"
#include "Model.h"
#include "Scene.h"

// Model::Model(MyShell *sh, std::unique_ptr<Scene> &pScene, std::string modelPath, std::string texPath = "",
//             std::string normPath = "", std::string specPath = "", std::string materialPath = "")
//    : sceneOffset_(pScene->getOffset()) {
//    // do something
//    FileLoader::tinyobj_data data = {modelPath, helpers::getFilePath(modelPath)};
//    FileLoader::getObjData(sh, data);
//
//    if (!data.materials.empty()) {
//        /*  If there is a mtl file and materials get texture and material data
//            from data object. One mesh per material.
//        */
//
//        for (auto &m : data.materials) {
//        }
//    } else {
//        /*  If there is not an mtl file with materials we can use create texture,
//            and material based on parameters.
//        */
//
//        // Create a mesh type based on parameters
//        if (!texPath.empty()) {
//            // auto pMesh =
//        } else {
//        }
//    }
//}

Model::Model(std::unique_ptr<Scene> &pScene, std::string modelPath, Material material, glm::mat4 model, bool async,
             std::function<void(std::vector<Mesh *>)> callback)
    : status(STATUS::PENDING), modelPath_(modelPath), sceneOffset_(pScene->getOffset()) {
    FileLoader::tinyobj_data data = {modelPath_, ""};

    auto lambda = [&pScene, &material, &model, &callback, &data, this](MyShell *sh) {
        // Get obj data from the file loader...
        FileLoader::getObjData(sh, data);

        // determine amount and type of meshes...
        std::vector<ColorMesh *> pMeshes;
        if (data.materials.empty()) {
            pMeshes.push_back(new ColorMesh(std::make_unique<Material>(material), model));
        } else {
            for (auto &m : data.materials) {
                auto pMaterial = std::make_unique<Material>(material);
                pMaterial->setMaterialData(m);
                pMeshes.push_back(new ColorMesh(std::move(pMaterial), model));
            }
        }

        // Load obj data into mesh...
        FileLoader::loadObjData(data, pMeshes);

        for (auto &pMesh : pMeshes) {
            pMesh->setStatusReady();
            colorOffsets_.push_back(pScene->moveMesh(sh->context(), std::unique_ptr<ColorMesh>(pMesh)));
        }

        // callback(pMeshes);
    };

    ModelHandler::loadModel(lambda, async);
}