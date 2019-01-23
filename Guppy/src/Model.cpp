
#include "FileLoader.h"
#include "Material.h"
#include "Mesh.h"
#include "ModelHandler.h"
#include "Model.h"
#include "Object3d.h"
#include "Scene.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

Model::Model(ModelCreateInfo *pCreateInfo, MODEL_INDEX sceneOffset)
    : Object3d(pCreateInfo->model),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      handlerOffset_(pCreateInfo->handlerOffset),
      modelPath_(pCreateInfo->modelPath),
      sceneOffset_(sceneOffset),
      smoothNormals_(pCreateInfo->smoothNormals),
      visualHelper_(pCreateInfo->visualHelper) {
    assert(sceneOffset_ < MODEL_INDEX_MAX);
}

std::vector<ColorMesh *> Model::loadColor(Shell *sh, const Material::Info &materialInfo) {
    FileLoader::tinyobj_data data = {modelPath_, ""};

    // Get obj data from the file loader...
    FileLoader::getObjData(sh, data);
    assert(data.attrib.vertices.size());

    MeshCreateInfo createInfo;

    // determine amount and type of meshes...
    std::vector<ColorMesh *> pMeshes;
    if (data.materials.empty()) {
        createInfo = {};
        createInfo.pipelineType = PIPELINE_TYPE;
        createInfo.materialInfo = materialInfo;
        createInfo.model = model_;

        pMeshes.push_back(new ColorMesh(&createInfo));
    } else {
        for (auto &tinyobj_mat : data.materials) {
            // Material
            Material::Info materialInfoCopy = materialInfo;
            Material::copyTinyobjData(tinyobj_mat, materialInfoCopy);
            // Mesh
            createInfo = {};
            createInfo.pipelineType = PIPELINE_TYPE;
            createInfo.materialInfo = materialInfoCopy;
            createInfo.model = model_;

            pMeshes.push_back(new ColorMesh(&createInfo));
        }
    }

    // Load obj data into mesh... (The map types have comparison predicates that
    // smooth or not)
    if (smoothNormals_) {
        FileLoader::loadObjData<unique_vertices_map_smoothing>(data, pMeshes);
    } else {
        FileLoader::loadObjData<unique_vertices_map_non_smoothing>(data, pMeshes);
    }

    for (auto &pMesh : pMeshes) assert(pMesh->getVertexCount());  // ensure something was loaded

    return pMeshes;
}

std::vector<TextureMesh *> Model::loadTexture(Shell *sh, const Material::Info &materialInfo) {
    std::string modelDirectory = helpers::getFilePath(modelPath_);
    FileLoader::tinyobj_data data = {modelPath_, modelDirectory.c_str()};

    // Get obj data from the file loader...
    FileLoader::getObjData(sh, data);
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
            createInfo.pipelineType = PIPELINE_TYPE;
            createInfo.materialInfo = materialInfoCopy;
            createInfo.model = model_;

            if (!tinyobj_mat.diffuse_texname.empty() /*|| !to_m.specular_texname.empty() || !to_m.bump_texname.empty()*/) {
                std::string diff, spec, norm;
                if (!tinyobj_mat.diffuse_texname.empty()) diff = modelDirectory + tinyobj_mat.diffuse_texname;

                // Check if the texture already exists (TODO: better check)
                if (!TextureHandler::getTextureByPath(diff)) {
                    if (!tinyobj_mat.specular_texname.empty()) spec = modelDirectory + tinyobj_mat.specular_texname;
                    if (!tinyobj_mat.bump_texname.empty()) norm = modelDirectory + tinyobj_mat.bump_texname;
                    TextureHandler::addTexture(diff, norm, spec);
                } else {
                    // TODO: deal with textures sharing some bitmap and not others...
                }

                createInfo.materialInfo.pTexture = TextureHandler::getTextureByPath(diff);

                pMeshes.push_back(new TextureMesh(&createInfo));

            } else {
                // TODO: deal with material that don't have a diffuse bitmap...
            }
        }
    } else {
        createInfo = {};
        createInfo.pipelineType = PIPELINE_TYPE;
        createInfo.materialInfo = materialInfo;
        createInfo.model = model_;

        pMeshes.push_back(new TextureMesh(&createInfo));
    }

    // Load obj data into mesh... (The map types have comparison predicates that
    // smooth or not)
    if (smoothNormals_) {
        FileLoader::loadObjData<unique_vertices_map_smoothing>(data, pMeshes);
    } else {
        FileLoader::loadObjData<unique_vertices_map_non_smoothing>(data, pMeshes);
    }

    for (auto &pMesh : pMeshes) assert(pMesh->getVertexCount());  // ensure something was loaded

    return pMeshes;
}

void Model::postLoad(MODEL_CALLBACK callback) {
    allMeshAction([this](Mesh *pMesh) { updateAggregateBoundingBox(pMesh); });
    // Invoke callback for say... model transformations. This is a pain in the ass.
    if (callback) callback(this);
}

void Model::addOffset(std::unique_ptr<ColorMesh> &pMesh) { colorOffsets_.push_back(pMesh->getOffset()); }
void Model::addOffset(std::unique_ptr<LineMesh> &pMesh) { lineOffsets_.push_back(pMesh->getOffset()); }
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

MODEL_INDEX Model::getMeshOffset(MESH type, uint8_t offset) {
    switch (type) {
        case MESH::COLOR:
            assert(offset < colorOffsets_.size());
            return colorOffsets_[offset];
        case MESH::LINE:
            assert(offset < lineOffsets_.size());
            return lineOffsets_[offset];
        case MESH::TEXTURE:
            assert(offset < texOffsets_.size());
            return texOffsets_[offset];
        default:
            throw std::runtime_error("Unhandled mesh type.");
    }
}

void Model::updateAggregateBoundingBox(Mesh *pMesh) { updateBoundingBox(pMesh->getBoundingBoxMinMax(false)); }
