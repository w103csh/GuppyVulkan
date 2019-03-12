
#ifndef UNIFORM_HANDLER_H
#define UNIFORM_HANDLER_H

#include <functional>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "BufferManagerDescriptor.h"
#include "Camera.h"
#include "Constants.h"
#include "Helpers.h"
#include "Light.h"
#include "PBR.h"
#include "Uniform.h"

namespace Uniform {

typedef uint8_t UNIFORM_INDEX;
const auto UNIFORM_MAX_COUNT = UINT8_MAX;

// **********************
//      Manager
// **********************

// TODO: inner class of Handler?
template <class TDerived>
class Manager : public Buffer::Manager::Descriptor<Uniform::Base, TDerived> {
   public:
    Manager(const std::string&& name, const DESCRIPTOR&& descriptorType, const UNIFORM_INDEX&& maxSize,
            const std::string&& macroName)
        : Buffer::Manager::Descriptor<Uniform::Base, TDerived>{
              std::forward<const std::string>(name),
              std::forward<const DESCRIPTOR>(descriptorType),
              std::forward<const UNIFORM_INDEX>(maxSize),
              true,
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
              static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
              std::forward<const std::string>(macroName),
              VK_SHARING_MODE_EXCLUSIVE,
              0} {}

    void init(const Shell::Context& ctx, const Game::Settings& settings,
              std::vector<uint32_t> queueFamilyIndices = {}) override {
        const auto& limits = ctx.physical_dev_props[ctx.physical_dev_index].properties.limits;
        assert(sizeof TDerived::DATA % limits.minUniformBufferOffsetAlignment == 0);
        // size = (size + limits.minUniformBufferOffsetAlignment - 1) & ~(limits.minUniformBufferOffsetAlignment - 1);
        Buffer::Manager::Base<Uniform::Base, TDerived>::init(ctx, settings,
                                                             std::forward<std::vector<uint32_t>>(queueFamilyIndices));
    }
};

// **********************
//      Handler
// **********************

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;

    // DESCRIPTOR
    uint32_t getDescriptorCount(const Descriptor::bindingMapValue& value);
    bool validatePipelineLayout(const Descriptor::bindingMap& map);
    bool validateUniformOffsets(const std::pair<DESCRIPTOR, UNIFORM_INDEX>& keyValue);
    std::vector<VkDescriptorBufferInfo> getWriteInfos(const Descriptor::bindingMapValue& value);

    void createVisualHelpers();
    void update();

    // MAIN CAMERA
    Camera::Default::Perspective::Base& getMainCamera() {
        return std::ref(*(Camera::Default::Perspective::Base*)(camDefPersMgr_.pItems[MAIN_CAMERA_OFFSET].get()));
    }

    // SHADER
    std::string macroReplace(const Descriptor::bindingMap& bindingMap, std::string text);

    template <typename T>
    inline T& getUniform(const DESCRIPTOR& type, const UNIFORM_INDEX& index) {
        auto& uniforms = getUniforms(type);
        assert(index < uniforms.size());
        return std::ref(*(T*)(uniforms[index].get()));
    }

    template <class T>
    inline void update(T& uniform) {
        getManager<T>().update(shell().context().dev, uniform.BUFFER_INFO);
    }

   private:
    void reset() override;

    // clang-format off
    template <class T> inline Uniform::Manager<T>& getManager() { static_assert(false, "Not implemented"); }
    template <> inline Uniform::Manager<Camera::Default::Perspective::Base>& getManager() { return camDefPersMgr_; }
    template <> inline Uniform::Manager<Light::Default::Positional::Base>& getManager() { return lgtDefPosMgr_; }
    template <> inline Uniform::Manager<Light::PBR::Positional::Base>& getManager() { return lgtPbrPosMgr_; }
    template <> inline Uniform::Manager<Light::Default::Spot::Base>& getManager() { return lgtDefSptMgr_; }
    template <> inline Uniform::Manager<Uniform::Default::Fog::Base>& getManager() { return uniDefFogMgr_; }
    // clang-format on

    std::set<uint32_t> getBindingOffsets(const Descriptor::bindingMapValue& value);

    inline const std::string& getMacroName(const DESCRIPTOR& type) const {
        switch (type) {
            case DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT:
                return camDefPersMgr_.MACRO_NAME;
            case DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT:
                return lgtDefPosMgr_.MACRO_NAME;
            case DESCRIPTOR::LIGHT_POSITIONAL_PBR:
                return lgtPbrPosMgr_.MACRO_NAME;
            case DESCRIPTOR::LIGHT_SPOT_DEFAULT:
                return lgtDefSptMgr_.MACRO_NAME;
            case DESCRIPTOR::FOG_DEFAULT:
                return uniDefFogMgr_.MACRO_NAME;
            default:
                assert(false);
                throw std::runtime_error("Unhandled uniform type");
        }
    }

    inline auto& getUniforms(const DESCRIPTOR& type) {
        switch (type) {
            case DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT:
                return camDefPersMgr_.pItems;
            case DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT:
                return lgtDefPosMgr_.pItems;
            case DESCRIPTOR::LIGHT_POSITIONAL_PBR:
                return lgtPbrPosMgr_.pItems;
            case DESCRIPTOR::LIGHT_SPOT_DEFAULT:
                return lgtDefSptMgr_.pItems;
            case DESCRIPTOR::FOG_DEFAULT:
                return uniDefFogMgr_.pItems;
            default:
                assert(false);
                throw std::runtime_error("Unhandled uniform type");
        }
    }

    // CAMERA
    const UNIFORM_INDEX MAIN_CAMERA_OFFSET = 0;
    void createCameras();
    Uniform::Manager<Camera::Default::Perspective::Base> camDefPersMgr_;
    // LIGHTS
    void createLights();
    Uniform::Manager<Light::Default::Positional::Base> lgtDefPosMgr_;
    Uniform::Manager<Light::PBR::Positional::Base> lgtPbrPosMgr_;
    Uniform::Manager<Light::Default::Spot::Base> lgtDefSptMgr_;
    // MISCELLANEOUS
    void createMiscellaneous();
    Uniform::Manager<Uniform::Default::Fog::Base> uniDefFogMgr_;

    bool hasVisualHelpers;
};

}  // namespace Uniform

#endif  // !UNIFORM_HANDLER_H