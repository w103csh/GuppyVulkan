#ifndef SHADER_HANDLER_H
#define SHADER_HANDLER_H

#include <map>
#include <queue>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Game.h"

namespace Shader {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() override { reset(); }

    void getStagesInfo(const shaderInfoMapKey &key, std::vector<VkPipelineShaderStageCreateInfo> &stagesInfo) const;
    VkShaderStageFlags getStageFlags(const std::set<SHADER> &shaderTypes) const;

    void recompileShader(std::string);

    void cleanup();

   private:
    void reset() override;

    bool make(shaderInfoMapKeyValue &keyValue, bool doAssert, bool isInit);
    std::vector<std::string> loadText(const shaderInfoMapKey &keyValue);
    void textReplace(const descSetMacroSlotMap &slotMap, std::string &text) const;

    // SHADERS
    void getShaderTypes(const SHADER_LINK &linkType, std::vector<SHADER> &types);

    bool clearTextsAfterLoad;
    std::map<SHADER, std::string> shaderTexts_;
    std::map<SHADER_LINK, std::string> shaderLinkTexts_;

    shaderInfoMap shaderInfoMap_;

    std::queue<PIPELINE> updateQueue_;
    std::vector<VkShaderModule> oldModules_;
};

}  // namespace Shader

#endif  // !SHADER_HANDLER_H