
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

    pipelineHandler().initShaderInfoMap(shaderInfoMap_);

    init_glslang();

    for (auto& keyValue : shaderInfoMap_) {
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
    for (auto& keyValue : shaderInfoMap_) {
        if (keyValue.second.module != VK_NULL_HANDLE)
            vkDestroyShaderModule(shell().context().dev, keyValue.second.module, nullptr);
    }

    cleanup();
}

bool Shader::Handler::make(shaderInfoMapKeyValue& keyValue, bool doAssert, bool isInit) {
    auto& stageInfo = keyValue.second;
    if (isInit && stageInfo.module != VK_NULL_HANDLE) return false;

    auto stringTexts = loadText(keyValue.first);

    // I can't figure out how else to do this atm.
    std::vector<const char*> texts;
    for (auto& s : stringTexts) texts.push_back(s.c_str());

    const auto& createInfo = SHADER_ALL.at(keyValue.first.first);
    std::vector<unsigned int> spv;
    bool success = GLSLtoSPV(createInfo.stage, texts, spv);

    // Return or assert on fail
    if (!success) {
        if (doAssert) {
            if (!success) shell().log(Shell::LOG_ERR, ("Error compiling: " + createInfo.fileName).c_str());
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

    if (settings().enable_debug_markers) {
        std::string markerName = createInfo.name + "shader module";
        ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)stageInfo.module,
                                      VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, markerName.c_str());
    }

    return needsUpdate;
}

std::vector<std::string> Shader::Handler::loadText(const shaderInfoMapKey& keyValue) {
    std::vector<std::string> texts;
    const auto& createInfo = SHADER_ALL.at(keyValue.first);

    // main shader text
    if (shaderTexts_.count(keyValue.first) == 0) {
        shaderTexts_[keyValue.first] = FileLoader::readFile(BASE_DIRNAME + createInfo.fileName);
    }
    texts.push_back(shaderTexts_.at(keyValue.first));
    textReplace(keyValue.second, texts.back());
    uniformHandler().shaderTextReplace(texts.back());

    // link shader text
    for (const auto& linkShaderType : createInfo.linkTypes) {
        const auto& linkCreateInfo = SHADER_LINK_ALL.at(linkShaderType);
        if (shaderLinkTexts_.count(linkShaderType) == 0) {
            shaderLinkTexts_[linkShaderType] = FileLoader::readFile(BASE_DIRNAME + linkCreateInfo.fileName);
        }
        texts.push_back(shaderLinkTexts_.at(linkShaderType));
        textReplace(keyValue.second, texts.back());
        uniformHandler().shaderTextReplace(texts.back());
    }
    return texts;
}

void Shader::Handler::textReplace(const descSetMacroSlotMap& slotMap, std::string& text) const {
    auto replaceInfo = helpers::getMacroReplaceInfo(DESC_SET_MACRO_ID_PREFIX, text);
    for (auto& info : replaceInfo) {
        if (slotMap.count(std::get<0>(info)) == 0) continue;  // TODO: Warn?
        auto slot = slotMap.at(std::get<0>(info));
        if (static_cast<int>(slot) != std::get<3>(info))  //
            helpers::macroReplace(info, static_cast<int>(slot), text);
    }
}

void Shader::Handler::getStagesInfo(const shaderInfoMapKey& key,
                                    std::vector<VkPipelineShaderStageCreateInfo>& stagesInfo) const {
    stagesInfo.push_back(shaderInfoMap_.at(key));
}

VkShaderStageFlags Shader::Handler::getStageFlags(const std::set<SHADER>& shaderTypes) const {
    VkShaderStageFlags stages = 0;
    for (const auto& shaderType : shaderTypes) stages |= SHADER_ALL.at(shaderType).stage;
    return stages;
}

void Shader::Handler::recompileShader(std::string fileName) {
    bool assert = settings().assert_on_recompile_shader;

    init_glslang();

    std::vector<SHADER> needsUpdateTypes;

    // Check for main shader
    for (const auto& createInfoKeyValue : SHADER_ALL) {
        if (createInfoKeyValue.second.fileName == fileName) {
            for (auto& stageInfoKeyValue : shaderInfoMap_) {
                if (stageInfoKeyValue.first.first == createInfoKeyValue.first) {
                    if (make(stageInfoKeyValue, assert, false)) {
                        needsUpdateTypes.push_back(createInfoKeyValue.first);
                    }
                }
            }
        }
    }

    // Check for link shader
    for (const auto& createInfoKeyValue : SHADER_LINK_ALL) {
        if (createInfoKeyValue.second.fileName == fileName) {
            // Get the mains mapped to this link shader...
            std::vector<SHADER> types;
            getShaderTypes(createInfoKeyValue.second.type, types);
            for (auto& stageInfoKeyValue : shaderInfoMap_) {
                for (const auto& type : types) {
                    if (stageInfoKeyValue.first.first == type) {
                        if (make(stageInfoKeyValue, assert, false)) {
                            needsUpdateTypes.push_back(type);
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
    for (const auto& keyValue : SHADER_LINK_MAP) {
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
