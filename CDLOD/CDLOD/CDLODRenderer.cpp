//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2020 Colin Hughes<colin.s.hughes @gmail.com>
// -------------------------------
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "CDLODRenderer.h"

#include "CDLODQuadTree.h"

// Not sure why this was hardcoded to 7. Shouldn't it be dynamic? I believe the meshes are not remade on any regular interval
// so stack v. heap memory shouldn't be an issue. I'm not sure, but I'm changing it for now. CH
constexpr int NUM_GRID_MESHES = 7;

//
CDLODRenderer::CDLODRenderer(const Context& context) : m_pContext(&context) {
    m_gridMeshes.assign(NUM_GRID_MESHES, context);
    for (int i = 0, dim = 2; i < NUM_GRID_MESHES; i++, dim *= 2) {
        m_gridMeshes[i].SetDimensions(dim);
    }
}
//
CDLODRenderer::~CDLODRenderer(void) {}
//
const VkGridMesh* CDLODRenderer::PickGridMesh(int dimensions) const {
    for (auto& gridMesh : m_gridMeshes)
        if (gridMesh.GetDimensions() == dimensions) return &gridMesh;
    return NULL;
}
//
void CDLODRenderer::SetIndependentGlobalVertexShaderConsts(UniformData& data, const CDLODQuadTree& cdlodQuadTree) const {
    int textureWidth = cdlodQuadTree.GetRasterSizeX();
    int textureHeight = cdlodQuadTree.GetRasterSizeY();

    const MapDimensions& mapDims = cdlodQuadTree.GetWorldMapDims();

    data = {};

    // Used to clamp edges to correct terrain size (only max-es needs clamping, min-s are clamped implicitly)
    data.worldMax = {mapDims.MaxX(), mapDims.MaxY()};

    data.terrainScale = {mapDims.SizeX, mapDims.SizeY, mapDims.SizeZ, 0.0f};
    data.terrainOffset = {mapDims.MinX, mapDims.MinY, mapDims.MinZ, 0.0f};
    data.samplerWorldToTextureScale = {(textureWidth - 1.0f) / (float)textureWidth,
                                       (textureHeight - 1.0f) / (float)textureHeight};
    data.heightmapTextureInfo = {(float)textureWidth, (float)textureHeight, 1.0f / (float)textureWidth,
                                 1.0f / (float)textureHeight};
}
//
vk::Result CDLODRenderer::Render(const CDLODRendererBatchInfo& batchInfo, CDLODRenderStats* renderStats) const {
    // IDirect3DDevice9* device = GetD3DDevice();
    // HRESULT hr;

    const VkGridMesh* gridMesh = PickGridMesh(batchInfo.MeshGridDimensions);
    assert(gridMesh != NULL);
    if (gridMesh == NULL) return vk::Result::eErrorUnknown;

    if (renderStats != NULL) renderStats->Reset();

    //////////////////////////////////////////////////////////////////////////
    // Setup mesh
    // V(device->SetStreamSource(0, (IDirect3DVertexBuffer9*)gridMesh->GetVertexBuffer(), 0, sizeof(PositionVertex)));
    // V(device->SetIndices((IDirect3DIndexBuffer9*)gridMesh->GetIndexBuffer()));
    // V(device->SetFVF(PositionVertex::FVF));
    //{
    //    batchInfo.VertexShader->SetFloatArray(batchInfo.VSGridDimHandle, (float)gridMesh->GetDimensions(),
    //                                          gridMesh->GetDimensions() * 0.5f, 2.0f / gridMesh->GetDimensions(), 0.0f);
    //}
    assert(false);
    // bindVertextBuffers????
    //////////////////////////////////////////////////////////////////////////

    const CDLODQuadTree::SelectedNode* selectionArray = batchInfo.CDLODSelection->GetSelection();
    const int selectionCount = batchInfo.CDLODSelection->GetSelectionCount();

    int qtRasterX = batchInfo.CDLODSelection->GetQuadTree()->GetRasterSizeX();
    int qtRasterY = batchInfo.CDLODSelection->GetQuadTree()->GetRasterSizeY();
    MapDimensions mapDims = batchInfo.CDLODSelection->GetQuadTree()->GetWorldMapDims();

    int prevMorphConstLevelSet = -1;
    for (int i = 0; i < selectionCount; i++) {
        const CDLODQuadTree::SelectedNode& nodeSel = selectionArray[i];

        // Filter out the node if required
        if (batchInfo.FilterLODLevel != -1 && batchInfo.FilterLODLevel != nodeSel.LODLevel) continue;

        // Set LOD level specific consts (if changed from previous node - shouldn't change too much as array should be
        // sorted)
        if (prevMorphConstLevelSet != nodeSel.LODLevel) {
            prevMorphConstLevelSet = nodeSel.LODLevel;
            float v[4];
            batchInfo.CDLODSelection->GetMorphConsts(prevMorphConstLevelSet, v);
            assert(false);
            // batchInfo.VertexShader->SetFloatArray(batchInfo.VSMorphConstsHandle, v, 4);

            bool useDetailMap = batchInfo.DetailMeshLODLevelsAffected > nodeSel.LODLevel;
            assert(false);
            // if (batchInfo.VSUseDetailMapHandle != NULL) {
            //    V(batchInfo.VertexShader->SetBool(batchInfo.VSUseDetailMapHandle, useDetailMap));
            //}
        }

        // debug render - should account for unselected parts below!
        // GetCanvas3D()->DrawBox( D3DXVECTOR3(worldX, worldY, m_mapDims.MinZ), D3DXVECTOR3(worldX+worldSizeX,
        // worldY+worldSizeY, m_mapDims.MaxZ()), 0xFF0000FF, 0x00FF0080 );

        bool drawFull = nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR;

        AABB boundingBox;
        nodeSel.GetAABB(boundingBox, qtRasterX, qtRasterY, mapDims);

        assert(false);
        // V(batchInfo.VertexShader->SetFloatArray(batchInfo.VSQuadScaleHandle, (boundingBox.Max.x - boundingBox.Min.x),
        //                                        (boundingBox.Max.y - boundingBox.Min.y), (float)nodeSel.LODLevel, 0.0f));

        assert(false);
        // V(batchInfo.VertexShader->SetFloatArray(batchInfo.VSQuadOffsetHandle, boundingBox.Min.x, boundingBox.Min.y,
        //                                        (boundingBox.Min.z + boundingBox.Max.z) * 0.5f, 0.0f));

        int gridDim = gridMesh->GetDimensions();

        int renderedTriangles = 0;

        int totalVertices = (gridDim + 1) * (gridDim + 1);
        int totalIndices = gridDim * gridDim * 2 * 3;
        if (drawFull) {
            assert(false);
            // V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, totalIndices / 3));
            renderedTriangles += totalIndices / 3;
        } else {
            int halfd = ((gridDim + 1) / 2) * ((gridDim + 1) / 2) * 2;

            // can be optimized by combining calls
            if (nodeSel.TL) {
                assert(false);
                // V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, halfd));
                renderedTriangles += halfd;
            }
            if (nodeSel.TR) {
                assert(false);
                // V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndTL(),
                // halfd));
                renderedTriangles += halfd;
            }
            if (nodeSel.BL) {
                assert(false);
                // V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndTR(),
                // halfd));
                renderedTriangles += halfd;
            }
            if (nodeSel.BR) {
                assert(false);
                // V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndBL(),
                // halfd));
                renderedTriangles += halfd;
            }
        }

        if (renderStats != NULL) {
            renderStats->RenderedQuads[nodeSel.LODLevel]++;
            renderStats->TotalRenderedQuads++;
            renderStats->TotalRenderedTriangles += renderedTriangles;
        }
    }

    return vk::Result::eSuccess;
}
