#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include <glm/glm.hpp>
#include <future>
#include <unordered_set>

#include "Game.h"
#include "Helpers.h"
#include "Instance.h"
#include "InstanceManager.h"
#include "MaterialHandler.h"  // TODO: including this is sketchy
#include "Mesh.h"
#include "VisualHelper.h"

namespace Mesh {

// TODO: FIND A WAY TO FORCE ALL MESH CREATION FROM THIS HANDLER !!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// The protected Mesh constructors were a crappy start...
class Handler : public Game::Handler {
    friend Mesh::Base;

   private:
    // TODO: All materials & meshes should be made here. End of story.
    template <class TMeshType, class TMeshBaseType, typename TMeshCreateInfo, typename TMaterialCreateInfo,
              typename TInstanceCreateInfo, typename... TArgs>
    auto &make(std::vector<TMeshBaseType> &meshes, TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
               TInstanceCreateInfo *pInstanceCreateInfo, TArgs... args) {
        // MATERIAL
        auto &pMaterial = materialHandler().makeMaterial(pMaterialCreateInfo);
        // INSTANCE
        std::shared_ptr<Instance::Base> &pInstanceData = pInstanceCreateInfo->pSharedData;
        if (pInstanceData == nullptr) {
            if (pInstanceCreateInfo == nullptr || pInstanceCreateInfo->data.empty())
                instDefMgr_.insert(shell().context().dev, true);
            else
                instDefMgr_.insert(shell().context().dev, pInstanceCreateInfo->update, pInstanceCreateInfo->data);
            pInstanceData = instDefMgr_.pItems.back();
        }

        // INSTANTIATE
        meshes.emplace_back(new TMeshType(std::ref(*this), pCreateInfo, pInstanceData, pMaterial, args...));

        // SET VALUES
        meshes.back()->offset_ = meshes.size() - 1;

        switch (meshes.back()->getStatus()) {
            case STATUS::PENDING_VERTICES:
                // Do nothing. So far this should only be a model mesh.
                // TODO: add more validation?
                break;
            case STATUS::PENDING_BUFFERS:
                meshes.back()->prepare();
                break;
            default:
                throw std::runtime_error("Invalid mesh status after instantiation");
        }

        return meshes.back();
    }

   public:
    Handler(Game *pGame);

    void init() override;

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

    // WARNING !!! THE REFERENCES RETURNED GO BAD !!! (TODO: what should be returned? anything?)
    template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    auto &makeColorMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
                        TInstanceCreateInfo *pInstanceCreateInfo) {
        return make<TMeshType>(colorMeshes_, pCreateInfo, pMaterialCreateInfo, pInstanceCreateInfo);
    }
    template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    auto &makeLineMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
                       TInstanceCreateInfo *pInstanceCreateInfo) {
        return make<TMeshType>(lineMeshes_, pCreateInfo, pMaterialCreateInfo, pInstanceCreateInfo);
    }
    template <class TMeshType, typename TMeshCreateInfo, typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    auto &makeTextureMesh(TMeshCreateInfo *pCreateInfo, TMaterialCreateInfo *pMaterialCreateInfo,
                          TInstanceCreateInfo *pInstanceCreateInfo) {
        return make<TMeshType>(texMeshes_, pCreateInfo, pMaterialCreateInfo, pInstanceCreateInfo);
    }

    // TODO: Do these pollute this class? Should this be in a derived Handler?
    template <class TObject3d>
    void makeModelSpaceVisualHelper(TObject3d &obj, float lineSize = 1.0f) {
        AxesCreateInfo meshInfo = {};
        meshInfo.lineSize = lineSize;
        Material::Default::CreateInfo matInfo = {};
        Instance::Default::CreateInfo instInfo = {};
        instInfo.data.push_back({obj.model()});
        make<VisualHelper::ModelSpace>(lineMeshes_, &meshInfo, &matInfo, &instInfo);
    }
    template <class TMesh>
    void makeTangentSpaceVisualHelper(TMesh pMesh, float lineSize = 0.1f) {
        AxesCreateInfo meshInfo = {};
        meshInfo.lineSize = lineSize;
        Material::Default::CreateInfo matInfo = {};
        Instance::Default::CreateInfo instInfo = {};
        instInfo.pSharedData = pMesh->pInstanceData_;
        make<VisualHelper::TangentSpace>(lineMeshes_, &meshInfo, &matInfo, &instInfo, pMesh);
    }

    inline std::unique_ptr<Mesh::Color> &getColorMesh(const size_t &index) { return colorMeshes_[index]; }
    inline std::unique_ptr<Mesh::Line> &getLineMesh(const size_t &index) { return lineMeshes_[index]; }
    inline std::unique_ptr<Mesh::Texture> &getTextureMesh(const size_t &index) { return texMeshes_[index]; }

    // TODO: remove this after Scene no longer iterates through these to draw
    inline const auto &getColorMeshes() const { return colorMeshes_; }
    inline const auto &getLineMeshes() const { return lineMeshes_; }
    inline const auto &getTextureMeshes() const { return texMeshes_; }

    void removeMesh(std::unique_ptr<Mesh::Base> &pMesh);

    void update();
    inline void updateInstanceData(const Buffer::Info &info) { instDefMgr_.updateData(shell().context().dev, info); }

    template <typename TMesh>
    inline void updateMesh(std::unique_ptr<TMesh> &pMesh) {
        instDefMgr_.updateData(shell().context().dev, pMesh->pInstanceData_->BUFFER_INFO);
    }

   private:
    void reset() override;

    // MESH
    // COLOR
    std::vector<std::unique_ptr<Mesh::Color>> colorMeshes_;
    std::vector<std::unique_ptr<Mesh::Line>> lineMeshes_;
    // TEXTURE
    std::vector<std::unique_ptr<Mesh::Texture>> texMeshes_;

    // INSTANCE
    Instance::Manager<Instance::Default::Base> instDefMgr_;

    // LOADING
    std::vector<std::future<Mesh::Base *>> ldgFutures_;
    std::unordered_set<std::pair<MESH, size_t>, hash_pair_enum_size_t<MESH>> ldgOffsets_;
};

}  // namespace Mesh

#endif  // !MESH_HANDLER_H
