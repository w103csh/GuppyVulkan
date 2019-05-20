#ifndef SHADER_H
#define SHADER_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "DescriptorSet.h"
#include "Handlee.h"
#include "Game.h"
#include "Shell.h"

namespace Shader {

// directory of shader files relative to the root of the repo
const std::string BASE_DIRNAME = "shaders/";

// **********************
//      Base
// **********************

class Base : public Handlee<Shader::Handler> {
   public:
    virtual ~Base() = default;

    const SHADER TYPE;
    const std::string FILE_NAME;
    const VkShaderStageFlagBits STAGE;
    const std::string NAME;
    const std::set<SHADER_LINK> LINK_TYPES;

    virtual void init(std::vector<VkShaderModule> &oldModules, bool load = true, bool doAssert = true);

    virtual const char *loadText(bool load);

    virtual void destroy();

    VkPipelineShaderStageCreateInfo info;
    VkShaderModule module;

   protected:
    Base(Shader::Handler &handler, const SHADER &&type, const std::string &fileName, const VkShaderStageFlagBits &&stage,
         const std::string &&name, const std::set<SHADER_LINK> &&linkTypes = {})
        : Handlee(handler),
          TYPE(type),
          FILE_NAME(fileName),
          STAGE(stage),
          NAME(name),
          LINK_TYPES(linkTypes),
          info(),
          module(VK_NULL_HANDLE){};

    std::string text_;
};

// **********************
//      Link Shaders
// **********************

namespace Link {

class Base : public Shader::Base {
   public:
    // TODO: get rid of LINK_TYPE
    const SHADER_LINK LINK_TYPE;

    void init(std::vector<VkShaderModule> &oldModules, bool load = true, bool doAssert = true) override;

   protected:
    Base(Shader::Handler &handler, const SHADER_LINK &&type, const std::string &fileName, const std::string &&name,
         const std::set<SHADER_LINK> &&linkTypes = {})
        : Shader::Base{handler,
                       SHADER::LINK,
                       fileName,
                       static_cast<VkShaderStageFlagBits>(0),
                       std::forward<const std::string>(name),
                       std::forward<const std::set<SHADER_LINK>>(linkTypes)},
          LINK_TYPE(type) {}
};

class ColorFragment : public Shader::Link::Base {
   public:
    ColorFragment(Shader::Handler &handler);
};

class TextureFragment : public Shader::Link::Base {
   public:
    TextureFragment(Shader::Handler &handler);
};

class BlinnPhongFragment : public Shader::Link::Base {
   public:
    BlinnPhongFragment(Shader::Handler &handler);
};

namespace Default {
class Material : public Shader::Link::Base {
   public:
    Material(Shader::Handler &handler);
};
}  // namespace Default

}  // namespace Link

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

class Cube : public Base {
   public:
    Cube(Shader::Handler &handler);
};

}  // namespace Default

}  // namespace Shader

#endif  // !SHADER_H