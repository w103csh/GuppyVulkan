
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
            // See if the texture already exists...
            pCreateInfo->pTexture = textureHandler().getTextureByName(tinyobj_mat.name);

            if (pCreateInfo->pTexture != nullptr) return;

            // For now its just easier to make separate samplers instead, of layering
            // everything.
            Texture::CreateInfo texCreateInfo = {
                tinyobj_mat.name + " Texture",
                {{tinyobj_mat.name + " Sampler", {}, VK_IMAGE_VIEW_TYPE_2D_ARRAY}},
            };

            // DIFFUSE
            if (!tinyobj_mat.diffuse_texname.empty()) {
                texCreateInfo.samplerCreateInfos.back().layerInfos.push_back(Sampler::GetDef4Comb3And1LayerInfo(
                    Sampler::TYPE::COLOR,         //
                    modelDirectory,               //
                    tinyobj_mat.diffuse_texname,  //
                    tinyobj_mat.alpha_texname     //
                    ));
            }
            // NORMAL
            if (!tinyobj_mat.bump_texname.empty()) {
                texCreateInfo.samplerCreateInfos.back().layerInfos.push_back(Sampler::GetDef4Comb3And1LayerInfo(
                    Sampler::TYPE::NORMAL,    //
                    modelDirectory,           //
                    tinyobj_mat.bump_texname  //
                                              // TODO: what should this be?
                    ));
            }
            // SPECULAR
            if (!tinyobj_mat.specular_texname.empty()) {
                texCreateInfo.samplerCreateInfos.back().layerInfos.push_back(Sampler::GetDef4Comb3And1LayerInfo(
                    Sampler::TYPE::SPECULAR,      //
                    modelDirectory,               //
                    tinyobj_mat.specular_texname  //
                                                  // TODO: what should this be?
                    ));
            }

            // TODO: there are a bunch of combinations not accounted for.

            pCreateInfo->pTexture = textureHandler().make(&texCreateInfo);

        } else {
            // TODO: deal with material that don't have a diffuse bitmap...
        }
    }
}
