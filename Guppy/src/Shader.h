#ifndef SHADER_H
#define SHADER_H

#include <set>
#include <string>
#include <list>
#include <vector>
#include <vulkan/vulkan.h>

#include "DescriptorSet.h"
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
    Base(Shader::Handler &handler, const SHADER &&type, const std::string &fileName, const VkShaderStageFlagBits &&stage,
         const std::string &&name, const std::list<DESCRIPTOR_SET> &&descriptorSets = {},
         const std::set<SHADER_LINK> &&linkTypes = {})
        : Handlee(handler),
          TYPE(type),
          FILE_NAME(fileName),
          STAGE(stage),
          NAME(name),
          DESCRIPTOR_SETS(descriptorSets),
          LINK_TYPES(linkTypes),
          info(),
          module(VK_NULL_HANDLE){};

    const SHADER TYPE;
    const std::string FILE_NAME;
    const VkShaderStageFlagBits STAGE;
    const std::string NAME;
    const std::list<DESCRIPTOR_SET> DESCRIPTOR_SETS;
    const std::set<SHADER_LINK> LINK_TYPES;

    virtual void init(std::vector<VkShaderModule> &oldModules, bool load = true, bool doAssert = true);

    virtual const char *loadText(bool load);

    virtual void destroy();

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

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
    Link(Shader::Handler &handler, const SHADER_LINK &&type, const std::string &fileName, const std::string &&name,
         const std::list<DESCRIPTOR_SET> &&descriptorSets = {}, const std::set<SHADER_LINK> &&linkTypes = {})
        : Base{handler,
               SHADER::LINK,
               fileName,
               static_cast<VkShaderStageFlagBits>(0),
               std::forward<const std::string>(name),
               std::forward<const std::list<DESCRIPTOR_SET>>(descriptorSets),
               std::forward<const std::set<SHADER_LINK>>(linkTypes)},
          LINK_TYPE(type) {}
};

// Utility Fragement Shader
class UtilityFragment : public Link {
   public:
    UtilityFragment(Shader::Handler &handler);

    void init(std::vector<VkShaderModule> &oldModules, bool load = true, bool doAssert = true) override;
};

// **********************
//      Default Shaders
// **********************

namespace Default {

class ColorVertex : public Base {
   public:
    ColorVertex(Shader::Handler &handler);
};

class ColorFragment : public Base {
   public:
    ColorFragment(Shader::Handler &handler);
};

class LineFragment : public Base {
   public:
    LineFragment(Shader::Handler &handler);
};

class TextureVertex : public Base {
   public:
    TextureVertex(Shader::Handler &handler);
};

class TextureFragment : public Base {
   public:
    TextureFragment(Shader::Handler &handler);
};

}  // namespace Default

}  // namespace Shader

#endif  // !SHADER_H