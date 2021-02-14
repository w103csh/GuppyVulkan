/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_RENDERER_H
#define OCEAN_RENDERER_H

#include <CDLOD/CDLODQuadTree.h>
#include <CDLOD/CDLODRenderer.h>

#include "BufferItem.h"
#include "Handlee.h"
#include "MeshConstants.h"

// clang-format off
namespace Particle { class Handler; }
// clang-format on

namespace Ocean {
namespace Renderer {

class Basic : public Handlee<Particle::Handler>, public CDLODRenderer {
   public:
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
    };

    Basic(Particle::Handler& handler);

    void init(const IHeightmapSource* pHeightmap, const MapDimensions& mapDims);
    void update();

   private:
    void renderTerrain(const CDLODQuadTree::LODSelection& cdlodSelection);

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
    bool useDebugCamera_;
    bool useDebugView_;
    Mesh::index whiteBoxOffset_;
    Mesh::index redBoxOffset_;
    Mesh::index greenBoxOffset_;
    Mesh::index blueBoxOffset_;
};

}  // namespace Renderer
}  // namespace Ocean

#endif  //! OCEAN_RENDERER_H