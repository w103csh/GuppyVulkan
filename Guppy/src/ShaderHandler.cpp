
#include <sstream>

#include "ShaderHandler.h"

#include "Camera.h"
#include "FileLoader.h"
#include "InputHandler.h"
#include "Material.h"
#include "Object3d.h"
#include "PBR.h"
#include "PipelineHandler.h"
#include "Scene.h"
#include "Shell.h"
#include "TextureHandler.h"
#include "util.hpp"

Shader::Handler::Handler(Game* pGame) : Game::Handler(pGame) {
    // SHADERS (main)
    for (const auto& type : SHADER_ALL) {
        switch (type) {
            case SHADER::COLOR_VERT:
                pShaders_.emplace_back(std::make_unique<Default::ColorVertex>(std::ref(*this)));
                break;
            case SHADER::COLOR_FRAG:
                pShaders_.emplace_back(std::make_unique<Default::ColorFragment>(std::ref(*this)));
                break;
            case SHADER::LINE_FRAG:
                pShaders_.emplace_back(std::make_unique<Default::LineFragment>(std::ref(*this)));
                break;
            case SHADER::TEX_VERT:
                pShaders_.emplace_back(std::make_unique<Default::TextureVertex>(std::ref(*this)));
                break;
            case SHADER::TEX_FRAG:
                pShaders_.emplace_back(std::make_unique<Default::TextureFragment>(std::ref(*this)));
                break;
            case SHADER::PBR_COLOR_FRAG:
                pShaders_.emplace_back(std::make_unique<PBR::ColorFrag>(std::ref(*this)));
                break;
            default:
                assert(false);  // add new ones here...
        }
    }
    // Validate the list.
    assert(pShaders_.size() == SHADER_ALL.size());
    for (const auto& pShader : pShaders_) assert(pShader != nullptr);

    // SHADERS (link)
    for (const auto& type : SHADER_LINK_ALL) {
        switch (type) {
            case SHADER_LINK::UTIL_FRAG:
                pLinkShaders_.emplace_back(std::make_unique<UtilityFragment>(std::ref(*this)));
                break;
            default:
                assert(false);  // add new ones here...
        }
    }
    // Validate the list.
    assert(pLinkShaders_.size() == SHADER_LINK_ALL.size());
    for (const auto& pLinkShader : pLinkShaders_) assert(pLinkShader != nullptr);
}

void Shader::Handler::init() {
    // Validate the list.

    reset();

    // LINK SHADERS
    for (auto& pLinkShader : pLinkShaders_) pLinkShader->init(oldModules_);

    init_glslang();

    // MAIN SHADERS
    for (auto& pShader : pShaders_) pShader->init(oldModules_);

    finalize_glslang();
}

void Shader::Handler::reset() {
    // SHADERS
    for (auto& pShader : pShaders_) pShader->destroy();

    cleanup();
}

const std::unique_ptr<Shader::Base>& Shader::Handler::getShader(const SHADER& type) const {
    assert(type != SHADER::LINK);  // TODO: this ain't great...
    return pShaders_[static_cast<uint32_t>(type)];
}

const std::unique_ptr<Shader::Link>& Shader::Handler::getLinkShader(const SHADER_LINK& type) const {
    return pLinkShaders_[static_cast<uint32_t>(type)];
}

void Shader::Handler::getStagesInfo(const std::set<SHADER>& shaderTypes,
                                    std::vector<VkPipelineShaderStageCreateInfo>& stagesInfo) const {
    for (auto& shaderType : shaderTypes) stagesInfo.push_back(getShader(shaderType)->info);
}

VkShaderStageFlags Shader::Handler::getStageFlags(const std::set<SHADER>& shaderTypes) const {
    VkShaderStageFlags stages = 0;
    for (const auto& shaderType : shaderTypes) stages |= getShader(shaderType)->STAGE;
    return stages;
}

void Shader::Handler::recompileShader(std::string fileName) {
    bool assert = settings().assert_on_recompile_shader;

    // Check for main shader
    for (auto& pShader : pShaders_) {
        if (pShader->FILE_NAME.compare(fileName) == 0) {
            if (loadShaders({pShader->TYPE}, assert)) {
                pipelineHandler().needsUpdate({pShader->TYPE});
                return;
            };
        }
    }

    // Check for link shader.
    for (auto& pLinkShader : pLinkShaders_) {
        if (pLinkShader->FILE_NAME.compare(fileName) == 0) {
            // Get the mains mapped to this link shader...
            std::vector<SHADER> types;
            getShaderTypes(pLinkShader->LINK_TYPE, types);

            if (loadShaders(types, assert)) {
                pipelineHandler().needsUpdate(types);
                return;
            };
        }
    }
}

void Shader::Handler::appendLinkTexts(const std::set<SHADER_LINK>& linkTypes, std::vector<const char*>& texts) const {
    for (auto& type : linkTypes) texts.push_back(getLinkShader(type)->loadText(false));
}

bool Shader::Handler::loadShaders(const std::vector<SHADER>& types, bool doAssert, bool load) {
    // For cleanup
    auto numOldResources = oldModules_.size();

    // Load link shaders
    if (load) {
        std::set<SHADER_LINK> linkTypes;
        // Gather unique list of link shaders...
        for (const auto& type : types) {
            if (SHADER_LINK_MAP.count(type)) {
                for (const auto& linkType : SHADER_LINK_MAP.at(type)) {
                    linkTypes.insert(linkType);
                }
            }
        }
        // Only load from file once.
        for (const auto& linkType : linkTypes) getLinkShader(linkType)->loadText(load);
    }

    init_glslang();

    for (const auto& type : types) getShader(type)->init(oldModules_, load, doAssert);

    finalize_glslang();

    return numOldResources < oldModules_.size();
}

void Shader::Handler::getShaderTypes(const SHADER_LINK& linkType, std::vector<SHADER>& types) {
    for (const auto& keyValue : SHADER_LINK_MAP) {
        for (const auto& setItem : keyValue.second) {
            if (setItem == linkType) types.push_back(keyValue.first);
        }
    }
}

void Shader::Handler::cleanup() {
    for (auto& module : oldModules_) vkDestroyShaderModule(shell().context().dev, module, nullptr);
    oldModules_.clear();
}
