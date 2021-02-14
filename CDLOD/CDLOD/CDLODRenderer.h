//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2020 Colin Hughes<colin.s.hughes @gmail.com>
// -------------------------------
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#ifndef CDLOD_RENDERER_H
#define CDLOD_RENDERER_H

#include <glm/glm.hpp>
#include <vector>

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

    // D3DXHANDLE VSGridDimHandle;
    // D3DXHANDLE VSQuadScaleHandle;
    // D3DXHANDLE VSQuadOffsetHandle;
    // D3DXHANDLE VSMorphConstsHandle;
    struct PerDrawData {
        // float4 g_quadScale;  // .z holds the current LOD level
        glm::vec4 quadScale;
        // float g_quadOffset;  // .z holds the z center of the AABB
        glm::vec4 quadOffset;
        // float4 g_morphConsts;
        glm::vec4 morphConsts;
        // float3 g_gridDim;    // .x = gridDim, .y = gridDimHalf, .z = oneOverGridDimHalf
        glm::vec3 gridDim;
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
    std::vector<VkGridMesh> m_gridMeshes;

    struct PerQuadTreeData {
        glm::vec4 terrainScale;
        glm::vec4 terrainOffset;
        glm::vec4 heightmapTextureInfo;
        glm::vec2 worldMax;  // .xy max used to clamp triangles outside of world range
        glm::vec2 samplerWorldToTextureScale;
    };

   public:
    CDLODRenderer(const Context& context);
    ~CDLODRenderer(void);
    //
    // public:
   protected:
    //
    void SetIndependentGlobalVertexShaderConsts(PerQuadTreeData& data, const CDLODQuadTree& cdlodQuadTree) const;
    vk::Result Render(const CDLODRendererBatchInfo& batchInfo, CDLODRenderStats* renderStats = NULL) const;
    //
    // protected:
    //
    const VkGridMesh* PickGridMesh(int dimensions) const;
    //
};

#endif  // !CDLOD_RENDERER_H