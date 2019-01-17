#ifndef SHADER_H
#define SHADER_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Shell.h"

namespace Shader {

// directory of shader files relative to the root of the repo
static const std::string BASE_DIRNAME = "Guppy\\src\\shaders\\";

// **********************
//      Base
// **********************

class Base {
   public:
    Base(SHADER_TYPE &&type, const std::string &fileName, VkShaderStageFlagBits &&stage, std::string &&name,
         std::vector<DESCRIPTOR_TYPE> &&descTypes = {}, std::vector<SHADER_TYPE> &&linkTypes = {})
        : info(),
          module(VK_NULL_HANDLE),
          DESCRIPTOR_TYPES(descTypes),
          LINK_TYPES(linkTypes),
          TYPE(type),
          fileName_(fileName),
          name_(name),
          stage_(stage){};

    virtual void init(const VkDevice &dev, const Game::Settings &settings, std::vector<VkShaderModule> &oldModules,
                      bool load = true, bool doAssert = true);

    virtual const char *loadText(bool load);

    const std::vector<SHADER_TYPE> LINK_TYPES;
    const std::vector<DESCRIPTOR_TYPE> DESCRIPTOR_TYPES;  // TODO: this naming convention is confusing
    const SHADER_TYPE TYPE;

    // DESCRIPTOR
    void getDescriptorTypes(std::set<DESCRIPTOR_TYPE> &set) {
        for (const auto &descType : DESCRIPTOR_TYPES) set.insert(descType);
    }

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

    virtual void getDescriptorBindings(std::vector<VkDescriptorSetLayoutBinding> &bindings){};

    virtual void destroy(const VkDevice &dev);

   protected:
    std::string text_;

   private:
    std::vector<SHADER_TYPE> linkShaders_;
    std::string fileName_;
    std::string name_;
    VkShaderStageFlagBits stage_;
};

// **********************
//      Link Shaders
// **********************

// Utility Fragement Shader
const std::string UTIL_FRAG_FILENAME = "util.frag.glsl";
class UtilityFragment : public Base {
   public:
    UtilityFragment()
        : Base(SHADER_TYPE::UTIL_FRAG, UTIL_FRAG_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, "Utility Fragement Shader",
               {DESCRIPTOR_TYPE::DEFAULT_UNIFORM}) {}

    void init(const VkDevice &dev, const Game::Settings &settings, std::vector<VkShaderModule> &oldModules, bool load = true,
              bool doAssert = true) override {
        loadText(load);
    };

    virtual const char *loadText(bool load) override;
};

// **********************
//      Default Shaders
// **********************

// Default Color Vertex
const std::string DEFAULT_COLOR_VERT_FILENAME = "color.vert";
class DefaultColorVertex : public Base {
   public:
    DefaultColorVertex()
        : Base(SHADER_TYPE::COLOR_VERT, DEFAULT_COLOR_VERT_FILENAME, VK_SHADER_STAGE_VERTEX_BIT,
               "Default Color Vertex Shader", {DESCRIPTOR_TYPE::DEFAULT_UNIFORM}) {}
};

// Default Color Fragment
const std::string DEFAULT_COLOR_FRAG_FILENAME = "color.frag";
class DefaultColorFragment : public Base {
   public:
    DefaultColorFragment()
        : Base(SHADER_TYPE::COLOR_FRAG, DEFAULT_COLOR_FRAG_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Color Fragment Shader", {}, {SHADER_TYPE::UTIL_FRAG}) {}
};

// Default Line Fragment
const std::string DEFAULT_LINE_FRAG_FILENAME = "line.frag";
class DefaultLineFragment : public Base {
   public:
    DefaultLineFragment()
        : Base(SHADER_TYPE::LINE_FRAG, DEFAULT_LINE_FRAG_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Line Fragment Shader", {}) {}
};

// Default Texture Vertex
const std::string DEFAULT_TEX_VERT_FILENAME = "texture.vert";
class DefaultTextureVertex : public Base {
   public:
    DefaultTextureVertex()
        : Base(SHADER_TYPE::TEX_VERT, DEFAULT_TEX_VERT_FILENAME, VK_SHADER_STAGE_VERTEX_BIT, "Default Texture Vertex Shader",
               {DESCRIPTOR_TYPE::DEFAULT_UNIFORM}) {}
};

// Default Texture Fragment
const std::string DEFAULT_TEX_FRAG_FILENAME = "texture.frag";
class DefaultTextureFragment : public Base {
   public:
    DefaultTextureFragment()
        : Base(SHADER_TYPE::TEX_FRAG, DEFAULT_TEX_FRAG_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT,
               "Default Texture Fragment Shader",
               {DESCRIPTOR_TYPE::DEFAULT_SAMPLER, DESCRIPTOR_TYPE::DEFAULT_DYNAMIC_UNIFORM}, {SHADER_TYPE::UTIL_FRAG}) {}
};

}  // namespace Shader

#endif  // !SHADER_H