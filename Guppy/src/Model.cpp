
#include "FileLoader.h"
#include "Material.h"
#include "Mesh.h"
#include "ModelHandler.h"
#include "Model.h"
#include "Object3d.h"
#include "Scene.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

Model::Model(Model::IDX handlerOffset, std::unique_ptr<Scene> &pScene, std::string modelPath, glm::mat4 model)
    : Object3d(model),
      status(STATUS::PENDING),
      handlerOffset_(handlerOffset),
      modelPath_(modelPath),
      sceneOffset_(pScene->getOffset()) {}

std::vector<ColorMesh *> Model::loadColor(Shell *sh, Material material) {
    FileLoader::tinyobj_data data = {modelPath_, ""};

    // Get obj data from the file loader...
    FileLoader::getObjData(sh, data);
    assert(data.attrib.vertices.size());

    // determine amount and type of meshes...
    std::vector<ColorMesh *> pMeshes;
    if (data.materials.empty()) {
        pMeshes.push_back(new ColorMesh(std::make_unique<Material>(material), model_));
    } else {
        for (auto &to_m : data.materials) {
            auto pMaterial = std::make_unique<Material>(material);
            pMaterial->setMaterialData(to_m);
            pMeshes.push_back(new ColorMesh(std::move(pMaterial), model_));
        }
    }

    // Load obj data into mesh...
    FileLoader::loadObjData(data, pMeshes);
    for (auto &pMesh : pMeshes) assert(pMesh->getVertexCount());

    return pMeshes;
}

std::vector<TextureMesh *> Model::loadTexture(Shell *sh, Material material, std::shared_ptr<Texture::Data> pTexture) {
    std::string modelDirectory = helpers::getFilePath(modelPath_);
    FileLoader::tinyobj_data data = {modelPath_, modelDirectory.c_str()};

    // Get obj data from the file loader...
    FileLoader::getObjData(sh, data);
    assert(data.attrib.vertices.size());

    std::vector<TextureMesh *> pMeshes;

    if (!data.materials.empty()) {
        /*  If there is a mtl file and materials get texture and material data
            from data object. One mesh per material.
        */

        for (auto &to_m : data.materials) {
            auto pMaterial = std::make_unique<Material>(material);
            pMaterial->setMaterialData(to_m);

            if (!to_m.diffuse_texname.empty() /*|| !to_m.specular_texname.empty() || !to_m.bump_texname.empty()*/) {
                std::string diff, spec, norm;
                if (!to_m.diffuse_texname.empty()) diff = modelDirectory + to_m.diffuse_texname;

                // Check if the texture already exists (TODO: better check)
                if (!TextureHandler::getTextureByPath(diff)) {
                    if (!to_m.specular_texname.empty()) spec = modelDirectory + to_m.specular_texname;
                    if (!to_m.bump_texname.empty()) norm = modelDirectory + to_m.bump_texname;
                    TextureHandler::addTexture(diff, spec, norm);
                } else {
                    // TODO: deal with textures sharing some bitmap and not others...
                }

                pMaterial->setTexture(TextureHandler::getTextureByPath(diff));
                pMeshes.push_back(new TextureMesh(std::move(pMaterial), model_));

            } else {
                // TODO: deal with material that don't have a diffuse bitmap...
            }
        }
    } else {
        auto pMaterial = std::make_unique<Material>(material);
        pMaterial->setTexture(pTexture);
        pMeshes.push_back(new TextureMesh(std::move(pMaterial), model_));
    }
    // Load obj data into mesh...
    FileLoader::loadObjData(data, pMeshes);
    for (auto &pMesh : pMeshes) assert(pMesh->getVertexCount());

    return pMeshes;
}

void Model::postLoad(Model::CALLBK callback) {
    allMeshAction([this](Mesh *pMesh) { updateAggregateBoundingBox(pMesh); });
    // Invoke callback for say... model transformations. This is a pain in the ass.
    if (callback) callback(this);
}

void Model::addOffset(std::unique_ptr<ColorMesh> &pMesh) { colorOffsets_.push_back(pMesh->getOffset()); }
// void addOffset(std::unique_ptr<LineMesh> &pMesh) { lineOffsets_.push_back(pMesh->getOffset()); }
void Model::addOffset(std::unique_ptr<TextureMesh> &pMesh) { texOffsets_.push_back(pMesh->getOffset()); }

void Model::allMeshAction(std::function<void(Mesh *)> action) {
    for (auto &offset : colorOffsets_) {
        auto &pMesh = SceneHandler::getColorMesh(sceneOffset_, offset);
        action(pMesh.get());
    }
    for (auto &offset : lineOffsets_) {
        auto &pMesh = SceneHandler::getLineMesh(sceneOffset_, offset);
        action(pMesh.get());
    }
    for (auto &offset : texOffsets_) {
        auto &pMesh = SceneHandler::getTextureMesh(sceneOffset_, offset);
        action(pMesh.get());
    }
}

void Model::transform(const glm::mat4 t) {
    Object3d::transform(t);
    allMeshAction([&t](Mesh *pMesh) { pMesh->transform(t); });
}

void Model::updateAggregateBoundingBox(Mesh *pMesh) { updateBoundingBox(pMesh->getBoundingBoxMinMax(false)); }
