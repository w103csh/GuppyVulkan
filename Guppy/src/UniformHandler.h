
#ifndef UNIFORM_HANDLER_H
#define UNIFORM_HANDLER_H

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <vulkan/vulkan.h>

#include "Camera.h"
#include "ConstantsAll.h"
#include "Deferred.h"
#include "Descriptor.h"
#include "DescriptorManager.h"
#include "Game.h"
#include "Geometry.h"
#include "Helpers.h"
#include "Light.h"
#include "PBR.h"
#include "Random.h"
#include "ScreenSpace.h"
#include "Shadow.h"
#include "Storage.h"
#include "Tessellation.h"
#include "Uniform.h"
#include "UniformOffsetsManager.h"

namespace Uniform {

// MANAGER

template <class TBase>
using ItemPointer = std::unique_ptr<TBase>;

template <class TDerived>
using ManagerType = Descriptor::Manager<Descriptor::Base, TDerived, ItemPointer>;

// TODO: inner class of Handler?
template <class TDerived>
class Manager : public ManagerType<TDerived> {
   public:
    Manager(const std::string&& name, const DESCRIPTOR&& descriptorType, const index&& maxSize,
            const std::string&& macroName)
        : ManagerType<TDerived>{
              std::forward<const std::string>(name),
              std::forward<const DESCRIPTOR>(descriptorType),
              std::forward<const index>(maxSize),
              true,
              std::forward<const std::string>(macroName),
              VK_SHARING_MODE_EXCLUSIVE,
              0,
          } {}
};

// HANDLER

class Handler : public Game::Handler {
   private:
    std::vector<std::unique_ptr<Descriptor::Base>>& getItems(const DESCRIPTOR& type);

   public:
    Handler(Game* pGame);

    void init() override;

    // DESCRIPTOR
    uint32_t getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets);
    bool validateUniformOffsets(const std::pair<DESCRIPTOR, index>& keyValue);
    void getWriteInfos(const DESCRIPTOR& descType, const Uniform::offsets& offsets,
                       Descriptor::Set::ResourceInfo& setResInfo);

    void createVisualHelpers();
    void attachSwapchain();
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
    inline void update(T& uniform, const int index = -1) {
        getManager<T>().updateData(shell().context().dev, uniform.BUFFER_INFO, index);
    }

    // DESCRIPTOR
    inline const auto& getOffsetsMgr() const { return offsetsManager_; }

    // RANDOM
    Random rand;  // TODO: where should this go?

   private:
    void reset() override;

    uint32_t getDescriptorCount(const std::vector<ItemPointer<Descriptor::Base>>& pItems,
                                const Uniform::offsets& offsets) const;
    std::set<uint32_t> getBindingOffsets(const std::vector<ItemPointer<Descriptor::Base>>& pItems,
                                         const Uniform::offsets& offsets) const;

    // clang-format off
    inline Manager<Camera::Default::Perspective::Base>& camDefPersMgr() { return std::get<Uniform::Manager<Camera::Default::Perspective::Base>>(managers_[0]);};
    inline Manager<Light::Default::Positional::Base>& lgtDefPosMgr() { return std::get<Uniform::Manager<Light::Default::Positional::Base>>(managers_[1]);};
    inline Manager<Light::PBR::Positional::Base>& lgtPbrPosMgr() { return std::get<Uniform::Manager<Light::PBR::Positional::Base>>(managers_[2]);};
    inline Manager<Light::Default::Spot::Base>& lgtDefSptMgr() { return std::get<Uniform::Manager<Light::Default::Spot::Base>>(managers_[3]);};
    inline Manager<Light::Shadow::Positional>& lgtShdwPosMgr() { return std::get<Uniform::Manager<Light::Shadow::Positional>>(managers_[4]);};
    inline Manager<Default::Fog::Base>& uniDefFogMgr() { return std::get<Uniform::Manager<Default::Fog::Base>>(managers_[5]);};
    inline Manager<Default::Projector::Base>& uniDefPrjMgr() { return std::get<Uniform::Manager<Default::Projector::Base>>(managers_[6]);};
    inline Manager<ScreenSpace::Default>& uniScrDefMgr() { return std::get<Uniform::Manager<ScreenSpace::Default>>(managers_[7]);};
    inline Manager<Deferred::SSAO>& uniDfrSSAOMgr() { return std::get<Uniform::Manager<Deferred::SSAO>>(managers_[8]);};
    inline Manager<Shadow::Base>& uniShdwDataMgr() { return std::get<Uniform::Manager<Shadow::Base>>(managers_[9]);};
   public: // Why aren't these public? If the memory is host visible then they should be public right?
    inline Manager<Tessellation::Default::Base>& uniTessDefMgr() { return std::get<Uniform::Manager<Tessellation::Default::Base>>(managers_[10]);};
   private:
    inline Manager<Geometry::Default::Base>& uniGeomDefMgr() { return std::get<Uniform::Manager<Geometry::Default::Base>>(managers_[11]);};
    inline Manager<Storage::PostProcess::Base>& strPstPrcMgr() { return std::get<Uniform::Manager<Storage::PostProcess::Base>>(managers_[12]);};

    template <class T> inline Manager<T>& getManager() { assert(false); }
    template <> inline Manager<Camera::Default::Perspective::Base>& getManager() { return camDefPersMgr(); }
    template <> inline Manager<Light::Default::Positional::Base>& getManager() { return lgtDefPosMgr(); }
    template <> inline Manager<Light::PBR::Positional::Base>& getManager() { return lgtPbrPosMgr(); }
    template <> inline Manager<Light::Default::Spot::Base>& getManager() { return lgtDefSptMgr(); }
    template <> inline Manager<Light::Shadow::Positional>& getManager() { return lgtShdwPosMgr(); }
    template <> inline Manager<Default::Fog::Base>& getManager() { return uniDefFogMgr(); }
    template <> inline Manager<Default::Projector::Base>& getManager() { return uniDefPrjMgr(); }
    template <> inline Manager<ScreenSpace::Default>& getManager() { return uniScrDefMgr(); }
    template <> inline Manager<Deferred::SSAO>& getManager() { return uniDfrSSAOMgr(); }
    template <> inline Manager<Shadow::Base>& getManager() { return uniShdwDataMgr(); }
    template <> inline Manager<Tessellation::Default::Base>& getManager() { return uniTessDefMgr(); }
    template <> inline Manager<Geometry::Default::Base>& getManager() { return uniGeomDefMgr(); }
    template <> inline Manager<Storage::PostProcess::Base>& getManager() { return strPstPrcMgr(); }
    // clang-format on

    void createCameras();
    void createLights();
    void createMiscellaneous();
    void createTessellationData();

    // BUFFER MANAGERS
    using Manager = std::variant<                     //
        Manager<Camera::Default::Perspective::Base>,  //
        Manager<Light::Default::Positional::Base>,    //
        Manager<Light::PBR::Positional::Base>,        //
        Manager<Light::Default::Spot::Base>,          //
        Manager<Light::Shadow::Positional>,           //
        Manager<Default::Fog::Base>,                  //
        Manager<Default::Projector::Base>,            //
        Manager<ScreenSpace::Default>,                //
        Manager<Deferred::SSAO>,                      //
        Manager<Shadow::Base>,                        //
        Manager<Tessellation::Default::Base>,         //
        Manager<Geometry::Default::Base>,           //
        Manager<Storage::PostProcess::Base>           //
        >;
    std::array<Manager, 13> managers_;

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