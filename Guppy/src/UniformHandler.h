/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef UNIFORM_HANDLER_H
#define UNIFORM_HANDLER_H

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <Common/Helpers.h>

#include "Camera.h"
#include "Cdlod.h"
#include "ConstantsAll.h"
#include "Deferred.h"
#include "Descriptor.h"
#include "DescriptorManager.h"
#include "Game.h"
#include "Geometry.h"
#include "Light.h"
#include "Ocean.h"
#include "OceanComputeWork.h"
#include "Particle.h"
#include "PBR.h"
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
              vk::SharingMode::eExclusive,
              {},
          } {}
};

// HANDLER

class Handler : public Game::Handler {
    // clang-format off
   public:
    virtual_inline auto& camPersDefMgr() { return std::get<Uniform::Manager<Camera::Perspective::Default::Base>>(managers_[0]);};
    virtual_inline auto& camPersCubeMgr() { return std::get<Uniform::Manager<Camera::Perspective::CubeMap::Base>>(managers_[1]);};
    virtual_inline auto& lgtDefDirMgr() { return std::get<Uniform::Manager<Light::Default::Directional::Base>>(managers_[2]);};
    virtual_inline auto& lgtDefPosMgr() { return std::get<Uniform::Manager<Light::Default::Positional::Base>>(managers_[3]);};
    virtual_inline auto& lgtPbrPosMgr() { return std::get<Uniform::Manager<Light::PBR::Positional::Base>>(managers_[4]);};
    virtual_inline auto& lgtDefSptMgr() { return std::get<Uniform::Manager<Light::Default::Spot::Base>>(managers_[5]);};
    virtual_inline auto& lgtShdwPosMgr() { return std::get<Uniform::Manager<Light::Shadow::Positional::Base>>(managers_[6]);};
    virtual_inline auto& lgtShdwCubeMgr() { return std::get<Uniform::Manager<Light::Shadow::Cube::Base>>(managers_[7]);};
    virtual_inline auto& uniDefFogMgr() { return std::get<Uniform::Manager<Default::Fog::Base>>(managers_[8]);};
    virtual_inline auto& uniDefPrjMgr() { return std::get<Uniform::Manager<Default::Projector::Base>>(managers_[9]);};
    virtual_inline auto& uniScrDefMgr() { return std::get<Uniform::Manager<ScreenSpace::Default>>(managers_[10]);};
    virtual_inline auto& uniDfrSSAOMgr() { return std::get<Uniform::Manager<Deferred::SSAO>>(managers_[11]);};
    virtual_inline auto& uniShdwDataMgr() { return std::get<Uniform::Manager<Shadow::Base>>(managers_[12]);};
    virtual_inline auto& uniTessDefMgr() { return std::get<Uniform::Manager<Tessellation::Default::Base>>(managers_[13]);};
    virtual_inline auto& uniGeomDefMgr() { return std::get<Uniform::Manager<Geometry::Default::Base>>(managers_[14]);};
    virtual_inline auto& uniWaveMgr() { return std::get<Uniform::Manager<Particle::Wave::Base>>(managers_[15]);};
    virtual_inline auto& strPstPrcMgr() { return std::get<Uniform::Manager<Storage::PostProcess::Base>>(managers_[16]);};
    virtual_inline auto& cdlodQdTrMgr() { return std::get<Uniform::Manager<Cdlod::QuadTree::Base>>(managers_[17]);};
    // DYNAMIC
    virtual_inline auto& tessPhongMgr() { return std::get<UniformDynamic::Tessellation::Phong::Manager>(managersDynamic_[0]);};
    virtual_inline auto& ocnSimDpchMgr() { return std::get<UniformDynamic::Ocean::SimulationDispatch::Manager>(managersDynamic_[1]);};
    virtual_inline auto& ocnSimDrawMgr() { return std::get<UniformDynamic::Ocean::SimulationDraw::Manager>(managersDynamic_[2]);};

   private:
    template <class T> virtual_inline Manager<T>& getManager() { assert(false); }
    template <> virtual_inline Manager<Camera::Perspective::Default::Base>& getManager() { return camPersDefMgr(); }
    template <> virtual_inline Manager<Camera::Perspective::CubeMap::Base>& getManager() { return camPersCubeMgr(); }
    template <> virtual_inline Manager<Light::Default::Directional::Base>& getManager() { return lgtDefDirMgr(); }
    template <> virtual_inline Manager<Light::Default::Positional::Base>& getManager() { return lgtDefPosMgr(); }
    template <> virtual_inline Manager<Light::PBR::Positional::Base>& getManager() { return lgtPbrPosMgr(); }
    template <> virtual_inline Manager<Light::Default::Spot::Base>& getManager() { return lgtDefSptMgr(); }
    template <> virtual_inline Manager<Light::Shadow::Positional::Base>& getManager() { return lgtShdwPosMgr(); }
    template <> virtual_inline Manager<Light::Shadow::Cube::Base>& getManager() { return lgtShdwCubeMgr(); }
    template <> virtual_inline Manager<Default::Fog::Base>& getManager() { return uniDefFogMgr(); }
    template <> virtual_inline Manager<Default::Projector::Base>& getManager() { return uniDefPrjMgr(); }
    template <> virtual_inline Manager<ScreenSpace::Default>& getManager() { return uniScrDefMgr(); }
    template <> virtual_inline Manager<Deferred::SSAO>& getManager() { return uniDfrSSAOMgr(); }
    template <> virtual_inline Manager<Shadow::Base>& getManager() { return uniShdwDataMgr(); }
    template <> virtual_inline Manager<Tessellation::Default::Base>& getManager() { return uniTessDefMgr(); }
    template <> virtual_inline Manager<Geometry::Default::Base>& getManager() { return uniGeomDefMgr(); }
    template <> virtual_inline Manager<Particle::Wave::Base>& getManager() { return uniWaveMgr(); }
    template <> virtual_inline Manager<Storage::PostProcess::Base>& getManager() { return strPstPrcMgr(); }
    template <> virtual_inline Manager<Cdlod::QuadTree::Base>& getManager() { return cdlodQdTrMgr(); }
    // clang-format on

    // BUFFER MANAGERS
    using Manager = std::variant<                     //
        Manager<Camera::Perspective::Default::Base>,  //
        Manager<Camera::Perspective::CubeMap::Base>,  //
        Manager<Light::Default::Directional::Base>,   //
        Manager<Light::Default::Positional::Base>,    //
        Manager<Light::PBR::Positional::Base>,        //
        Manager<Light::Default::Spot::Base>,          //
        Manager<Light::Shadow::Positional::Base>,     //
        Manager<Light::Shadow::Cube::Base>,           //
        Manager<Default::Fog::Base>,                  //
        Manager<Default::Projector::Base>,            //
        Manager<ScreenSpace::Default>,                //
        Manager<Deferred::SSAO>,                      //
        Manager<Shadow::Base>,                        //
        Manager<Tessellation::Default::Base>,         //
        Manager<Geometry::Default::Base>,             //
        Manager<Particle::Wave::Base>,                //
        Manager<Storage::PostProcess::Base>,          //
        Manager<Cdlod::QuadTree::Base>                //
        >;
    std::array<Manager, 18> managers_;
    // DYNAMIC
    using ManagerDynamic = std::variant<                     //
        UniformDynamic::Tessellation::Phong::Manager,        //
        UniformDynamic::Ocean::SimulationDispatch::Manager,  //
        UniformDynamic::Ocean::SimulationDraw::Manager       //
        >;
    std::array<ManagerDynamic, 3> managersDynamic_;

    std::vector<std::unique_ptr<Descriptor::Base>>& getItems(const DESCRIPTOR& type);

   public:
    Handler(Game* pGame);

    void init() override;
    void frame() override;

    // DESCRIPTOR
    uint32_t getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets);
    bool validateUniformOffsets(const std::pair<DESCRIPTOR, index>& keyValue);
    void getWriteInfos(const DESCRIPTOR& descType, const Uniform::offsets& offsets,
                       Descriptor::Set::ResourceInfo& setResInfo);

    void createVisualHelpers();
    void attachSwapchain();

    // CAMERAS
    void cycleCamera();
    void moveToDebugCamera();
    // ACTIVE
    inline auto& getActiveCamera() {
        return *static_cast<Camera::Perspective::Default::Base*>(camPersDefMgr().pItems[activeCameraOffset_].get());
    }
    // MAIN
    inline auto& getMainCamera() {
        return *static_cast<Camera::Perspective::Default::Base*>(camPersDefMgr().pItems[mainCameraOffset_].get());
    }
    // DEBUG
    virtual_inline bool hasDebugCamera() const { return debugCameraOffset_ != BAD_OFFSET; }
    inline auto& getDebugCamera() {
        assert(hasDebugCamera());
        return *static_cast<Camera::Perspective::Default::Base*>(camPersDefMgr().pItems[debugCameraOffset_].get());
    }

    // FIRST LIGHT
    inline auto& getDefPosLight(const uint32_t index = 0) {
        assert(index < lgtDefPosMgr().pItems.size());
        return *static_cast<Light::Default::Positional::Base*>(lgtDefPosMgr().pItems[index].get());
    }

    // SHADER
    void textReplaceShader(const Descriptor::Set::textReplaceTuples& replaceTuples, const std::string_view& fileName,
                           std::string& text) const;

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
    template <class T>
    inline void update(T* pUniform, const int index = -1) {
        getManager<T>().updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
    }

    // DESCRIPTOR
    inline const auto& getOffsetsMgr() const { return offsetsManager_; }

   private:
    void reset() override;

    uint32_t getDescriptorCount(const std::vector<ItemPointer<Descriptor::Base>>& pItems,
                                const Uniform::offsets& offsets) const;
    std::set<uint32_t> getBindingOffsets(const std::vector<ItemPointer<Descriptor::Base>>& pItems,
                                         const Uniform::offsets& offsets) const;

    void createCameras();
    void createLights();
    void createMiscellaneous();
    void createTessellationData();

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

    index activeCameraOffset_ = 0;
    index mainCameraOffset_ = 0;
    index debugCameraOffset_ = 0;

    bool hasVisualHelpers_;
};

}  // namespace Uniform

#endif  // !UNIFORM_HANDLER_H