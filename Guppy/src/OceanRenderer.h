/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_RENDERER_H
#define OCEAN_RENDERER_H

#include <memory>

#include <CDLOD/Common.h>

#include "CdlodRenderer.h"
#include "InstanceManager.h"
#include "Ocean.h"

// clang-format off
namespace Scene { class Handler; }
// clang-format on

namespace Ocean {

class Buffer;

struct Heightmap : public IHeightmapSource {
    Heightmap() : mapDims() {
        mapDims.SizeX = 40960.0f;
        mapDims.SizeY = 20480.0f;
        mapDims.SizeZ = 1200.0f;
        mapDims.MinX = -0.5f * mapDims.SizeX;  //-20480.0
        mapDims.MinY = -0.5f * mapDims.SizeY;  //-10240.0
        mapDims.MinZ = -0.5f * mapDims.SizeZ;  //-600.00
    }

    MapDimensions mapDims;

    int GetSizeX() const override { return 4096; }
    int GetSizeY() const override { return 2048; }
    unsigned short GetHeightAt(int x, int y) const override { return NormalizeForZ(0.0f); }
    void GetAreaMinMaxZ(int x, int y, int sizeX, int sizeY, unsigned short& minZ, unsigned short& maxZ) const override {
        minZ = NormalizeForZ(-50.0f);
        maxZ = NormalizeForZ(50.0f);
    }
    // CDLOD wants to normalize dimension values to the range of an unsigned short.
    unsigned short NormalizeForX(float x) const {
        return static_cast<unsigned short>((x - mapDims.MinX) * 65535.0f / mapDims.SizeX);
    }
    unsigned short NormalizeForY(float y) const {
        return static_cast<unsigned short>((y - mapDims.MinY) * 65535.0f / mapDims.SizeY);
    }
    unsigned short NormalizeForZ(float z) const {
        return static_cast<unsigned short>((z - mapDims.MinZ) * 65535.0f / mapDims.SizeZ);
    }
};

// This class is based off of DemoRender in CDLOD proper.
class Renderer : public Cdlod::Renderer::Base {
   public:
    Renderer(Scene::Handler& handler);

    void tick() override;

    bool shouldDraw(const PIPELINE type) const override;
    void record(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                const vk::CommandBuffer& cmd) override;

    std::shared_ptr<Instance::Cdlod::Ocean::Base>& makeInstance(Instance::Cdlod::Ocean::CreateInfo* pInfo);

    std::unique_ptr<GraphicsWork::OceanSurface> pGraphicsWork;

   private:
    void init() override;
    void reset() override;

    SurfaceCreateInfo surfaceInfo;

    // Cdlod::Renderer::Base
    const Cdlod::Renderer::Settings* getSettings() const { return &settings_; }
    const IHeightmapSource* getHeightmap() const override { return &dbgHeightmap_; }
    const MapDimensions* getMapDimensions() const override { return &dbgHeightmap_.mapDims; }
    void bindDescSetData(const vk::CommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                         const int lodLevel) const override;
    void setGlobalShaderSettings() override;
    PerQuadTreeData& getPerQuadTreeData() override;

    Cdlod::Renderer::Settings settings_;
    Heightmap dbgHeightmap_;
    Uniform::Cdlod::QuadTree::Base* pPerQuadTreeItem_;

    Instance::Manager<Instance::Cdlod::Ocean::Base, Instance::Cdlod::Ocean::Base> instMgr_;
};

}  // namespace Ocean

#endif  //! OCEAN_RENDERER_H