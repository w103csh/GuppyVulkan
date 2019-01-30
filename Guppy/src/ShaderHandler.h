#ifndef SHADER_HANDLER_H
#define SHADER_HANDLER_H

#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Helpers.h"
#include "Shader.h"
#include "Texture.h"
#include "Uniform.h"

class Camera;

namespace Shader {

// **********************
//      Handler
// **********************

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() { reset(); }

    const std::unique_ptr<Shader::Base> &getShader(const SHADER &type) const;
    const std::unique_ptr<Shader::Link> &getLinkShader(const SHADER_LINK &type) const;

    void getStagesInfo(const std::set<SHADER> &types, std::vector<VkPipelineShaderStageCreateInfo> &stagesInfo) const;
    VkShaderStageFlags getStageFlags(const std::set<SHADER> &shaderTypes) const;

    void lightMacroReplace(std::string &&macroVariable, uint16_t &&numLights, std::string &text);
    void recompileShader(std::string);
    void appendLinkTexts(const std::set<SHADER_LINK> &linkTypes, std::vector<const char *> &texts) const;

    // DESCRIPTOR
    void updateDescriptorSets(const std::set<DESCRIPTOR> types, std::vector<VkDescriptorSet> &sets, uint32_t setCount,
                              const std::shared_ptr<Texture::DATA> &pTexture = nullptr) const;
    std::vector<uint32_t> getDynamicOffsets(const std::set<DESCRIPTOR> types) const;

    void cleanup();

   private:
    void reset() override;

    bool loadShaders(const std::vector<SHADER> &types, bool doAssert, bool load = true);

    template <typename T1, typename T2>
    std::string textReplace(std::string text, std::string s1, std::string s2, T1 r1, T2 r2);

    // SHADERS
    void getShaderTypes(const SHADER_LINK &linkType, std::vector<SHADER> &types);
    std::vector<std::unique_ptr<Shader::Base>> pShaders_;
    std::vector<std::unique_ptr<Shader::Link>> pLinkShaders_;

    std::queue<PIPELINE> updateQueue_;
    std::vector<VkShaderModule> oldModules_;

    // // **********************
    // //      UNIFORM BUFFER
    // // **********************
    // public:
    // void updateDefaultUniform(Camera &camera);
    // void updateDefaultDynamicUniform();

    // // DESCRIPTOR
    // VkDescriptorSetLayoutBinding getDescriptorLayoutBinding(const DESCRIPTOR &type);
    // void getDescriptorSetTypes(const SHADER &type, std::set<DESCRIPTOR> &set) {
    //     getShader(type)->getDescriptorTypes(set);
    // }

    // // TODO: These names are terrible. Make a template type condition function.
    // uint16_t getDefUniformPositionLightCount() { return pDefaultUniform_->getPositionLightCount(); }
    // uint16_t getDefUniformSpotLightCount() { return pDefaultUniform_->getSpotLightCount(); }

    // template <typename T>
    // void defaultLightsAction(T &&updateFunc) {
    //     pDefaultUniform_->lightsAction(std::forward<T &&>(updateFunc));
    // }

    // template <typename T>
    // void defaultUniformAction(T &&updateFunc) {
    //     pDefaultUniform_->uniformAction(std::forward<T &&>(updateFunc));
    // }

    // private:
    // std::unique_ptr<Uniform::Default> pDefaultUniform_;
    // std::unique_ptr<Uniform::DefaultDynamic> pDefaultDynamicUniform_;
};

}  // namespace Shader

#endif  // !SHADER_HANDLER_H