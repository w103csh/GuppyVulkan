
#include "ModelHandler.h"

#include "Mesh.h"
#include "Shell.h"
#include "TextureHandler.h"

Model::Handler::Handler(Game *pGame) : Game::Handler(pGame) {}

void Model::Handler::update(std::unique_ptr<Scene::Base> &pScene) {
    checkFutures(pScene, ldgColorFutures_);
    checkFutures(pScene, ldgTexFutures_);
}

FileLoader::tinyobj_data Model::Handler::loadData(Model::Base &model) {
    // Get .obj data from the file loader.
    std::string modelDirectory = helpers::getFilePath(model.modelPath_);
    FileLoader::tinyobj_data data = {model.modelPath_, modelDirectory.c_str()};
    FileLoader::getObjData(shell(), data);
    assert(data.attrib.vertices.size());
    return data;
}

void Model::Handler::makeTexture(const tinyobj::material_t &tinyobj_mat, const std::string &modelDirectory,
                                 Material::CreateInfo *pCreateInfo) {
    // Determine what type of textures if any need to be made.
    bool hasTexture = pCreateInfo->pTexture != nullptr;
    // If there is a texture ignore .obj materials for now. (TODO: more dynamic)
    if (!hasTexture) {
        if (!tinyobj_mat.diffuse_texname.empty() /*|| !to_m.specular_texname.empty() || !to_m.bump_texname.empty()*/) {
            std::string diff, spec, norm, alph;
            if (!tinyobj_mat.diffuse_texname.empty()) diff = modelDirectory + tinyobj_mat.diffuse_texname;

            // Check if the texture already exists (TODO: better check)
            if (textureHandler().getTextureByPath(diff) == nullptr) {
                if (!tinyobj_mat.specular_texname.empty()) spec = modelDirectory + tinyobj_mat.specular_texname;
                if (!tinyobj_mat.bump_texname.empty()) norm = modelDirectory + tinyobj_mat.bump_texname;
                if (!tinyobj_mat.alpha_texname.empty()) alph = modelDirectory + tinyobj_mat.alpha_texname;
                textureHandler().addTexture(diff, norm, spec, alph);
            } else {
                // TODO: deal with textures sharing some bitmap and not others...
            }
            pCreateInfo->pTexture = textureHandler().getTextureByPath(diff);
        } else {
            // TODO: deal with material that don't have a diffuse bitmap...
        }
    }
}
