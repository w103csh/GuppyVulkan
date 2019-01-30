
#include "Shader.h"

#include "FileLoader.h"
#include "ShaderHandler.h"
#include "util.hpp"

// **********************
//      Base
// **********************

void Shader::Base::init(const VkDevice& dev, const Game::Settings& settings, std::vector<VkShaderModule>& oldModules,
                        bool load, bool doAssert) {
    bool isRecompile = (module != VK_NULL_HANDLE);

    // Check if shader needs to be (re)compiled
    std::vector<const char*> shaderTexts = {loadText(load)};

    handler_.appendLinkTexts(LINK_TYPES, shaderTexts);
    // pShaderTexts.insert(pShaderTexts.end(), pLinkTexts.begin(), pLinkTexts.end());

    std::vector<unsigned int> spv;
    bool success = GLSLtoSPV(STAGE, shaderTexts, spv);

    // Return or assert on fail
    if (!success) {
        if (doAssert)
            assert(success);
        else
            return;
    }

    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = spv.size() * sizeof(unsigned int);
    moduleInfo.pCode = spv.data();

    // Store old module for clean up if necessary
    if (module != VK_NULL_HANDLE) oldModules.push_back(std::move(module));

    vk::assert_success(vkCreateShaderModule(dev, &moduleInfo, nullptr, &module));

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = STAGE;
    stageInfo.pName = "main";
    stageInfo.module = module;
    info = std::move(stageInfo);

    if (settings.enable_debug_markers) {
        std::string markerName = NAME + "shader module";
        ext::DebugMarkerSetObjectName(dev, (uint64_t)module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
                                      markerName.c_str());
    }
}

const char* Shader::Base::loadText(bool load) {
    if (load) text_ = FileLoader::readFile(BASE_DIRNAME + FILE_NAME);
    return text_.c_str();
}

void Shader::Base::destroy(const VkDevice& dev) {
    if (module != VK_NULL_HANDLE) vkDestroyShaderModule(dev, module, nullptr);
}

using namespace Shader;

// **********************
//      LINK SHADERS
// **********************

const char* UtilityFragment::loadText(bool load) {
    Base::loadText(load);
    assert(false);
    // Shader::Handler::lightMacroReplace("NUM_POS_LIGHTS", Shader::Handler::getDefUniformPositionLightCount(), text_);
    // Shader::Handler::lightMacroReplace("NUM_SPOT_LIGHTS", Shader::Handler::getDefUniformSpotLightCount(), text_);
    return text_.c_str();
}

// **********************
//      DEFAULT SHADERS
// **********************
