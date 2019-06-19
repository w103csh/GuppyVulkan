
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

#include "Camera.h"
#include "Constants.h"
#include "DescriptorManager.h"
#include "Helpers.h"
#include "Light.h"
#include "PBR.h"
#include "Uniform.h"
#include "UniformOffsetsManager.h"

namespace Uniform {

// MANAGER

template <class TBase>
using ItemPointer = std::unique_ptr<TBase>;

template <class TDerived>
using ManagerType = Descriptor::Manager<Uniform::Base, TDerived, ItemPointer>;

// TODO: inner class of Handler?
template <class TDerived>
class Manager : public ManagerType<TDerived> {
   public:
    Manager(const std::string&& name, const DESCRIPTOR&& descriptorType, const index&& maxSize,
            const std::string&& macroName)
        : ManagerType<TDerived>{std::forward<const std::string>(name),
                                std::forward<const DESCRIPTOR>(descriptorType),
                                std::forward<const index>(maxSize),
                                true,
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                                std::forward<const std::string>(macroName),
                                VK_SHARING_MODE_EXCLUSIVE,
                                0} {}
};

// HANDLER

class Handler : public Game::Handler {
   private:
    inline auto& getItems(const DESCRIPTOR& type) {
        switch (std::visit(Descriptor::GetUniform{}, type)) {
            case UNIFORM::CAMERA_PERSPECTIVE_DEFAULT:
                return camDefPersMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_DEFAULT:
                return lgtDefPosMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_PBR:
                return lgtPbrPosMgr().pItems;
            case UNIFORM::LIGHT_SPOT_DEFAULT:
                return lgtDefSptMgr().pItems;
            case UNIFORM::FOG_DEFAULT:
                return uniDefFogMgr().pItems;
            case UNIFORM::PROJECTOR_DEFAULT:
                return uniDefPrjMgr().pItems;
            default:
                assert(false);
                throw std::runtime_error("Unhandled uniform type");
        }
    }

   public:
    Handler(Game* pGame);

    void init() override;

    // DESCRIPTOR
    uint32_t getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets);
    bool validatePipelineLayout(const Descriptor::bindingMap& map);
    bool validateUniformOffsets(const std::pair<DESCRIPTOR, index>& keyValue);
    std::vector<VkDescriptorBufferInfo> getWriteInfos(const DESCRIPTOR& descType, const Uniform::offsets& offsets);

    void createVisualHelpers();
    void update();

    // MAIN CAMERA
    inline Camera::Default::Perspective::Base& getMainCamera() {
        return std::ref(*(Camera::Default::Perspective::Base*)(camDefPersMgr().pItems[mainCameraOffset_].get()));
    }

    // FIRST LIGHT
    inline Light::Default::Positional::Base& getDefPosLight(uint32_t index = 0) {
        assert(index < lgtDefPosMgr().pItems.size());
        return std::ref(*(Light::Default::Positional::Base*)(lgtDefPosMgr().pItems[index].get()));
    }

    // SHADER
    void shaderTextReplace(const Descriptor::Set::textReplaceTuples& replaceTuples, std::string& text) const;

    template <typename T>
    inline T& getUniform(const DESCRIPTOR& type, const index& index) {
        auto& pItems = getItems(type);
        assert(index < pItems.size());
        return std::ref(*(T*)(pItems[index].get()));
    }

    template <class T>
    inline void update(T& uniform) {
        getManager<T>().updateData(shell().context().dev, uniform.BUFFER_INFO);
    }

    // DESCRIPTOR
    inline const auto& getOffsetsMgr() const { return offsetsManager_; }

   private:
    void reset() override;

    uint32_t getDescriptorCount(const std::vector<ItemPointer<Uniform::Base>>& pItems,
                                const Uniform::offsets& offsets) const;
    std::set<uint32_t> getBindingOffsets(const std::vector<ItemPointer<Uniform::Base>>& pItems,
                                         const Uniform::offsets& offsets) const;

    // clang-format off
    inline Uniform::Manager<Camera::Default::Perspective::Base>& camDefPersMgr() { return std::get<Uniform::Manager<Camera::Default::Perspective::Base>>(managers_[0]);};
    inline Uniform::Manager<Light::Default::Positional::Base>& lgtDefPosMgr() { return std::get<Uniform::Manager<Light::Default::Positional::Base>>(managers_[1]);};
    inline Uniform::Manager<Light::PBR::Positional::Base>& lgtPbrPosMgr() { return std::get<Uniform::Manager<Light::PBR::Positional::Base>>(managers_[2]);};
    inline Uniform::Manager<Light::Default::Spot::Base>& lgtDefSptMgr() { return std::get<Uniform::Manager<Light::Default::Spot::Base>>(managers_[3]);};
    inline Uniform::Manager<Uniform::Default::Fog::Base>& uniDefFogMgr() { return std::get<Uniform::Manager<Uniform::Default::Fog::Base>>(managers_[4]);};
    inline Uniform::Manager<Uniform::Default::Projector::Base>& uniDefPrjMgr() { return std::get<Uniform::Manager<Uniform::Default::Projector::Base>>(managers_[5]);};

    template <class T> inline Uniform::Manager<T>& getManager() { assert(false); }
    template <> inline Uniform::Manager<Camera::Default::Perspective::Base>& getManager() { return camDefPersMgr(); }
    template <> inline Uniform::Manager<Light::Default::Positional::Base>& getManager() { return lgtDefPosMgr(); }
    template <> inline Uniform::Manager<Light::PBR::Positional::Base>& getManager() { return lgtPbrPosMgr(); }
    template <> inline Uniform::Manager<Light::Default::Spot::Base>& getManager() { return lgtDefSptMgr(); }
    template <> inline Uniform::Manager<Uniform::Default::Fog::Base>& getManager() { return uniDefFogMgr(); }
    template <> inline Uniform::Manager<Uniform::Default::Projector::Base>& getManager() { return uniDefPrjMgr(); }
    // clang-format on

    void createCameras();
    void createLights();
    void createMiscellaneous();

    // BUFFER MANAGERS
    using Manager = std::variant<                              //
        Uniform::Manager<Camera::Default::Perspective::Base>,  //
        Uniform::Manager<Light::Default::Positional::Base>,    //
        Uniform::Manager<Light::PBR::Positional::Base>,        //
        Uniform::Manager<Light::Default::Spot::Base>,          //
        Uniform::Manager<Uniform::Default::Fog::Base>,         //
        Uniform::Manager<Uniform::Default::Projector::Base>    //
        >;
    std::array<Manager, 6> managers_;

    // DESCRIPTOR
    OffsetsManager offsetsManager_;

    struct GetMacroName {
        template <typename TManager>
        std::string operator()(const TManager& manager) const {
            return manager.MACRO_NAME;
        }
    };

    struct GetItems {
        template <typename TManager>
        const auto& operator()(const TManager& manager) const {
            return manager.pItems;
        }
    };

    struct GetType {
        template <typename TManager>
        const auto& operator()(const TManager& manager) const {
            return manager.DESCRIPTOR_TYPE;
        }
    };

    index mainCameraOffset_ = 0;
    bool hasVisualHelpers;
};

}  // namespace Uniform

#endif  // !UNIFORM_HANDLER_H