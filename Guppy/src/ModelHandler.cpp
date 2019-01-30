
#include "ModelHandler.h"

#include "FileLoader.h"
#include "Mesh.h"
#include "Shell.h"
#include "TextureHandler.h"

Model::Handler::Handler(Game *pGame) : Game::Handler(pGame) {}

void Model::Handler::makeModel(Model::CreateInfo *pCreateInfo, std::unique_ptr<Scene::Base> &pScene) {
    if (pCreateInfo->needsTexture) {
        makeTextureModel(pCreateInfo, pScene);
    } else {
        makeColorModel(pCreateInfo, pScene);
    }
}

void Model::Handler::makeColorModel(Model::CreateInfo *pCreateInfo, std::unique_ptr<Scene::Base> &pScene) {
    // Add a new model instance
    pCreateInfo->handlerOffset = pModels_.size();
    pModels_.emplace_back(std::make_unique<Model::Base>(std::ref(*this), pCreateInfo, pScene->getOffset()));

    if (pCreateInfo->async) {
        ldgColorFutures_[pModels_.back()->getHandlerOffset()] = std::make_pair(
            std::async(std::launch::async, &Handler::loadColor, this, pCreateInfo->materialInfo, std::ref(*pModels_.back())),
            std::move(pCreateInfo->callback));
    } else {
        handleMeshes(pScene, pModels_.back(), loadColor(pCreateInfo->materialInfo, std::ref(*pModels_.back())));
        pModels_.back()->postLoad(pCreateInfo->callback);
    }
}

void Model::Handler::makeTextureModel(Model::CreateInfo *pCreateInfo, std::unique_ptr<Scene::Base> &pScene) {
    // Add a new model instance
    pCreateInfo->handlerOffset = pModels_.size();
    pModels_.emplace_back(std::make_unique<Model::Base>(std::ref(*this), pCreateInfo, pScene->getOffset()));

    if (pCreateInfo->async) {
        ldgTexFutures_[pModels_.back()->getHandlerOffset()] =
            std::make_pair(std::async(std::launch::async, &Model::Handler::loadTexture, this, pCreateInfo->materialInfo,
                                      std::ref(*pModels_.back())),
                           std::move(pCreateInfo->callback));
    } else {
        handleMeshes(pScene, pModels_.back(), loadTexture(pCreateInfo->materialInfo, std::ref(*pModels_.back())));
        pModels_.back()->postLoad(pCreateInfo->callback);
    }
}

std::vector<ColorMesh *> Model::Handler::loadColor(const Material::Info &materialInfo, Model::Base &model) {
    FileLoader::tinyobj_data data = {model.modelPath_, ""};

    // Get obj data from the file loader...
    FileLoader::getObjData(shell(), data);
    assert(data.attrib.vertices.size());

    MeshCreateInfo createInfo;

    // determine amount and type of meshes...
    std::vector<ColorMesh *> pMeshes;
    if (data.materials.empty()) {
        createInfo = {};
        createInfo.pipelineType = model.PIPELINE_TYPE;
        createInfo.materialInfo = materialInfo;
        createInfo.model = model.getData().model;

        pMeshes.push_back(new ColorMesh("", &createInfo));  // TODO: name
    } else {
        for (auto &tinyobj_mat : data.materials) {
            // Material
            Material::Info materialInfoCopy = materialInfo;
            Material::copyTinyobjData(tinyobj_mat, materialInfoCopy);
            // Mesh
            createInfo = {};
            createInfo.pipelineType = model.PIPELINE_TYPE;
            createInfo.materialInfo = materialInfoCopy;
            createInfo.model = model.getData().model;

            pMeshes.push_back(new ColorMesh("", &createInfo));  // TODO: name
        }
    }

    // Load obj data into mesh... (The map types have comparison predicates that
    // smooth or not)
    if (model.smoothNormals_) {
        FileLoader::loadObjData<unique_vertices_map_smoothing>(data, pMeshes);
    } else {
        FileLoader::loadObjData<unique_vertices_map_non_smoothing>(data, pMeshes);
    }

    for (auto &pMesh : pMeshes) assert(pMesh->getVertexCount());  // ensure something was loaded

    return pMeshes;
}

std::vector<TextureMesh *> Model::Handler::loadTexture(const Material::Info &materialInfo, Model::Base &model) {
    std::string modelDirectory = helpers::getFilePath(model.modelPath_);
    FileLoader::tinyobj_data data = {model.modelPath_, modelDirectory.c_str()};

    // Get obj data from the file loader...
    FileLoader::getObjData(shell(), data);
    assert(data.attrib.vertices.size());

    std::vector<TextureMesh *> pMeshes;

    MeshCreateInfo createInfo;

    if (!data.materials.empty()) {
        /*  If there is a mtl file and materials get texture and material data
            from data object. One mesh per material.
        */

        for (auto &tinyobj_mat : data.materials) {
            // Material
            Material::Info materialInfoCopy = materialInfo;
            Material::copyTinyobjData(tinyobj_mat, materialInfoCopy);
            // Mesh
            createInfo = {};
            createInfo.pipelineType = model.PIPELINE_TYPE;
            createInfo.materialInfo = materialInfoCopy;
            createInfo.model = model.getData().model;

            if (!tinyobj_mat.diffuse_texname.empty() /*|| !to_m.specular_texname.empty() || !to_m.bump_texname.empty()*/) {
                std::string diff, spec, norm;
                if (!tinyobj_mat.diffuse_texname.empty()) diff = modelDirectory + tinyobj_mat.diffuse_texname;

                // Check if the texture already exists (TODO: better check)
                if (textureHandler().getTextureByPath(diff) == nullptr) {
                    if (!tinyobj_mat.specular_texname.empty()) spec = modelDirectory + tinyobj_mat.specular_texname;
                    if (!tinyobj_mat.bump_texname.empty()) norm = modelDirectory + tinyobj_mat.bump_texname;
                    textureHandler().addTexture(diff, norm, spec);
                } else {
                    // TODO: deal with textures sharing some bitmap and not others...
                }

                createInfo.materialInfo.pTexture = textureHandler().getTextureByPath(diff);

                pMeshes.push_back(new TextureMesh("", &createInfo));  // TODO: name

            } else {
                // TODO: deal with material that don't have a diffuse bitmap...
            }
        }
    } else {
        createInfo = {};
        createInfo.pipelineType = model.PIPELINE_TYPE;
        createInfo.materialInfo = materialInfo;
        createInfo.model = model.getData().model;

        pMeshes.push_back(new TextureMesh("", &createInfo)); // TODO: name
    }

    // Load obj data into mesh... (The map types have comparison predicates that
    // smooth or not)
    if (model.smoothNormals_) {
        FileLoader::loadObjData<unique_vertices_map_smoothing>(data, pMeshes);
    } else {
        FileLoader::loadObjData<unique_vertices_map_non_smoothing>(data, pMeshes);
    }

    for (auto &pMesh : pMeshes) assert(pMesh->getVertexCount());  // ensure something was loaded

    return pMeshes;
}

void Model::Handler::update(std::unique_ptr<Scene::Base> &pScene) {
    checkFutures(pScene, ldgColorFutures_);
    checkFutures(pScene, ldgTexFutures_);
}
