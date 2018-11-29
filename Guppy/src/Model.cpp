
#include "FileLoader.h"
#include "Material.h"
#include "Mesh.h"
#include "ModelHandler.h"
#include "Model.h"
#include "Object3d.h"
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

Model::Model(mdl_idx handlerOffset, std::unique_ptr<Scene> &pScene, std::string modelPath, glm::mat4 model)
    : Object3d(model),
      status(STATUS::PENDING),
      handlerOffset_(handlerOffset),
      modelPath_(modelPath),
      sceneOffset_(pScene->getOffset()) {}

std::vector<ColorMesh *> Model::load(MyShell *sh, Material material, std::function<void(Mesh *)> callback) {
    FileLoader::tinyobj_data data = {modelPath_, ""};

    // Get obj data from the file loader...
    FileLoader::getObjData(sh, data);
    assert(data.attrib.vertices.size());

    // determine amount and type of meshes...
    std::vector<ColorMesh *> pMeshes;
    if (data.materials.empty()) {
        pMeshes.push_back(new ColorMesh(std::make_unique<Material>(material), model_));
    } else {
        for (auto &m : data.materials) {
            auto pMaterial = std::make_unique<Material>(material);
            pMaterial->setMaterialData(m);
            pMeshes.push_back(new ColorMesh(std::move(pMaterial), model_));
        }
    }

    // Load obj data into mesh...
    FileLoader::loadObjData(data, pMeshes);

    // Invoke callback for say... model transformations...
    if (callback)
        for (auto &pMesh : pMeshes) callback(pMesh);

    return pMeshes;
}


void Model::addOffset(std::unique_ptr<ColorMesh> &pMesh) { colorOffsets_.push_back(pMesh->getOffset()); }
// void addOffset(std::unique_ptr<LineMesh> &pMesh) { lineOffsets_.push_back(pMesh->getOffset()); }
void Model::addOffset(std::unique_ptr<TextureMesh> &pMesh) { texOffsets_.push_back(pMesh->getOffset()); }