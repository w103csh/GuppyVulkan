
#include <sstream>

#include "ShaderHandler.h"

#include "Camera.h"
#include "FileLoader.h"
#include "InputHandler.h"
#include "Material.h"
#include "Object3d.h"
#include "PBR.h"
#include "Scene.h"
#include "TextureHandler.h"
#include "util.hpp"

// **********************
//      Shader::Handler
// **********************

Shader::Handler Shader::Handler::inst_;
Shader::Handler::Handler()
    :  // UNIFORMS
      pDefaultUniform_(std::make_unique<Uniform::Default>()),
      pDefaultDynamicUniform_(std::make_unique<Uniform::DefaultDynamic>()) {
    // SHADERS (main)
    for (const auto& type : SHADER_ALL) {
        switch (type) {
            case SHADER::COLOR_VERT:
                pShaders_.emplace_back(std::make_unique<Default::ColorVertex>());
                break;
            case SHADER::COLOR_FRAG:
                pShaders_.emplace_back(std::make_unique<Default::ColorFragment>());
                break;
            case SHADER::LINE_FRAG:
                pShaders_.emplace_back(std::make_unique<Default::LineFragment>());
                break;
            case SHADER::TEX_VERT:
                pShaders_.emplace_back(std::make_unique<Default::TextureVertex>());
                break;
            case SHADER::TEX_FRAG:
                pShaders_.emplace_back(std::make_unique<Default::TextureFragment>());
                break;
            case SHADER::PBR_COLOR_FRAG:
                pShaders_.emplace_back(std::make_unique<PBR::ColorFrag>());
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
                pLinkShaders_.emplace_back(std::make_unique<UtilityFragment>());
                break;
            default:
                assert(false);  // add new ones here...
        }
    }
    // Validate the list.
    assert(pLinkShaders_.size() == SHADER_LINK_ALL.size());
    for (const auto& pLinkShader : pLinkShaders_) assert(pLinkShader != nullptr);
}

void Shader::Handler::init(Shell& sh, const Game::Settings& settings, const Camera& camera) {
    inst_.reset();

    inst_.ctx_ = sh.context();
    inst_.settings_ = settings;

    // UNIFORMS
    inst_.pDefaultUniform_->init(inst_.ctx_.dev, settings, camera);
    inst_.pDefaultDynamicUniform_->init(inst_.ctx_, settings, TEXTURE_LIMIT);

    // LINK SHADERS
    for (auto& pLinkShader : inst_.pLinkShaders_) pLinkShader->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);

    init_glslang();

    // MAIN SHADERS
    for (auto& pShader : inst_.pShaders_) pShader->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_);

    finalize_glslang();

    // listen to changes to the shader files
    if (settings.enable_directory_listener) {
        sh.watchDirectory(BASE_DIRNAME, &Shader::Handler::recompileShader);
    }
}

void Shader::Handler::reset() {
    // SHADERS
    for (auto& pShader : pShaders_) pShader->destroy(inst_.ctx_.dev);

    cleanup();

    // UNIFORMS
    pDefaultUniform_->destroy(ctx_.dev);
    pDefaultDynamicUniform_->destroy(ctx_.dev);
}

std::unique_ptr<Shader::Base>& Shader::Handler::getShader(const SHADER& type) {
    assert(type != SHADER::LINK);  // TODO: this ain't great...
    return inst_.pShaders_[static_cast<uint32_t>(type)];
}

std::unique_ptr<Shader::Link>& Shader::Handler::getLinkShader(const SHADER_LINK& type) {
    return inst_.pLinkShaders_[static_cast<uint32_t>(type)];
}

void Shader::Handler::getStagesInfo(const std::set<SHADER>& shaderTypes,
                                    std::vector<VkPipelineShaderStageCreateInfo>& stagesInfo) {
    for (auto& shaderType : shaderTypes) stagesInfo.push_back(inst_.getShader(shaderType)->info);
}

VkShaderStageFlags Shader::Handler::getStageFlags(const std::set<SHADER>& shaderTypes) {
    VkShaderStageFlags stages = 0;
    for (const auto& shaderType : shaderTypes) stages |= inst_.getShader(shaderType)->STAGE;
    return stages;
}

void Shader::Handler::recompileShader(std::string fileName) {
    bool assert = inst_.settings_.assert_on_recompile_shader;

    // Check for main shader
    for (auto& pShader : inst_.pShaders_) {
        if (pShader->FILE_NAME.compare(fileName) == 0) {
            if (inst_.loadShaders({pShader->TYPE}, assert)) {
                Pipeline::Handler::needsUpdate({pShader->TYPE});
                return;
            };
        }
    }

    // Check for link shader.
    for (auto& pLinkShader : inst_.pLinkShaders_) {
        if (pLinkShader->FILE_NAME.compare(fileName) == 0) {
            // Get the mains mapped to this link shader...
            std::vector<SHADER> types;
            inst_.getShaderTypes(pLinkShader->LINK_TYPE, types);

            if (inst_.loadShaders(types, assert)) {
                Pipeline::Handler::needsUpdate(types);
                return;
            };
        }
    }
}

void Shader::Handler::appendLinkTexts(const std::set<SHADER_LINK>& linkTypes, std::vector<const char*>& texts) {
    for (auto& type : linkTypes) texts.push_back(getLinkShader(type)->loadText(false));
}

bool Shader::Handler::loadShaders(const std::vector<SHADER>& types, bool doAssert, bool load) {
    // For cleanup
    auto numOldResources = inst_.oldModules_.size();

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

    for (const auto& type : types) getShader(type)->init(inst_.ctx_.dev, inst_.settings_, inst_.oldModules_, load, doAssert);

    finalize_glslang();

    return numOldResources < inst_.oldModules_.size();
}

void Shader::Handler::getShaderTypes(const SHADER_LINK& linkType, std::vector<SHADER>& types) {
    for (const auto& keyValue : SHADER_LINK_MAP) {
        for (const auto& setItem : keyValue.second) {
            if (setItem == linkType) types.push_back(keyValue.first);
        }
    }
}

void Shader::Handler::updateDescriptorSets(const std::set<DESCRIPTOR> types, std::vector<VkDescriptorSet>& sets,
                                           uint32_t setCount, const std::shared_ptr<Texture::Data>& pTexture) {
    std::vector<VkWriteDescriptorSet> writes;
    for (uint32_t i = 0; i < setCount; i++) {
        for (const auto& type : types) {
            switch (type) {
                case DESCRIPTOR::DEFAULT_UNIFORM: {
                    writes.push_back(inst_.pDefaultUniform_->getWrite());
                    writes.back().dstSet = sets[i];
                } break;
                case DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM: {
                    writes.push_back(inst_.pDefaultDynamicUniform_->getWrite());
                    writes.back().dstSet = sets[i];
                } break;
                case DESCRIPTOR::DEFAULT_SAMPLER: {
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
std::vector<uint32_t> Shader::Handler::getDynamicOffsets(const std::set<DESCRIPTOR> types) {
    for (const auto& type : types) {
        switch (type) {
            case DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM:
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

VkDescriptorSetLayoutBinding Shader::Handler::getDescriptorLayoutBinding(const DESCRIPTOR& type) {
    switch (type) {
        case DESCRIPTOR::DEFAULT_UNIFORM:
            return inst_.pDefaultUniform_->getDecriptorLayoutBinding();
        case DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM:
            return inst_.pDefaultDynamicUniform_->getDecriptorLayoutBinding();
        default:
            throw std::runtime_error("descriptor type not handled!");
    }
}
