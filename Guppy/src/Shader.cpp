
#include <sstream>

#include "util.hpp"

#include "FileLoader.h"
#include "Scene.h"
#include "Shader.h"

// **********************
//      Shader
// **********************

// directory of shader files relative to the root of the repo
const std::string SHADER_DIR = "Guppy\\src\\shaders\\";
// file names
// link
const std::string UTIL_FRAG_FILENAME = "util.frag.glsl";
// fragment
const std::string COLOR_FRAG_FILENAME = "color.frag";
const std::string TEX_FRAG_FILENAME = "texture.frag";
const std::string LINE_FRAG_FILENAME = "line.frag";
// vertex
const std::string COLOR_VERT_FILENAME = "color.vert";
const std::string TEX_VERT_FILENAME = "texture.vert";

Shader::Shader(const VkDevice dev, std::string fileName, std::vector<const char*> pLinkTexts, VkShaderStageFlagBits stage,
               bool updateTextFromFile, std::string markerName)
    : info({}), module(VK_NULL_HANDLE), fileName_(fileName), markerName_(markerName), stage_(stage) {
    init(dev, pLinkTexts, true, std::vector<VkShaderModule>{}, updateTextFromFile);
}

void Shader::init(const VkDevice dev, std::vector<const char*> pLinkTexts, bool doAssert,
                  std::vector<VkShaderModule>& oldModules, bool updateTextFromFile) {
    bool isRecompile = (module != VK_NULL_HANDLE);

    // Check if shader needs to be (re)compiled
    if (updateTextFromFile) {
        text_ = FileLoader::readFile(SHADER_DIR + fileName_);
    }

    std::vector<const char*> pShaderTexts = {text_.c_str()};
    pShaderTexts.insert(pShaderTexts.end(), pLinkTexts.begin(), pLinkTexts.end());

    std::vector<unsigned int> spv;
    bool success = GLSLtoSPV(stage_, pShaderTexts, spv);

    // Return or assert on fail
    if (!success) {
        if (doAssert)
            assert(success);
        else
            return;
    }

    VkShaderModuleCreateInfo module_info = {};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spv.size() * sizeof(unsigned int);
    module_info.pCode = spv.data();

    VkShaderModule sm;
    vk::assert_success(vkCreateShaderModule(dev, &module_info, nullptr, &sm));
    // Store old module for clean up if necessary
    if (isRecompile) oldModules.push_back(std::move(module));
    module = std::move(sm);

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage_;
    stageInfo.pName = "main";
    stageInfo.module = module;
    info = std::move(stageInfo);

    // TODO: check settings
    if (!markerName_.empty()) {
        ext::DebugMarkerSetObjectName(dev, (uint64_t)module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
                                      (markerName_ + " shader module").c_str());
    }
}

// **********************
//      ShaderHandler
// **********************

ShaderHandler ShaderHandler::inst_;
ShaderHandler::ShaderHandler() {}

void ShaderHandler::reset() {
    for (auto& shader : shaders_) {
        if (shader->module) vkDestroyShaderModule(ctx_.dev, shader->module, nullptr);
    }
    inst_.cleanupOldResources();
}

void ShaderHandler::init(Shell& sh, const Game::Settings& settings, uint32_t numPosLights, uint32_t numSpotLights) {
    // Clean up owned memory...
    inst_.reset();

    inst_.ctx_ = sh.context();
    inst_.settings_ = settings;
    inst_.numPosLights_ = numPosLights;
    inst_.numSpotLights_ = numSpotLights;

    inst_.loadShaders({SHADER_TYPE::COLOR_FRAG, SHADER_TYPE::COLOR_VERT, SHADER_TYPE::LINE_FRAG, SHADER_TYPE::TEX_FRAG,
                       SHADER_TYPE::TEX_VERT},
                      true);

    // listen to changes to the shader files
    if (settings.enable_directory_listener) {
        sh.watchDirectory(SHADER_DIR, &ShaderHandler::recompileShader);
    }
}
void ShaderHandler::update(std::unique_ptr<Scene>& pScene) {
    for (; !inst_.updateQueue_.empty(); inst_.updateQueue_.pop()) {
        pScene->updatePipeline(inst_.updateQueue_.front());
    }
}

void ShaderHandler::recompileShader(std::string fileName) {
    bool assert = inst_.settings_.assert_on_recompile_shader;
    if (fileName == UTIL_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::COLOR_FRAG, SHADER_TYPE::TEX_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_COLOR);
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_TEX);
        };

    } else if (fileName == COLOR_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::COLOR_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_COLOR);
        }

    } else if (fileName == TEX_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::TEX_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_TEX);
        }

    } else if (fileName == LINE_FRAG_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::LINE_FRAG}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::LINE);
        }

    } else if (fileName == COLOR_VERT_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::COLOR_VERT}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_COLOR);
        }

    } else if (fileName == TEX_VERT_FILENAME) {
        if (inst_.loadShaders({SHADER_TYPE::TEX_VERT}, assert)) {
            inst_.updateQueue_.push(PIPELINE_TYPE::TRI_LIST_TEX);
        }
    }
}

bool ShaderHandler::loadShaders(std::vector<SHADER_TYPE> types, bool doAssert, bool updateFromFile) {
    if (types.empty()) return false;  // TODO: warn

    auto numOldResources = inst_.oldModules_.size();

    // Make sure the storage vector is big enough
    if (shaders_.size() < types.size()) shaders_.resize(types.size());

    // Link text flags
    bool utilFragLoaded = false;
    std::string fileName;
    std::vector<const char*> pLinkTexts;
    VkShaderStageFlagBits stage;

    init_glslang();

    for (auto& type : types) {
        int index = static_cast<int>(type);

        switch (type) {
            case SHADER_TYPE::COLOR_VERT: {
                fileName = COLOR_VERT_FILENAME;
                stage = VK_SHADER_STAGE_VERTEX_BIT;
                pLinkTexts = {};

            } break;

            case SHADER_TYPE::TEX_VERT: {
                fileName = TEX_VERT_FILENAME;
                stage = VK_SHADER_STAGE_VERTEX_BIT;
                pLinkTexts = {};

            } break;

            case SHADER_TYPE::COLOR_FRAG: {
                // load link text
                if (!utilFragLoaded) {
                    loadLinkText(SHADER_TYPE::UTIL_FRAG);
                    utilFragLoaded = true;
                }

                fileName = COLOR_FRAG_FILENAME;
                stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                pLinkTexts = {utilFragText_.c_str()};

            } break;

            case SHADER_TYPE::TEX_FRAG: {
                // load link text
                if (!utilFragLoaded) {
                    loadLinkText(SHADER_TYPE::UTIL_FRAG);
                    utilFragLoaded = true;
                }

                fileName = TEX_FRAG_FILENAME;
                stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                pLinkTexts = {utilFragText_.c_str()};

            } break;

            case SHADER_TYPE::LINE_FRAG: {
                fileName = LINE_FRAG_FILENAME;
                stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                pLinkTexts = {};

            } break;

            default:
                throw std::runtime_error("Unhandled shader type.");
                break;
        }

        if (!shaders_[index]) {
            // instantiate
            shaders_[index] = std::make_unique<Shader>(ctx_.dev, fileName, pLinkTexts, stage, updateFromFile, fileName);
        } else {
            // recompile
            shaders_[index]->init(ctx_.dev, pLinkTexts, doAssert, oldModules_, updateFromFile);
        }
    }

    finalize_glslang();

    return numOldResources < inst_.oldModules_.size();
}

void ShaderHandler::loadLinkText(SHADER_TYPE type, bool updateFromFile) {
    switch (type) {
        case SHADER_TYPE::UTIL_FRAG: {
            // update text from file
            if (updateFromFile) utilFragText_ = FileLoader::readFile(SHADER_DIR + UTIL_FRAG_FILENAME);
            lightMacroReplace("NUM_POS_LIGHTS", numPosLights_, utilFragText_);
            lightMacroReplace("NUM_SPOT_LIGHTS", numSpotLights_, utilFragText_);
        } break;

        default:
            throw std::runtime_error("Unhandled shader type for linking.");
            break;
    }
}

void ShaderHandler::cleanupOldResources() {
    for (auto& module : inst_.oldModules_) vkDestroyShaderModule(inst_.ctx_.dev, module, nullptr);
    inst_.oldModules_.clear();
}

void ShaderHandler::lightMacroReplace(std::string macroVariable, uint32_t numLights, std::string& text) {
    // TODO: fix the regex so that you don't have to read from file...
    std::stringstream ss;
    ss << "#define " << macroVariable << " ";
    text = textReplace(text, ss.str(), "", 0, numLights);
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