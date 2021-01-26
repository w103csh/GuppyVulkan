/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include <glm/glm.hpp>
#include <future>
#include <unordered_set>

#include <Common/Helpers.h>

#include "Game.h"
#include "Instance.h"
#include "InstanceManager.h"
#include "MaterialHandler.h"  // TODO: including this is sketchy
#include "Mesh.h"
#include "VisualHelper.h"

// clang-format off
namespace Model { class Handler; }
// clang-format on

namespace Mesh {

// TODO: FIND A WAY TO FORCE ALL MESH CREATION FROM THIS HANDLER !!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// The protected Mesh constructors were a crappy start...
class Handler : public Game::Handler {
    friend Mesh::Base;
    friend Model::Handler;  // TODO: calls "make" function directly. Another way?

   private:
    // TODO: All materials & meshes should be made here. End of story.
    template <class TMeshType, class TMeshBaseType, typename TMeshCreateInfo, typename TMaterialCreateInfo,
              typename... TArgs>
    auto &make(std::vector<TMeshBaseType> &meshes, TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
               std::shared_ptr<Instance::Obj3d::Base> &pInstObj3d, TArgs... args) {
        // MATERIAL
        auto &pMaterial = materialHandler().makeMaterial(pMaterialCreateInfo);

        meshes.emplace_back(
            new TMeshType(std::ref(*this), static_cast<index>(meshes.size()), pCreateInfo, pInstObj3d, pMaterial, args...));
        assert(meshes.back()->getOffset() == meshes.size() - 1);

        switch (meshes.back()->getStatus()) {
            case STATUS::PENDING_VERTICES:
                // Do nothing. So far this should only be a model mesh.
                // TODO: add more validation?
                break;
            case STATUS::PENDING_BUFFERS:
                meshes.back()->prepare();
                break;
            default:
                assert(false && "Invalid mesh status after instantiation");
                exit(EXIT_FAILURE);
        }

        return meshes.back();
    }

   public:
    Handler(Game *pGame);

    void init() override;
    void tick() override;

    bool checkOffset(const MESH type, const Mesh::index offset);

    // template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo>
    // std::pair<MESH, Mesh::INDEX> makeMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo) {
    //    if (std::is_base_of<Mesh::Color, TMeshType>::value) {
    //        return std::make_pair(                                                            //
    //            MESH::COLOR,                                                                  //
    //            make<TMeshType>(colorMeshes_, pCreateInfo, pMaterialCreateInfo)->getOffset()  //
    //        );
    //    } else if (std::is_base_of<Mesh::Line, TMeshType>::value) {
    //        return std::make_pair(                                                           //
    //            MESH::LINE,                                                                  //
    //            make<TMeshType>(lineMeshes_, pCreateInfo, pMaterialCreateInfo)->getOffset()  //
    //        );
    //    } else if (std::is_base_of<Mesh::Texture, TMeshType>::value) {
    //        return std::make_pair(                                                          //
    //            MESH::TEXTURE,                                                              //
    //            make<TMeshType>(texMeshes_, pCreateInfo, pMaterialCreateInfo)->getOffset()  //
    //        );
    //    } else {
    //        assert(false && "Unhandled mesh type");
    //    }
    //}

    template <typename TInstanceCreateInfo>
    std::shared_ptr<Instance::Obj3d::Base> &makeInstanceObj3d(TInstanceCreateInfo *pInfo) {
        std::shared_ptr<Instance::Obj3d::Base> &pInstObj3d = pInfo->pSharedData;
        if (pInstObj3d == nullptr) {
            if (pInfo == nullptr || pInfo->data.empty())
                instObj3dMgr_.insert(shell().context().dev, true);
            else
                instObj3dMgr_.insert(shell().context().dev, pInfo->update, pInfo->data);
            pInstObj3d = instObj3dMgr_.pItems.back();
        }
        return pInstObj3d;
    }

    // WARNING !!! THE REFERENCES RETURNED GO BAD !!! (TODO: what should be returned? anything?)
    template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    auto &makeColorMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
                        TInstanceCreateInfo *pInstanceCreateInfo) {
        auto &pInstObj3d = makeInstanceObj3d(pInstanceCreateInfo);
        return make<TMeshType>(colorMeshes_, pCreateInfo, pMaterialCreateInfo, pInstObj3d);
    }
    template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    auto &makeLineMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
                       TInstanceCreateInfo *pInstanceCreateInfo) {
        auto &pInstObj3d = makeInstanceObj3d(pInstanceCreateInfo);
        return make<TMeshType>(lineMeshes_, pCreateInfo, pMaterialCreateInfo, pInstObj3d);
    }
    template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    auto &makeTextureMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
                          TInstanceCreateInfo *pInstanceCreateInfo) {
        auto &pInstObj3d = makeInstanceObj3d(pInstanceCreateInfo);
        return make<TMeshType>(texMeshes_, pCreateInfo, pMaterialCreateInfo, pInstObj3d);
    }

    // TODO: Do these pollute this class? Should this be in a derived Handler?
    template <class TObject3d>
    void makeModelSpaceVisualHelper(TObject3d &obj, float lineSize = 1.0f) {
        // MESH
        AxesCreateInfo meshInfo = {};
        meshInfo.lineSize = lineSize;
        // MATERIAL
        Material::Default::CreateInfo matInfo = {};
        // INSTANCE
        Instance::Obj3d::CreateInfo instInfo = {};
        instInfo.data.push_back({obj.getModel()});
        auto &pInstObj3d = makeInstanceObj3d(&instInfo);

        auto offset = make<VisualHelper::ModelSpace>(lineMeshes_, &meshInfo, &matInfo, pInstObj3d)->getOffset();
        addOffsetToScene(MESH::LINE, offset);
    }
    template <class TMesh>
    void makeTangentSpaceVisualHelper(TMesh pMesh, float lineSize = 0.1f) {
        // MESH
        AxesCreateInfo meshInfo = {};
        meshInfo.lineSize = lineSize;
        // MATERIAL
        Material::Default::CreateInfo matInfo = {};

        auto offset =
            make<VisualHelper::TangentSpace>(lineMeshes_, &meshInfo, &matInfo, pMesh->pInstObj3d_, pMesh)->getOffset();
        addOffsetToScene(MESH::LINE, offset);
    }
    void makeSkyBox() { assert(false); }

    inline Mesh::Base &getMesh(const MESH &type, const size_t &index) {
        switch (type) {
            case MESH::COLOR:
                return std::ref(*colorMeshes_[index].get());
            case MESH::LINE:
                return std::ref(*lineMeshes_[index].get());
            case MESH::TEXTURE:
                return std::ref(*texMeshes_[index].get());
            default:
                assert(false);
                throw std::runtime_error("Unhandled mesh type");
        }
    }
    inline std::unique_ptr<Mesh::Color> &getColorMesh(const size_t &index) { return colorMeshes_[index]; }
    inline std::unique_ptr<Mesh::Line> &getLineMesh(const size_t &index) { return lineMeshes_[index]; }
    inline std::unique_ptr<Mesh::Texture> &getTextureMesh(const size_t &index) { return texMeshes_[index]; }

    // TODO: remove this after Scene no longer iterates through these to draw
    inline const auto &getColorMeshes() const { return colorMeshes_; }
    inline const auto &getLineMeshes() const { return lineMeshes_; }
    inline const auto &getTextureMeshes() const { return texMeshes_; }

    void removeMesh(std::unique_ptr<Mesh::Base> &pMesh);

    inline void updateInstanceData(const Buffer::Info &info) { instObj3dMgr_.updateData(shell().context().dev, info); }

    template <typename TMesh>
    inline void updateMesh(std::unique_ptr<TMesh> &pMesh) {
        instObj3dMgr_.updateData(shell().context().dev, pMesh->pInstObj3d_->BUFFER_INFO);
    }

   private:
    void reset() override;

    // TODO: Not sure this should be here.
    void addOffsetToScene(const MESH type, const Mesh::index offset);

    // MESH
    // COLOR
    std::vector<std::unique_ptr<Mesh::Color>> colorMeshes_;
    std::vector<std::unique_ptr<Mesh::Line>> lineMeshes_;
    // TEXTURE
    std::vector<std::unique_ptr<Mesh::Texture>> texMeshes_;

    // INSTANCE
    Instance::Manager<Instance::Obj3d::Base, Instance::Obj3d::Base> instObj3dMgr_;

    // LOADING
    std::vector<std::future<Mesh::Base *>> ldgFutures_;
    std::unordered_set<std::pair<MESH, size_t>, hash_pair_enum_size_t<MESH>> ldgOffsets_;
};

}  // namespace Mesh

#endif  // !MESH_HANDLER_H
