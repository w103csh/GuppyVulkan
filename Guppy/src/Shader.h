#ifndef SHADER_H
#define SHADER_H

#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Shell.h"

namespace Shader {

// directory of shader files relative to the root of the repo
const std::string BASE_DIRNAME = "Guppy\\src\\shaders\\";

// **********************
//      Base
// **********************

class Base {
   public:
    Base(const SHADER &&type, const std::string &fileName, const VkShaderStageFlagBits &&stage, const std::string &&name,
         const std::vector<DESCRIPTOR> &&descTypes = {}, const std::set<SHADER_LINK> &&linkTypes = {})
        : TYPE(type),
          info(),
          module(VK_NULL_HANDLE),
          DESCRIPTOR_TYPES(descTypes),
          LINK_TYPES(linkTypes),
          FILE_NAME(fileName),
          NAME(name),
          STAGE(stage){};

    const SHADER TYPE;
    const std::string FILE_NAME;
    const VkShaderStageFlagBits STAGE;
    const std::string NAME;
    const std::vector<DESCRIPTOR> DESCRIPTOR_TYPES;
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
    Link(const SHADER_LINK &&type, const std::string &fileName, const VkShaderStageFlagBits &&stage,
         const std::string &&name, const std::vector<DESCRIPTOR> &&descTypes = {},
         const std::set<SHADER_LINK> &&linkTypes = {})
        : Base{SHADER::LINK,
               fileName,
               std::forward<const VkShaderStageFlagBits>(stage),
               std::forward<const std::string>(name),
               std::forward<const std::vector<DESCRIPTOR>>(descTypes),
               std::forward<const std::set<SHADER_LINK>>(linkTypes)},
          LINK_TYPE(type) {}
};

// Utility Fragement Shader
const std::string UTIL_FRAG_FILENAME = "util.frag.glsl";
class UtilityFragment : public Link {
   public:
    UtilityFragment()
        : Link{SHADER_LINK::UTIL_FRAG,
               UTIL_FRAG_FILENAME,
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Utility Fragement Shader",
               {DESCRIPTOR::DEFAULT_UNIFORM}} {}

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
    ColorVertex()
        : Base{//
               SHADER::COLOR_VERT,
               "color.vert",
               VK_SHADER_STAGE_VERTEX_BIT,
               "Default Color Vertex Shader",
               {DESCRIPTOR::DEFAULT_UNIFORM}} {}
};

class ColorFragment : public Base {
   public:
    ColorFragment()
        : Base{                     //
               SHADER::COLOR_FRAG,  //
               "color.frag",
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Color Fragment Shader",
               {},
               {SHADER_LINK::UTIL_FRAG}} {}
};

class LineFragment : public Base {
   public:
    LineFragment()
        : Base{//
               SHADER::LINE_FRAG,
               "line.frag",
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Line Fragment Shader",
               {}} {}
};

class TextureVertex : public Base {
   public:
    TextureVertex()
        : Base{//
               SHADER::TEX_VERT,
               "texture.vert",
               VK_SHADER_STAGE_VERTEX_BIT,
               "Default Texture Vertex Shader",
               {DESCRIPTOR::DEFAULT_UNIFORM}} {}
};

class TextureFragment : public Base {
   public:
    TextureFragment()
        : Base{//
               SHADER::TEX_FRAG,
               "texture.frag",
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Texture Fragment Shader",
               {DESCRIPTOR::DEFAULT_SAMPLER, DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM},
               {SHADER_LINK::UTIL_FRAG}} {}
};
}  // namespace Default

}  // namespace Shader

#endif  // !SHADER_H