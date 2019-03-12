#ifndef SHADER_HANDLER_H
#define SHADER_HANDLER_H

#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Helpers.h"
#include "Shader.h"
#include "Texture.h"

namespace Shader {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() { reset(); }

    const std::unique_ptr<Shader::Base> &getShader(const SHADER &type) const;
    const std::unique_ptr<Shader::Link> &getLinkShader(const SHADER_LINK &type) const;

    void getStagesInfo(const std::set<SHADER> &types, std::vector<VkPipelineShaderStageCreateInfo> &stagesInfo) const;
    VkShaderStageFlags getStageFlags(const std::set<SHADER> &shaderTypes) const;

    void recompileShader(std::string);
    void appendLinkTexts(const std::set<SHADER_LINK> &linkTypes, std::vector<const char *> &texts) const;

    void cleanup();

   private:
    void reset() override;

    bool loadShaders(const std::vector<SHADER> &types, bool doAssert, bool load = true);

    // SHADERS
    void getShaderTypes(const SHADER_LINK &linkType, std::vector<SHADER> &types);
    std::vector<std::unique_ptr<Shader::Base>> pShaders_;
    std::vector<std::unique_ptr<Shader::Link>> pLinkShaders_;

    std::queue<PIPELINE> updateQueue_;
    std::vector<VkShaderModule> oldModules_;
};

}  // namespace Shader

#endif  // !SHADER_HANDLER_H