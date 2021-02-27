/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef CDLOD_RENDERER_H
#define CDLOD_RENDERER_H

#include <array>
#include <vulkan/vulkan.hpp>

#include <CDLOD/CDLODQuadTree.h>
#include <CDLOD/CDLODRenderer.h>

#include "BufferItem.h"
#include "DescriptorConstants.h"
#include "Enum.h"
#include "Handlee.h"
#include "MeshConstants.h"
#include "PipelineConstants.h"

// clang-format off
namespace Scene { class Handler; }
namespace Descriptor { class Base; }
// clang-format on

namespace Cdlod {
namespace Renderer {

struct DebugHeightmap : public IHeightmapSource {
    DebugHeightmap() : mapDims() {
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
class Base : public Handlee<Scene::Handler>, public CDLODRenderer {
   public:
    // These settings come from .ini files in CDLOD proper.
    struct Settings {
        // This determines the granularity of the quadtree: Reduce this to reduce
        // the size of the smallest LOD level quad and increase the number of possible
        // LOD levels. Increase this value to reduce memory usage.
        // Restrictions:
        // - it must be power of two (2^n)
        // - it must be bigger or equal to 2
        int LeafQuadTreeNodeSize;
        // RenderGridResolution (resolution of mesh grid used to render terrain quads)
        // will be  LeafQuadTreeNodeSize * RenderGridResolutionMult. Use value of 1
        // for 1:1 grid:heightmap_texel ratio, or use bigger value to enable virtual
        // resolution when using additional noise or DetailHeightmap.
        // Restrictions:
        //  - it must be power of two (2^n)
        //  - LeafQuadTreeNodeSize * RenderGridResolutionMult must be <= 128
        int RenderGridResolutionMult;
        // How many LOD levels to have - big heightmaps will require more LOD levels,
        // and also the smaller the LeafQuadTreeNodeSize is, the more LOD levels will
        // be needed.
        int LODLevelCount = 8;
        // Minimum view range setting that can be set.
        // View range defines range at which terrain will be visible.Setting ViewRange it
        // to too low values will result in incorrect transition between LODs layers
        // (warning message will be displayed on screen in that case) so that 's why there' s
        // this minimum range specified here.
        // If lower ranges than that are required, LODLevelCount must be reduced too.
        float MinViewRange;
        // Maximum view range setting that can be set.
        // Although there's no theoretical maximum, demo does not support too high values
        float MaxViewRange;
        // Determines rendering LOD level distribution based on distance from the viewer.
        // Value of 2.0 should result in equal number of triangles displayed on screen
        // (in average) for all distances.
        // Values above 2.0 will result in more triangles on more distant areas, and vice versa.
        float LODLevelDistanceRatio;

        // DEBUG
        float dbgTexScale;  // Scale of texture (1,1) to world space
    };

    Base(Scene::Handler& handler);

    void init(const IHeightmapSource* pHeightmap, const MapDimensions* pMapDimensions);
    void update(const std::shared_ptr<Pipeline::BindData>& pPipelineBindData, const vk::CommandBuffer& cmd);
    void destroy() override {
        CDLODRenderer::destroy();
        reset();
    }

   private:
    void reset();

    void renderTerrain(const CDLODQuadTree::LODSelection& cdlodSelection,
                       const std::shared_ptr<Pipeline::BindData>& pPipelineBindData, const vk::CommandBuffer& cmd);

    Settings settings_;
    int maxRenderGridResolutionMult_;
    int terrainGridMeshDims_;

    const IHeightmapSource* pHeightmap_;
    int rasterWidth_;
    int rasterHeight_;
    MapDimensions mapDims_;

    CDLODQuadTree cdlodQuadTree_;

    // DEBUG
    void debugResetBoxes();
    void debugUpdateBoxes();
    void debugAddBox(const int lodLevel, const AABB& aabb);
    bool enableDebug_;
    bool useDebugCamera_;
    bool useDebugBoxes_;
    bool useDebugWireframe_;
    bool useDebugTexture_;
    // [0]: white
    // [1]: red
    // [2]: green
    // [3]: blue
    // [4]: texture
    std::array<Descriptor::Base*, 5> pMaterials_;
    Descriptor::Set::bindDataMap wfDescSetBindDataMap_;
    DebugHeightmap dbgHeighmap_;
    Mesh::index whiteBoxOffset_;
    Mesh::index redBoxOffset_;
    Mesh::index greenBoxOffset_;
    Mesh::index blueBoxOffset_;
};

}  // namespace Renderer
}  // namespace Cdlod

#endif  //! CDLOD_RENDERER_H