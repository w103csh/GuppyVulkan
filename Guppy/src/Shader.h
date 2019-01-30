#ifndef SHADER_H
#define SHADER_H

#include <set>
#include <string>
#include <map>
#include <vector>
#include <vulkan/vulkan.h>

#include "Handlee.h"
#include "Helpers.h"
#include "Game.h"
#include "Shell.h"

namespace Shader {

// directory of shader files relative to the root of the repo
const std::string BASE_DIRNAME = "Guppy\\src\\shaders\\";

// **********************
//      Base
// **********************

class Base : public Handlee<Shader::Handler> {
   public:
    Base(const Shader::Handler &handler, const SHADER &&type, const std::string &fileName,
         const VkShaderStageFlagBits &&stage, const std::string &&name,
         const std::map<uint32_t, UNIFORM> &&bindingUniformMap = {}, const std::set<SHADER_LINK> &&linkTypes = {})
        : Handlee(handler),
          TYPE(type),
          info(),
          module(VK_NULL_HANDLE),
          BINDING_UNIFORM_MAP(bindingUniformMap),
          LINK_TYPES(linkTypes),
          FILE_NAME(fileName),
          NAME(name),
          STAGE(stage){};

    const SHADER TYPE;
    const std::string FILE_NAME;
    const VkShaderStageFlagBits STAGE;
    const std::string NAME;
    const std::vector<DESCRIPTOR> DESCRIPTOR_TYPES;
    const std::map<uint32_t, UNIFORM> BINDING_UNIFORM_MAP;
    const std::set<SHADER_LINK> LINK_TYPES;

    virtual void init(const VkDevice &dev, const Game::Settings &settings, std::vector<VkShaderModule> &oldModules,
                      bool load = true, bool doAssert = true);

    virtual const char *loadText(bool load);

    // DESCRIPTOR
    void getDescriptorTypes(std::set<DESCRIPTOR> &set) {
        for (const auto &descType : DESCRIPTOR_TYPES) set.insert(descType);
    }

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

    virtual void getDescriptorBindings(std::vector<VkDescriptorSetLayoutBinding> &bindings){};

    virtual void destroy(const VkDevice &dev);

   protected:
    std::string text_;
};

// **********************
//      Link Shaders
// **********************

class Link : public Base {
   public:
    const SHADER_LINK LINK_TYPE;

   protected:
    Link(const Shader::Handler &handler, const SHADER_LINK &&type, const std::string &fileName,
         const VkShaderStageFlagBits &&stage, const std::string &&name,
         const std::map<uint32_t, UNIFORM> &&bindingUniformMap = {}, const std::set<SHADER_LINK> &&linkTypes = {})
        : Base{handler,
               SHADER::LINK,
               fileName,
               std::forward<const VkShaderStageFlagBits>(stage),
               std::forward<const std::string>(name),
               std::forward<const std::map<uint32_t, UNIFORM>>(bindingUniformMap),
               std::forward<const std::set<SHADER_LINK>>(linkTypes)},
          LINK_TYPE(type) {}
};

// Utility Fragement Shader
const std::string UTIL_FRAG_FILENAME = "util.frag.glsl";
class UtilityFragment : public Link {
   public:
    UtilityFragment(const Shader::Handler &handler)
        : Link{handler,
               SHADER_LINK::UTIL_FRAG,
               UTIL_FRAG_FILENAME,
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Utility Fragement Shader",
               {{0, UNIFORM::CAMERA_PERSPECTIVE_DEFAULT},
                {1, UNIFORM::MATERIAL_DEFAULT},
                {2, UNIFORM::FOG_DEFAULT},
                {3, UNIFORM::LIGHT_POSITIONAL_DEFAULT},
                {4, UNIFORM::LIGHT_SPOT_DEFAULT}}} {}

    void init(const VkDevice &dev, const Game::Settings &settings, std::vector<VkShaderModule> &oldModules, bool load = true,
              bool doAssert = true) override {
        loadText(load);
    };

    virtual const char *loadText(bool load) override;
};

// **********************
//      Default Shaders
// **********************

namespace Default {

class ColorVertex : public Base {
   public:
    ColorVertex(const Shader::Handler &handler)
        : Base{handler,  //
               SHADER::COLOR_VERT,
               "color.vert",
               VK_SHADER_STAGE_VERTEX_BIT,
               "Default Color Vertex Shader",
               {{0, UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}}} {}
};

class ColorFragment : public Base {
   public:
    ColorFragment(const Shader::Handler &handler)
        : Base{handler,             //
               SHADER::COLOR_FRAG,  //
               "color.frag",
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Color Fragment Shader",
               {},
               {SHADER_LINK::UTIL_FRAG}} {}
};

class LineFragment : public Base {
   public:
    LineFragment(const Shader::Handler &handler)
        : Base{handler,  //
               SHADER::LINE_FRAG, "line.frag", VK_SHADER_STAGE_FRAGMENT_BIT, "Default Line Fragment Shader", {}} {}
};

class TextureVertex : public Base {
   public:
    TextureVertex(const Shader::Handler &handler)
        : Base{handler,  //
               SHADER::TEX_VERT,
               "texture.vert",
               VK_SHADER_STAGE_VERTEX_BIT,
               "Default Texture Vertex Shader",
               {{0, UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}}} {}
};

class TextureFragment : public Base {
   public:
    TextureFragment(const Shader::Handler &handler)
        : Base{handler,  //
               SHADER::TEX_FRAG,
               "texture.frag",
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Texture Fragment Shader",
               {{1, UNIFORM::SAMPLER_DEFAULT}, {2, UNIFORM::MATERIAL_DEFAULT}},
               {SHADER_LINK::UTIL_FRAG}} {}
};

}  // namespace Default

}  // namespace Shader

#endif  // !SHADER_H