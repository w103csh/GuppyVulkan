/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
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
        if (keyValue.second.second.module)
            shell().context().dev.destroyShaderModule(keyValue.second.second.module, shell().context().pAllocator);
    }

    cleanup();
}

#define PRINT_NAME_ON_COMPILE false
#define PRINT_NAME_ON_CREATE false
#if PRINT_NAME_ON_CREATE
#include <sstream>
#endif

bool Shader::Handler::make(infoMapKeyValue& keyValue, bool doAssert, bool isInit) {
    auto& stageInfo = keyValue.second.second;
    if (isInit && stageInfo.module) return false;

    const auto& createInfo = ALL.at(std::get<0>(keyValue.first));
    if (!isInit) {
        shell().log(Shell::LogPriority::LOG_INFO,
                    ("Recompiling shader: \"" + std::string(createInfo.fileName) + "\"").c_str());
    }

    // if (std::get<0>(keyValue.first) == SHADER::OCEAN_VERT)  //
    //    auto pause = true;
    auto stringTexts = loadText(keyValue, createInfo.replaceMap);

    // I can't figure out how else to do this atm.
    std::vector<const char*> texts;
    for (auto& s : stringTexts) texts.push_back(s.c_str());

    std::vector<unsigned int> spv;
    if (std::get<0>(keyValue.first) == SHADER::TEX_FRAG)  //
        auto pause = true;
#if PRINT_NAME_ON_COMPILE
    shell().log(Shell::LogPriority::LOG_INFO, ("Compiling shader: " + std::string(createInfo.fileName)).c_str());
#endif
    bool success = GLSLtoSPV(static_cast<VkShaderStageFlagBits>(createInfo.stage), texts, spv);

    // Return or assert on fail
    if (!success) {
        shell().log(Shell::LogPriority::LOG_ERR, ("Error compiling shader: " + std::string(createInfo.fileName)).c_str());
        if (doAssert) {
            assert(success);
        }
        return false;
    }

    vk::ShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.codeSize = spv.size() * sizeof(unsigned int);
    moduleInfo.pCode = spv.data();

    // Store old module for clean up if necessary
    bool needsUpdate = stageInfo.module;
    if (needsUpdate) oldModules_.push_back(std::move(stageInfo.module));

    stageInfo = vk::PipelineShaderStageCreateInfo{};
    stageInfo.stage = createInfo.stage;
    stageInfo.pName = "main";
    stageInfo.module = shell().context().dev.createShaderModule(moduleInfo, shell().context().pAllocator);
#if PRINT_NAME_ON_CREATE
    std::stringstream ss;
    ss << "Creating shader module: " << createInfo.fileName << " 0x" << stageInfo.module;
    shell().log(Shell::LogPriority::LOG_INFO, ss.str().c_str());
#endif
    // shell().context().dbg.setMarkerName(stageInfo.module, createInfo.name.c_str());

    return needsUpdate;
}

std::vector<std::string> Shader::Handler::loadText(const infoMapKeyValue& keyValue,
                                                   const std::map<std::string, std::string>& replaceMap) {
    std::vector<std::string> texts;
    const auto& createInfo = ALL.at(std::get<0>(keyValue.first));

    // main shader text
    if (shaderTexts_.count(std::get<0>(keyValue.first)) == 0) {
        shaderTexts_[std::get<0>(keyValue.first)] = FileLoader::readFile(BASE_DIRNAME + std::string(createInfo.fileName));
    }
    texts.push_back(shaderTexts_.at(std::get<0>(keyValue.first)));
    // TEXT REPLACE
    helpers::textReplaceFromMap(replaceMap, texts.back());
    textReplaceDescSet(keyValue.second.first, texts.back());
    uniformHandler().textReplaceShader(keyValue.second.first, Shader::ALL.at(std::get<0>(keyValue.first)).fileName,
                                       texts.back());
    textReplacePipeline(std::get<1>(keyValue.first), texts.back());

    // link shader text
    for (const auto& linkShaderType : createInfo.linkTypes) {
        const auto& linkCreateInfo = LINK_ALL.at(linkShaderType);
        if (shaderLinkTexts_.count(linkShaderType) == 0) {
            shaderLinkTexts_[linkShaderType] = FileLoader::readFile(BASE_DIRNAME + std::string(linkCreateInfo.fileName));
        }
        texts.push_back(shaderLinkTexts_.at(linkShaderType));
        // TEXT REPLACE
        helpers::textReplaceFromMap(linkCreateInfo.replaceMap, texts.back());
        textReplaceDescSet(keyValue.second.first, texts.back());
        uniformHandler().textReplaceShader(keyValue.second.first, Shader::LINK_ALL.at(linkShaderType).fileName,
                                           texts.back());
        textReplacePipeline(std::get<1>(keyValue.first), texts.back());
    }
    return texts;
}

void Shader::Handler::textReplaceDescSet(const Descriptor::Set::textReplaceTuples& replaceTuples, std::string& text) const {
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

void Shader::Handler::textReplacePipeline(const PIPELINE pipelineType, std::string& text) const {
    if (!std::visit(Pipeline::IsCompute{}, pipelineType)) return;
    {  // LOCAL SIZE
        const auto localSize =
            static_cast<const Pipeline::Compute*>(pipelineHandler().getPipeline(pipelineType).get())->getLocalSize();
        auto replaceInfo = helpers::getMacroReplaceInfo(Pipeline::LOCAL_SIZE_MACRO_ID_PREFIX, text);
        for (auto& info : replaceInfo) {
            switch (std::get<0>(info).back()) {
                case 'X':
                    helpers::macroReplace(info, localSize.x, text);
                    break;
                case 'Y':
                    helpers::macroReplace(info, localSize.y, text);
                    break;
                case 'Z':
                    helpers::macroReplace(info, localSize.z, text);
                    break;
                default:
                    assert(false && "Unhandled case");
                    break;
            }
        }
    }
}

void Shader::Handler::getStagesInfo(const SHADER& shaderType, const PIPELINE& pipelineType, const PASS& passType,
                                    std::vector<vk::PipelineShaderStageCreateInfo>& stagesInfo) const {
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

vk::ShaderStageFlags Shader::Handler::getStageFlags(const std::set<SHADER>& shaderTypes) const {
    vk::ShaderStageFlags stages = {};
    for (const auto& shaderType : shaderTypes) stages |= ALL.at(shaderType).stage;
    return stages;
}

void Shader::Handler::recompileShader(std::string fileName) {
    init_glslang();

    std::vector<SHADER> needsUpdateTypes;

    // Check for main shader
    for (const auto& [shaderType, createInfo] : ALL) {
        if (createInfo.fileName == fileName) {
            for (auto& stageInfoKeyValue : infoMap_) {
                if (std::get<0>(stageInfoKeyValue.first) == shaderType) {
                    if (make(stageInfoKeyValue, settings().assertOnRecompileShader, false)) {
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
                        if (make(stageInfoKeyValue, settings().assertOnRecompileShader, false)) {
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
        shell().context().dev.destroyShaderModule(module, shell().context().pAllocator);
    oldModules_.clear();
}
