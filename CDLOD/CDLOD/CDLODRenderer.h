//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2021 Colin Hughes<colin.s.hughes @gmail.com>
// -------------------------------
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#ifndef _CDLOD_RENDERER_H_
#define _CDLOD_RENDERER_H_

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Common.h"
#include "VkGridMesh.h"
#include "CDLODQuadTree.h"

#include <Common/Context.h>

// class DemoCamera;
// class DemoSky;
// class DxVertexShader;
// class DxPixelShader;

struct CDLODRenderStats {
    int RenderedQuads[CDLODQuadTree::c_maxLODLevels];
    int TotalRenderedQuads;
    int TotalRenderedTriangles;

    void Reset() { memset(this, 0, sizeof(*this)); }

    void Add(const CDLODRenderStats& other) {
        for (int i = 0; i < CDLODQuadTree::c_maxLODLevels; i++) RenderedQuads[i] += other.RenderedQuads[i];
        TotalRenderedQuads += other.TotalRenderedQuads;
        TotalRenderedTriangles += other.TotalRenderedTriangles;
    }
};

struct CDLODRendererBatchInfo {
    MapDimensions MapDims;
    const CDLODQuadTree::LODSelection* CDLODSelection;
    int MeshGridDimensions;
    int DetailMeshLODLevelsAffected;  // set to 0 if not using detail mesh!

    // DxVertexShader* VertexShader;
    // DxPixelShader* PixelShader;
    struct RenderData {
        vk::CommandBuffer cmd;
        vk::PipelineLayout pipelineLayout;
        vk::ShaderStageFlags pushConstantStages;
        glm::vec4 dbgCamData;  // .x,.y,.z world position, .w use camera
    } renderData;

    // D3DXHANDLE VSGridDimHandle;
    // D3DXHANDLE VSQuadScaleHandle;
    // D3DXHANDLE VSQuadOffsetHandle;
    // D3DXHANDLE VSMorphConstsHandle;
    struct PerDrawData {
        glm::vec4 data0;  // gridDim:         .x (dimension), .y (dimension/2), .z (2/dimension)
                          //                  .w (LODLevel)
        glm::vec4 data1;  // morph constants: .x (start), .y (1/(end-start)), .z (end/(end-start))
                          //                  .w ((aabb.minZ+aabb.maxZ)/2)
        glm::vec4 data2;  // quadOffset:      .x (aabb.minX), .y (aabb.minY)
                          // quadScale:       .z (aabb.sizeX), .w (aabb.sizeY)
        glm::vec4 data3;  // dbg camera:      .x (wpos.x), .y (wpos.y), .z (wpos.z), .w (use camera)
    };

    // D3DXHANDLE VSUseDetailMapHandle;

    int FilterLODLevel;  // only render quad if it is of FilterLODLevel; if -1 then render all

    CDLODRendererBatchInfo()
        : CDLODSelection(NULL),
          MeshGridDimensions(0),
          // VertexShader(NULL),
          // PixelShader(NULL),
          // VSGridDimHandle(0),
          FilterLODLevel(-1) {}
};

class CDLODRenderer {
   private:
    const Context* m_pContext;

   public:
    /* This confused me greatly. Only one grid mesh is ever used in the demo. The grid mesh that is used is determined by
     * CDLODRendererBatchInfo::MeshGridDimensions via terrainGridMeshDims_, which is in turn determined by
     * LeafQuadTreeNodeSize * RenderGridResolutionMult (.ini settings). I'm guessing the original idea here was people would
     * have different settings in their .ini files, and instead of creating the meshes based on those values just have a
     * predetermined list??? I don't know. CH
     */
    std::vector<VkGridMesh> m_gridMeshes;

    struct PerQuadTreeData {
        glm::vec4 terrainScale;
        glm::vec4 terrainOffset;
        glm::vec4 heightmapTextureInfo;
        // glm::vec2 quadWorldMax;  // .xy max used to clamp triangles outside of world range
        // glm::vec2 samplerWorldToTextureScale;
        glm::vec4 data0;
    };

   public:
    CDLODRenderer(void);
    ~CDLODRenderer(void);
    //
    void init(const Context& context);
    virtual void destroy() { reset(); }
    //
   public:
    //
    void SetIndependentGlobalVertexShaderConsts(const CDLODQuadTree& cdlodQuadTree, PerQuadTreeData& data) const;
    vk::Result Render(const CDLODRendererBatchInfo& batchInfo, CDLODRenderStats* renderStats = NULL) const;
    //
   protected:
    //
    const VkGridMesh* PickGridMesh(int dimensions) const;
    //
   private:
    void reset();
};

#endif  // !_CDLOD_RENDERER_H_