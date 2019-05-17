
#ifndef UNIFORM_HANDLER_H
#define UNIFORM_HANDLER_H

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
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

template <class TBase>
using ItemPointer = std::unique_ptr<TBase>;

// TODO: inner class of Handler?
template <class TDerived>
class Manager : public Buffer::Manager::Descriptor<Uniform::Base, TDerived, ItemPointer> {
   public:
    Manager(const std::string&& name, const DESCRIPTOR&& descriptorType, const UNIFORM_INDEX&& maxSize,
            const std::string&& macroName)
        : Buffer::Manager::Descriptor<Uniform::Base, TDerived, ItemPointer>{
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
};

// **********************
//      Handler
// **********************

class Handler : public Game::Handler {
   private:
    inline auto& getUniforms(const DESCRIPTOR& type) {
        switch (type) {
            case DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT:
                return camDefPersMgr().pItems;
            case DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT:
                return lgtDefPosMgr().pItems;
            case DESCRIPTOR::LIGHT_POSITIONAL_PBR:
                return lgtPbrPosMgr().pItems;
            case DESCRIPTOR::LIGHT_SPOT_DEFAULT:
                return lgtDefSptMgr().pItems;
            case DESCRIPTOR::FOG_DEFAULT:
                return uniDefFogMgr().pItems;
            default:
                assert(false);
                throw std::runtime_error("Unhandled uniform type");
        }
    }

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
    inline Camera::Default::Perspective::Base& getMainCamera() {
        return std::ref(*(Camera::Default::Perspective::Base*)(camDefPersMgr().pItems[MAIN_CAMERA_OFFSET].get()));
    }

    // FIRST LIGHT
    inline Light::Default::Positional::Base& getDefPosLight(uint32_t index = 0) {
        assert(index < lgtDefPosMgr().pItems.size());
        return std::ref(*(Light::Default::Positional::Base*)(lgtDefPosMgr().pItems[index].get()));
    }

    // SHADER
    void shaderTextReplace(std::string& text);

    template <typename T>
    inline T& getUniform(const DESCRIPTOR& type, const UNIFORM_INDEX& index) {
        auto& uniforms = getUniforms(type);
        assert(index < uniforms.size());
        return std::ref(*(T*)(uniforms[index].get()));
    }

    template <class T>
    inline void update(T& uniform) {
        getManager<T>().updateData(shell().context().dev, uniform.BUFFER_INFO);
    }

   private:
    void reset() override;

    std::set<uint32_t> getBindingOffsets(const Descriptor::bindingMapValue& value);

    // clang-format off
    inline Uniform::Manager<Camera::Default::Perspective::Base>& camDefPersMgr() { return std::get<Uniform::Manager<Camera::Default::Perspective::Base>>(managers_[0]);};
    inline Uniform::Manager<Light::Default::Positional::Base>& lgtDefPosMgr() { return std::get<Uniform::Manager<Light::Default::Positional::Base>>(managers_[1]);};
    inline Uniform::Manager<Light::PBR::Positional::Base>& lgtPbrPosMgr() { return std::get<Uniform::Manager<Light::PBR::Positional::Base>>(managers_[2]);};
    inline Uniform::Manager<Light::Default::Spot::Base>& lgtDefSptMgr() { return std::get<Uniform::Manager<Light::Default::Spot::Base>>(managers_[3]);};
    inline Uniform::Manager<Uniform::Default::Fog::Base>& uniDefFogMgr() { return std::get<Uniform::Manager<Uniform::Default::Fog::Base>>(managers_[4]);};

    template <class T> inline Uniform::Manager<T>& getManager() { /*static_assert(false, "Not implemented");*/ }
    template <> inline Uniform::Manager<Camera::Default::Perspective::Base>& getManager() { return camDefPersMgr(); }
    template <> inline Uniform::Manager<Light::Default::Positional::Base>& getManager() { return lgtDefPosMgr(); }
    template <> inline Uniform::Manager<Light::PBR::Positional::Base>& getManager() { return lgtPbrPosMgr(); }
    template <> inline Uniform::Manager<Light::Default::Spot::Base>& getManager() { return lgtDefSptMgr(); }
    template <> inline Uniform::Manager<Uniform::Default::Fog::Base>& getManager() { return uniDefFogMgr(); }
    // clang-format on

    void createCameras();
    void createLights();
    void createMiscellaneous();

    using Manager = std::variant<                              //
        Uniform::Manager<Camera::Default::Perspective::Base>,  //
        Uniform::Manager<Light::Default::Positional::Base>,    //
        Uniform::Manager<Light::PBR::Positional::Base>,        //
        Uniform::Manager<Light::Default::Spot::Base>,          //
        Uniform::Manager<Uniform::Default::Fog::Base>          //
        >;

    std::array<Manager, 5> managers_;

    struct MacroName {
        template <typename TManager>
        std::string operator()(const TManager& manager) const {
            return manager.MACRO_NAME;
        }
    };

    struct ItemCount {
        template <typename TManager>
        auto operator()(const TManager& manager) const {
            return manager.pItems.size();
        }
    };

    const UNIFORM_INDEX MAIN_CAMERA_OFFSET = 0;
    bool hasVisualHelpers;
};

}  // namespace Uniform

#endif  // !UNIFORM_HANDLER_H