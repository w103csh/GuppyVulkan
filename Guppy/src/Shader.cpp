
#include "Shader.h"

#include "DescriptorHandler.h"
#include "FileLoader.h"
#include "ShaderHandler.h"
#include "UniformHandler.h"
#include "util.hpp"

// **********************
//      Base
// **********************

void Shader::Base::init(std::vector<VkShaderModule>& oldModules, bool load, bool doAssert) {
    bool isRecompile = (module != VK_NULL_HANDLE);

    // Check if shader needs to be (re)compiled
    std::vector<const char*> shaderTexts = {loadText(load)};

    handler().appendLinkTexts(LINK_TYPES, shaderTexts);
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

    auto& dev = handler().shell().context().dev;
    vk::assert_success(vkCreateShaderModule(dev, &moduleInfo, nullptr, &module));

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = STAGE;
    stageInfo.pName = "main";
    stageInfo.module = module;
    info = std::move(stageInfo);

    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + "shader module";
        ext::DebugMarkerSetObjectName(dev, (uint64_t)module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
                                      markerName.c_str());
    }
}

const char* Shader::Base::loadText(bool load) {
    if (load) text_ = FileLoader::readFile(BASE_DIRNAME + FILE_NAME);
    for (const auto& setType : DESCRIPTOR_SETS) {
        const auto& set = handler().descriptorHandler().getDescriptorSet(setType);
        text_ = handler().uniformHandler().macroReplace(set.BINDING_MAP, text_);
    }
    return text_.c_str();
}

void Shader::Base::destroy() {
    auto& dev = handler().shell().context().dev;
    if (module != VK_NULL_HANDLE) vkDestroyShaderModule(dev, module, nullptr);
}

// **********************
//      LINK SHADERS
// **********************

Shader::UtilityFragment::UtilityFragment(Shader::Handler& handler)
    : Link{handler,
           SHADER_LINK::UTIL_FRAG,
           "util.frag.glsl",
           "Utility Fragement Shader",
           {
               {DESCRIPTOR_SET::UNIFORM_DEFAULT}  //
           }} {}

void Shader::UtilityFragment::init(std::vector<VkShaderModule>& oldModules, bool load, bool doAssert) { loadText(load); };

// **********************
//      DEFAULT SHADERS
// **********************

Shader::Default::ColorVertex::ColorVertex(Shader::Handler& handler)
    : Base{handler,  //
           SHADER::COLOR_VERT,
           "color.vert",
           VK_SHADER_STAGE_VERTEX_BIT,
           "Default Color Vertex Shader",
           {
               {DESCRIPTOR_SET::UNIFORM_DEFAULT}  //
           }} {};

Shader::Default::ColorFragment::ColorFragment(Shader::Handler& handler)
    : Base{handler,             //
           SHADER::COLOR_FRAG,  //
           "color.frag",
           VK_SHADER_STAGE_FRAGMENT_BIT,
           "Default Color Fragment Shader",
           {
               {DESCRIPTOR_SET::UNIFORM_DEFAULT}  //
           },
           {SHADER_LINK::UTIL_FRAG}} {}

Shader::Default::LineFragment::LineFragment(Shader::Handler& handler)
    : Base{handler,            //
           SHADER::LINE_FRAG,  //
           "line.frag",
           VK_SHADER_STAGE_FRAGMENT_BIT,
           "Default Line Fragment Shader",
           {
               {DESCRIPTOR_SET::UNIFORM_DEFAULT}  //
           }} {}

Shader::Default::TextureVertex::TextureVertex(Shader::Handler& handler)
    : Base{handler,  //
           SHADER::TEX_VERT,
           "texture.vert",
           VK_SHADER_STAGE_VERTEX_BIT,
           "Default Texture Vertex Shader",
           {
               {DESCRIPTOR_SET::UNIFORM_DEFAULT}  //
           }} {}

Shader::Default::TextureFragment::TextureFragment(Shader::Handler& handler)
    : Base{handler,  //
           SHADER::TEX_FRAG,
           "texture.frag",
           VK_SHADER_STAGE_FRAGMENT_BIT,
           "Default Texture Fragment Shader",
           {
               {DESCRIPTOR_SET::UNIFORM_DEFAULT},  //
               {DESCRIPTOR_SET::SAMPLER_DEFAULT}   //
           },
           {SHADER_LINK::UTIL_FRAG}} {}
