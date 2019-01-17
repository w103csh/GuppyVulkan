
#include <sstream>
#include <unordered_set>

#include "ShaderHandler.h"

#include "Camera.h"
#include "FileLoader.h"
#include "InputHandler.h"
#include "Scene.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "util.hpp"

// **********************
//      Shader::Handler
// **********************

Shader::Handler Shader::Handler::inst_;
Shader::Handler::Handler()
    :  // UNIFORMS
      pDefaultUniform_(std::make_unique<Uniform::Default>()),
      pDefaultDynamicUniform_(std::make_unique<Uniform::DefaultDynamic>()),
      // SHADERS
      pUtilFragShader_(std::make_unique<UtilityFragment>()),
      pDefColorVertShader_(std::make_unique<DefaultColorVertex>()),
      pDefColorFragShader_(std::make_unique<DefaultColorFragment>()),
      pDefLineFragShader_(std::make_unique<DefaultLineFragment>()),
      pDefTexVertShader_(std::make_unique<DefaultTextureVertex>()),
      pDefTexFragShader_(std::make_unique<DefaultTextureFragment>()) {}

void Shader::Handler::init(Shell& sh, const Game::Settings& settings, const Camera& camera) {
    inst_.reset();

    inst_.ctx_ = sh.context();
    inst_.settings_ = settings;

    // UNIFORMS
    inst_.pDefaultUniform_->init(inst_.ctx_.dev, settings, camera);
    inst_.pDefaultDynamicUniform_->init(inst_.ctx_, settings, TEXTURE_LIMIT);

    // LINK SHADERS
    inst_.pUtilFragShader_->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);

    init_glslang();

    // MAIN SHADERS
    inst_.pDefColorVertShader_->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);
    inst_.pDefLineFragShader_->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);
    inst_.pDefTexVertShader_->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);
    inst_.pDefColorFragShader_->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);
    inst_.pDefTexFragShader_->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);

    finalize_glslang();

    // listen to changes to the shader files
    if (settings.enable_directory_listener) {
        sh.watchDirectory(BASE_DIRNAME, &Shader::Handler::recompileShader);
    }
}

void Shader::Handler::reset() {
    // SHADERS
    pDefColorVertShader_->destroy(inst_.ctx_.dev);
    pDefColorFragShader_->destroy(inst_.ctx_.dev);
    pDefLineFragShader_->destroy(inst_.ctx_.dev);
    pDefTexVertShader_->destroy(inst_.ctx_.dev);
    pDefTexFragShader_->destroy(inst_.ctx_.dev);

    inst_.cleanup();

    // UNIFORMS
    pDefaultUniform_->destroy(ctx_.dev);
    pDefaultDynamicUniform_->destroy(ctx_.dev);
}

std::unique_ptr<Shader::Base>& Shader::Handler::getShader(const SHADER_TYPE& type) {
    switch (type) {
        // LINKS
        case SHADER_TYPE::UTIL_FRAG:
            return inst_.pUtilFragShader_;
        // DEFAULTS
        case SHADER_TYPE::COLOR_VERT:
            return inst_.pDefColorVertShader_;
        case SHADER_TYPE::TEX_VERT:
            return inst_.pDefTexVertShader_;
        case SHADER_TYPE::COLOR_FRAG:
            return inst_.pDefColorFragShader_;
        case SHADER_TYPE::TEX_FRAG:
            return inst_.pDefTexFragShader_;
        case SHADER_TYPE::LINE_FRAG:
            return inst_.pDefLineFragShader_;
        default:
            throw std::runtime_error("Unhandled shader type.");
    }
}

std::unique_ptr<Shader::Base>& Shader::Handler::getShader(SHADER_TYPE&& type) {
    return getShader(std::forward<const SHADER_TYPE&>(type));
}

void Shader::Handler::getStagesInfo(std::vector<SHADER_TYPE>&& types,
                                    std::vector<VkPipelineShaderStageCreateInfo>& stagesInfo) {
    for (auto& type : types) stagesInfo.push_back(inst_.getShader(type)->info);
}

void Shader::Handler::update(std::unique_ptr<Scene>& pScene) {
    for (; !inst_.updateQueue_.empty(); inst_.updateQueue_.pop()) {
        auto type = inst_.updateQueue_.front();
        // Create any new pipelines that have need an updated shader module.
        const auto& pipeline = Pipeline::Handler::createPipeline(type);
        // Notify scenes that the old pipeline is going to be invalid...
        SceneHandler::updatePipelineReferences(type, pipeline);
    }
}

void Shader::Handler::recompileShader(std::string fileName) {
    bool assert = inst_.settings_.assert_on_recompile_shader;
    if (fileName == UTIL_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::COLOR_FRAG, SHADER_TYPE::TEX_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_COLOR);
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_TEX);
        };

    } else if (fileName == DEFAULT_COLOR_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::COLOR_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_COLOR);
        }

    } else if (fileName == DEFAULT_TEX_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::TEX_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_TEX);
        }

    } else if (fileName == DEFAULT_LINE_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::LINE_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::LINE);
        }

    } else if (fileName == DEFAULT_COLOR_VERT_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::COLOR_VERT}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_COLOR);
        }

    } else if (fileName == DEFAULT_TEX_VERT_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::TEX_VERT}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_TEX);
        }
    }
}

void Shader::Handler::appendLinkTexts(Shader::Base* pShader, std::vector<const char*>& texts) {
    for (auto& type : pShader->LINK_TYPES) texts.push_back(getShader(type)->loadText(false));
}

bool Shader::Handler::loadShaders(std::vector<SHADER_TYPE> types, bool doAssert, bool load) {
    // For cleanup
    auto numOldResources = inst_.oldModules_.size();

    // Load link shaders
    if (load) {
        std::unordered_set<SHADER_TYPE> linkTypes;
        // Gather unique list of link shaders...
        for (auto& type : types) {
            for (auto& linkType : getShader(type)->LINK_TYPES) linkTypes.insert(linkType);
        }
        // Only load from file once.
        for (auto it = linkTypes.begin(); it != linkTypes.end(); ++it) getShader((*it))->loadText(load);
    }

    init_glslang();

    for (auto& type : types) getShader(type)->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_, load, doAssert);

    finalize_glslang();

    return numOldResources < inst_.oldModules_.size();
}

void Shader::Handler::updateDescriptorSets(const std::set<DESCRIPTOR_TYPE> types, std::vector<VkDescriptorSet>& sets,
                                           uint32_t setCount, const std::shared_ptr<Texture::Data>& pTexture) {
    std::vector<VkWriteDescriptorSet> writes;
    for (uint32_t i = 0; i < setCount; i++) {
        for (const auto& type : types) {
            switch (type) {
                case DESCRIPTOR_TYPE::DEFAULT_UNIFORM: {
                    writes.push_back(inst_.pDefaultUniform_->getWrite());
                    writes.back().dstSet = sets[i];
                } break;
                case DESCRIPTOR_TYPE::DEFAULT_DYNAMIC_UNIFORM: {
                    writes.push_back(inst_.pDefaultDynamicUniform_->getWrite());
                    writes.back().dstSet = sets[i];
                } break;
                case DESCRIPTOR_TYPE::DEFAULT_SAMPLER: {
                    writes.push_back(TextureHandler::getDescriptorWrite(type));
                    writes.back().dstSet = sets[i];
                    writes.back().pImageInfo = &pTexture->imgDescInfo;
                } break;
            }
        }
        vkUpdateDescriptorSets(inst_.ctx_.dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        writes.clear();
    }
}

// TODO: This was done hastily and needs to be redone
std::vector<uint32_t> Shader::Handler::getDynamicOffsets(const std::set<DESCRIPTOR_TYPE> types) {
    for (const auto& type : types) {
        switch (type) {
            case DESCRIPTOR_TYPE::DEFAULT_DYNAMIC_UNIFORM:
                return {static_cast<uint32_t>(inst_.pDefaultDynamicUniform_->getInfo()->range)};
        }
    }
    return {};
}

void Shader::Handler::cleanup() {
    for (auto& module : inst_.oldModules_) vkDestroyShaderModule(inst_.ctx_.dev, module, nullptr);
    inst_.oldModules_.clear();
}

void Shader::Handler::lightMacroReplace(std::string&& macroVariable, uint16_t&& numLights, std::string& text) {
    // TODO: fix the regex so that you don't have to read from file...
    std::stringstream ss;
    ss << "#define " << macroVariable << " ";
    text = textReplace(text, ss.str(), "", 0, std::forward<uint16_t&&>(numLights));
}

template <typename T1, typename T2>
std::string Shader::Handler::textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2) {
    std::string s = text;
    std::stringstream rss, nss;
    rss << s1 << r1 << s2;
    nss << s1 << r2 << s2;
    size_t f = s.find(rss.str());
    if (f != std::string::npos) s.replace(f, rss.str().length(), nss.str());
    return s;
}

// **********************
//      Uniform
// **********************

void Shader::Handler::updateDefaultUniform(Camera& camera) { inst_.pDefaultUniform_->update(inst_.ctx_.dev, camera); }

void Shader::Handler::updateDefaultDynamicUniform() { inst_.pDefaultDynamicUniform_->update(inst_.ctx_.dev); }

VkDescriptorSetLayoutBinding Shader::Handler::getDescriptorLayoutBinding(const DESCRIPTOR_TYPE& type) {
    switch (type) {
        case DESCRIPTOR_TYPE::DEFAULT_UNIFORM:
            return inst_.pDefaultUniform_->getDecriptorLayoutBinding();
        case DESCRIPTOR_TYPE::DEFAULT_DYNAMIC_UNIFORM:
            return inst_.pDefaultDynamicUniform_->getDecriptorLayoutBinding();
        default:
            throw std::runtime_error("descriptor type not handled!");
    }
}
