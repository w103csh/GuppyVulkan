#ifndef UNIFORM_HANDLER_H
#define UNIFORM_HANDLER_H

#include <vulkan/vulkan.h>

#include "Game.h"
#include "Helpers.h"
#include "Uniform.h"

namespace Uniform {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    //// void updateDefaultUniform(Camera &camera);
    //// void updateDefaultDynamicUniform();

    //// DESCRIPTOR
    // VkDescriptorSetLayoutBinding getDescriptorLayoutBinding(const DESCRIPTOR &type);
    // void getDescriptorSetTypes(const SHADER &type, std::set<DESCRIPTOR> &set) {
    //    // inst_.getShader(type)->getDescriptorTypes(set);
    //}

    //// TODO: These names are terrible. Make a template type condition function.
    // uint16_t getDefUniformPositionLightCount() {
    //    return 0;
    //    // return pDefaultUniform_->getPositionLightCount();
    //}

    // uint16_t getDefUniformSpotLightCount() {
    //    return 0;
    //    // return pDefaultUniform_->getSpotLightCount();
    //}

    // template <typename T>
    // void defaultLightsAction(T &&updateFunc) {
    //    inst_.pDefaultUniform_->lightsAction(std::forward<T &&>(updateFunc));
    //}

    // template <typename T>
    // void defaultUniformAction(T &&updateFunc) {
    //    inst_.pDefaultUniform_->uniformAction(std::forward<T &&>(updateFunc));
    //}
};

}  // namespace Uniform

#endif  // !UNIFORM_HANDLER_H
