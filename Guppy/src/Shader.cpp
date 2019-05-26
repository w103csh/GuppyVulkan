//
//#include "Shader.h"
//
//#include "DescriptorHandler.h"
//#include "FileLoader.h"
//#include "PipelineHandler.h"
//#include "ShaderHandler.h"
//#include "UniformHandler.h"
//#include "util.hpp"
//
//// **********************
////      Base
//// **********************
//
//void Shader::Base::init(std::vector<VkShaderModule>& oldModules, bool load, bool doAssert) {
//    bool isRecompile = (module != VK_NULL_HANDLE);
//
//    // Check if shader needs to be (re)compiled
//    std::vector<const char*> shaderTexts = {loadText(load)};
//
//    // GET DESC SET SLOT INFO
//    // shaderInfoTuple infoTuple = {};
//    // handler()
//    //    .pipelineHandler().get
//
//    handler().appendLinkTexts(LINK_TYPES, shaderTexts);
//
//    std::vector<unsigned int> spv;
//    bool success = GLSLtoSPV(STAGE, shaderTexts, spv);
//
//    // Return or assert on fail
//    if (!success) {
//        if (doAssert) {
//            if (!success) handler().shell().log(Shell::LOG_ERR, ("Error compiling: " + FILE_NAME).c_str());
//            assert(success);
//        } else {
//            return;
//        }
//    }
//
//    VkShaderModuleCreateInfo moduleInfo = {};
//    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//    moduleInfo.codeSize = spv.size() * sizeof(unsigned int);
//    moduleInfo.pCode = spv.data();
//
//    // Store old module for clean up if necessary
//    if (module != VK_NULL_HANDLE) oldModules.push_back(std::move(module));
//
//    auto& dev = handler().shell().context().dev;
//    vk::assert_success(vkCreateShaderModule(dev, &moduleInfo, nullptr, &module));
//
//    VkPipelineShaderStageCreateInfo stageInfo = {};
//    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    stageInfo.stage = STAGE;
//    stageInfo.pName = "main";
//    stageInfo.module = module;
//    info = std::move(stageInfo);
//
//    if (handler().settings().enable_debug_markers) {
//        std::string markerName = NAME + "shader module";
//        ext::DebugMarkerSetObjectName(dev, (uint64_t)module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
//                                      markerName.c_str());
//    }
//}
//
//const char* Shader::Base::loadText(bool load) {
//    if (load) text_ = FileLoader::readFile(BASE_DIRNAME + FILE_NAME);
//    handler().uniformHandler().shaderTextReplace(text_);
//    return text_.c_str();
//}
//
//void Shader::Base::destroy() {}
//
//// **********************
////      LINK SHADERS
//// **********************
//
//void Shader::Link::Base::init(std::vector<VkShaderModule>& oldModules, bool load, bool doAssert) { loadText(load); };
//
//Shader::Link::ColorFragment::ColorFragment(Shader::Handler& handler)
//    : Shader::Link::Base{
//          handler,                  //
//          SHADER_LINK::COLOR_FRAG,  //
//          "link.color.frag",        //
//          "Link Color Fragment",    //
//      } {}
//
//Shader::Link::TextureFragment::TextureFragment(Shader::Handler& handler)
//    : Shader::Link::Base{
//          handler,                  //
//          SHADER_LINK::TEX_FRAG,    //
//          "link.texture.frag",      //
//          "Link Texture Fragment",  //
//      } {}
//
//Shader::Link::UtilityVertex::UtilityVertex(Shader::Handler& handler)
//    : Shader::Link::Base{
//          handler,                       //
//          SHADER_LINK::UTILITY_VERT,     //
//          "link.utility.vert.glsl",      //
//          "Link Utility Vertex Shader",  //
//      } {}
//
//Shader::Link::UtilityFragment::UtilityFragment(Shader::Handler& handler)
//    : Shader::Link::Base{
//          handler,                         //
//          SHADER_LINK::UTILITY_FRAG,       //
//          "link.utility.frag.glsl",        //
//          "Link Utility Fragment Shader",  //
//      } {}
//
//Shader::Link::BlinnPhong::BlinnPhong(Shader::Handler& handler)
//    : Shader::Link::Base{
//          handler,                    //
//          SHADER_LINK::BLINN_PHONG,   //
//          "link.blinnPhong.glsl",     //
//          "Link Blinn Phong Shader",  //
//      } {}
//
//Shader::Link::Default::Material::Material(Shader::Handler& handler)
//    : Shader::Link::Base{
//          handler,                         //
//          SHADER_LINK::DEFAULT_MATERIAL,   //
//          "link.default.material.glsl",    //
//          "Link Default Material Shader",  //
//      } {}
//
//// **********************
////      DEFAULT SHADERS
//// **********************
//
//Shader::Default::ColorVertex::ColorVertex(Shader::Handler& handler)
//    : Base{
//          handler,                        //
//          SHADER::COLOR_VERT,             //
//          "color.vert",                   //
//          VK_SHADER_STAGE_VERTEX_BIT,     //
//          "Default Color Vertex Shader",  //
//          {SHADER_LINK::UTILITY_VERT},
//      } {};
//
//Shader::Default::ColorFragment::ColorFragment(Shader::Handler& handler)
//    : Base{
//          handler,
//          SHADER::COLOR_FRAG,
//          "color.frag",
//          VK_SHADER_STAGE_FRAGMENT_BIT,
//          "Default Color Fragment Shader",
//          {
//              SHADER_LINK::COLOR_FRAG,
//              SHADER_LINK::UTILITY_FRAG,
//              SHADER_LINK::BLINN_PHONG,
//              SHADER_LINK::DEFAULT_MATERIAL,
//          },
//      } {}
//
//Shader::Default::LineFragment::LineFragment(Shader::Handler& handler)
//    : Base{
//          handler,                         //
//          SHADER::LINE_FRAG,               //
//          "line.frag",                     //
//          VK_SHADER_STAGE_FRAGMENT_BIT,    //
//          "Default Line Fragment Shader",  //
//      } {}
//
//Shader::Default::TextureVertex::TextureVertex(Shader::Handler& handler)
//    : Base{
//          handler,                          //
//          SHADER::TEX_VERT,                 //
//          "texture.vert",                   //
//          VK_SHADER_STAGE_VERTEX_BIT,       //
//          "Default Texture Vertex Shader",  //
//      } {}
//
//Shader::Default::TextureFragment::TextureFragment(Shader::Handler& handler)
//    : Base{
//          handler,
//          SHADER::TEX_FRAG,
//          "texture.frag",
//          VK_SHADER_STAGE_FRAGMENT_BIT,
//          "Default Texture Fragment Shader",
//          {
//              SHADER_LINK::TEX_FRAG,
//              SHADER_LINK::UTILITY_FRAG,
//              SHADER_LINK::BLINN_PHONG,
//              SHADER_LINK::DEFAULT_MATERIAL,
//          },
//      } {}
//
//Shader::Default::CubeVertex::CubeVertex(Shader::Handler& handler)
//    : Base{
//          handler,                          //
//          SHADER::CUBE_VERT,                //
//          "cube.vert",                      //
//          VK_SHADER_STAGE_VERTEX_BIT,       //
//          "Cube Vertex Shader",             //
//          {SHADER_LINK::DEFAULT_MATERIAL},  //
//      } {}
//
//Shader::Default::CubeFragment::CubeFragment(Shader::Handler& handler)
//    : Base{
//          handler,                          //
//          SHADER::CUBE_FRAG,                //
//          "cube.frag",                      //
//          VK_SHADER_STAGE_FRAGMENT_BIT,     //
//          "Cube Fragment Shader",           //
//          {SHADER_LINK::DEFAULT_MATERIAL},  //
//      } {}
