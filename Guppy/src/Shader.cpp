
#include <sstream>

#include "util.hpp"

#include "FileLoader.h"
#include "Shader.h"

// **********************
//      Shader
// **********************

const std::string SHADER_DIR = "Guppy\\src\\shaders\\";

Shader::Shader(const VkDevice dev, std::string fileName, std::vector<const char*> pLinkTexts, VkShaderStageFlagBits stage,
               std::string markerName) {
    text_ = FileLoader::read_file(SHADER_DIR + fileName);

    std::vector<const char*> pShaderTexts = {text_.c_str()};
    pShaderTexts.insert(pShaderTexts.end(), pLinkTexts.begin(), pLinkTexts.end());

    std::vector<unsigned int> spv;
    assert(GLSLtoSPV(stage, pShaderTexts, spv));

    VkShaderModuleCreateInfo module_info = {};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spv.size() * sizeof(unsigned int);
    module_info.pCode = spv.data();
    vk::assert_success(vkCreateShaderModule(dev, &module_info, nullptr, &module));

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage;
    stageInfo.pName = "main";
    stageInfo.module = module;
    info = std::move(stageInfo);

    // TODO: check settings
    if (!markerName.empty()) {
        ext::DebugMarkerSetObjectName(dev, (uint64_t)module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
                                      (markerName + " shader module").c_str());
    }
}

// **********************
//      ShaderHandler
// **********************

ShaderHandler ShaderHandler::inst_;
ShaderHandler::ShaderHandler() {}

void ShaderHandler::reset() {
    for (auto& shaderRes : shaders_) {
        if (shaderRes.module) vkDestroyShaderModule(ctx_.dev, shaderRes.module, nullptr);
    }
}

void ShaderHandler::init(MyShell& sh, const Game::Settings& settings, uint32_t numPosLights) {
    // Clean up owned memory...
    inst_.reset();

    inst_.ctx_ = sh.context();
    inst_.settings_ = settings;

    // link shaders
    inst_.utilFragText_ = FileLoader::read_file(SHADER_DIR + "util.frag.glsl");

    inst_.createShaderModules(numPosLights);

    // listen to changes to the shader files
    if (settings.enable_directory_listener) {
        sh.watch_directory(SHADER_DIR, &ShaderHandler::recompileShader);
    }
}

void ShaderHandler::recompileShader(std::string fileName) {
    //inst_.reset();
}

void ShaderHandler::createShaderModules(uint32_t numPositionLights) {
    // link shaders
    std::string utilFragText = utilFragText_;          // copy text for string replace ...
    posLightReplace(numPositionLights, utilFragText);  // replace position light array size

    init_glslang();

    shaders_.emplace_back(Shader(ctx_.dev, "color.vert", {}, VK_SHADER_STAGE_VERTEX_BIT, "COLOR VERT"));
    shaders_.emplace_back(Shader(ctx_.dev, "texture.vert", {}, VK_SHADER_STAGE_VERTEX_BIT, "TEXTURE VERT"));
    shaders_.emplace_back(Shader(ctx_.dev, "color.frag", {utilFragText.c_str()}, VK_SHADER_STAGE_FRAGMENT_BIT, "COLOR FRAG"));
    shaders_.emplace_back(Shader(ctx_.dev, "line.frag", {}, VK_SHADER_STAGE_FRAGMENT_BIT, "LINE FRAG"));
    shaders_.emplace_back(Shader(ctx_.dev, "texture.frag", {utilFragText.c_str()}, VK_SHADER_STAGE_FRAGMENT_BIT, "TEXTURE FRAG"));

    finalize_glslang();
}

void ShaderHandler::posLightReplace(uint32_t numLights, std::string& text) {
    text = textReplace(text, "PositionalLight lights[", "];", 1, numLights);
}

template <typename T1, typename T2>
std::string ShaderHandler::textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2) {
    std::string s = text;
    std::stringstream rss, nss;
    rss << s1 << r1 << s2;
    nss << s1 << r2 << s2;
    size_t f = s.find(rss.str());
    if (f != std::string::npos) s.replace(f, rss.str().length(), nss.str());
    return s;
}