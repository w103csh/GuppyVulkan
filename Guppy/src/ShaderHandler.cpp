/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "ShaderHandler.h"

#include "FileLoader.h"
#include "Shell.h"
#include "util.hpp"
// HANDLERS
#include "PipelineHandler.h"
#include "UniformHandler.h"

Shader::Handler::Handler(Game* pGame) : Game::Handler(pGame), clearTextsAfterLoad(true) {}

void Shader::Handler::init() {
    reset();

    pipelineHandler().makeShaderInfoMap(infoMap_);

    init_glslang();

    for (auto& keyValue : infoMap_) {
        bool needsUpdate = make(keyValue, true, true);
        assert(!needsUpdate);
    }

    finalize_glslang();

    if (clearTextsAfterLoad) {
        shaderTexts_.clear();
        shaderLinkTexts_.clear();
    }
}

void Shader::Handler::reset() {
    // SHADERS
    for (auto& keyValue : infoMap_) {
        if (keyValue.second.second.module != VK_NULL_HANDLE)
            vkDestroyShaderModule(shell().context().dev, keyValue.second.second.module, nullptr);
    }

    cleanup();
}

bool Shader::Handler::make(infoMapKeyValue& keyValue, bool doAssert, bool isInit) {
    auto& stageInfo = keyValue.second.second;
    if (isInit && stageInfo.module != VK_NULL_HANDLE) return false;

    auto stringTexts = loadText(keyValue);

    // I can't figure out how else to do this atm.
    std::vector<const char*> texts;
    for (auto& s : stringTexts) texts.push_back(s.c_str());

    const auto& createInfo = ALL.at(std::get<0>(keyValue.first));
    std::vector<unsigned int> spv;
    bool success = GLSLtoSPV(createInfo.stage, texts, spv);

    // Return or assert on fail
    if (!success) {
        if (doAssert) {
            if (!success) shell().log(Shell::LogPriority::LOG_ERR, ("Error compiling: " + createInfo.fileName).c_str());
            assert(success);
        } else {
            return false;
        }
    }

    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = spv.size() * sizeof(unsigned int);
    moduleInfo.pCode = spv.data();

    // Store old module for clean up if necessary
    bool needsUpdate = stageInfo.module != VK_NULL_HANDLE;
    if (needsUpdate) oldModules_.push_back(std::move(stageInfo.module));

    stageInfo = {};
    vk::assert_success(vkCreateShaderModule(shell().context().dev, &moduleInfo, nullptr, &stageInfo.module));

    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = createInfo.stage;
    stageInfo.pName = "main";

    if (shell().context().debugMarkersEnabled) {
        std::string markerName = createInfo.name + "shader module";
        ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)stageInfo.module,
                                      VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, markerName.c_str());
    }

    return needsUpdate;
}

std::vector<std::string> Shader::Handler::loadText(const infoMapKeyValue& keyValue) {
    std::vector<std::string> texts;
    const auto& createInfo = ALL.at(std::get<0>(keyValue.first));

    // main shader text
    if (shaderTexts_.count(std::get<0>(keyValue.first)) == 0) {
        shaderTexts_[std::get<0>(keyValue.first)] = FileLoader::readFile(BASE_DIRNAME + createInfo.fileName);
    }
    texts.push_back(shaderTexts_.at(std::get<0>(keyValue.first)));
    textReplace(keyValue.second.first, texts.back());
    uniformHandler().shaderTextReplace(keyValue.second.first, texts.back());

    // link shader text
    for (const auto& linkShaderType : createInfo.linkTypes) {
        const auto& linkCreateInfo = LINK_ALL.at(linkShaderType);
        if (shaderLinkTexts_.count(linkShaderType) == 0) {
            shaderLinkTexts_[linkShaderType] = FileLoader::readFile(BASE_DIRNAME + linkCreateInfo.fileName);
        }
        texts.push_back(shaderLinkTexts_.at(linkShaderType));
        textReplace(keyValue.second.first, texts.back());
        uniformHandler().shaderTextReplace(keyValue.second.first, texts.back());
    }
    return texts;
}

void Shader::Handler::textReplace(const Descriptor::Set::textReplaceTuples& replaceTuples, std::string& text) const {
    auto replaceInfo = helpers::getMacroReplaceInfo(Descriptor::Set::MACRO_ID_PREFIX, text);
    for (auto& info : replaceInfo) {
        for (const auto& tuple : replaceTuples) {
            if (std::get<0>(tuple) != std::get<0>(info)) continue;  // TODO: Warn?
            auto slot = static_cast<int>(std::get<1>(tuple));
            if (slot != std::get<3>(info))  //
                helpers::macroReplace(info, slot, text);
        }
    }
}

void Shader::Handler::getStagesInfo(const SHADER& shaderType, const PIPELINE& pipelineType, const PASS& passType,
                                    std::vector<VkPipelineShaderStageCreateInfo>& stagesInfo) const {
    auto it1 = infoMap_.begin();
    auto it2 = infoMap_.end();
    // Look for a specific stage info
    for (; it1 != infoMap_.end(); ++it1) {
        if (std::get<0>(it1->first) != shaderType) continue;
        if (std::get<1>(it1->first) != pipelineType) continue;
        if (it2 == infoMap_.end()) it2 = it1;  // Save the spot where the relevant info begins
        if (std::get<2>(it1->first).find(passType) != std::get<2>(it1->first).end()) {
            stagesInfo.push_back(it1->second.second);
            return;
        };
    }
    assert(it2 != infoMap_.end());
    it1 = it2;
    // Look for a default stage info
    for (; it1 != infoMap_.end(); ++it1) {
        if (std::get<0>(it1->first) != shaderType) continue;
        if (std::get<1>(it1->first) != pipelineType) continue;
        if (std::get<2>(it1->first) == Uniform::PASS_ALL_SET) {
            stagesInfo.push_back(it1->second.second);
            return;
        };
    }
    assert(false);  // A shader stage info was not found.
}

VkShaderStageFlags Shader::Handler::getStageFlags(const std::set<SHADER>& shaderTypes) const {
    VkShaderStageFlags stages = 0;
    for (const auto& shaderType : shaderTypes) stages |= ALL.at(shaderType).stage;
    return stages;
}

void Shader::Handler::recompileShader(std::string fileName) {
    bool assert = settings().assert_on_recompile_shader;

    init_glslang();

    std::vector<SHADER> needsUpdateTypes;

    // Check for main shader
    for (const auto& [shaderType, createInfo] : ALL) {
        if (createInfo.fileName == fileName) {
            for (auto& stageInfoKeyValue : infoMap_) {
                if (std::get<0>(stageInfoKeyValue.first) == shaderType) {
                    if (make(stageInfoKeyValue, assert, false)) {
                        needsUpdateTypes.push_back(shaderType);
                    }
                }
            }
        }
    }

    // Check for link shader
    for (const auto& [linkShaderType, createInfo] : LINK_ALL) {
        if (createInfo.fileName == fileName) {
            // Get the mains mapped to this link shader...
            std::vector<SHADER> shaderTypes;
            getShaderTypes(linkShaderType, shaderTypes);
            for (auto& stageInfoKeyValue : infoMap_) {
                for (const auto& shaderType : shaderTypes) {
                    if (std::get<0>(stageInfoKeyValue.first) == shaderType) {
                        if (make(stageInfoKeyValue, assert, false)) {
                            needsUpdateTypes.push_back(shaderType);
                        }
                    }
                }
            }
        }
    }

    // Notify pipeline handler
    if (needsUpdateTypes.size()) pipelineHandler().needsUpdate(needsUpdateTypes);

    finalize_glslang();

    if (clearTextsAfterLoad) {
        shaderTexts_.clear();
        shaderLinkTexts_.clear();
    }
}

void Shader::Handler::getShaderTypes(const SHADER_LINK& linkType, std::vector<SHADER>& types) {
    for (const auto& keyValue : LINK_MAP) {
        for (const auto& setItem : keyValue.second) {
            if (setItem == linkType) types.push_back(keyValue.first);
        }
    }
}

void Shader::Handler::cleanup() {
    for (auto& module : oldModules_)  //
        vkDestroyShaderModule(shell().context().dev, module, nullptr);
    oldModules_.clear();
}
