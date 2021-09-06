//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2021 Colin Hughes<colin.s.hughes @gmail.com>
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
CDLODRenderer::CDLODRenderer() : m_pContext(nullptr) {}
//
CDLODRenderer::~CDLODRenderer(void) {}
//
void CDLODRenderer::init(const Context& context) {
    reset();
    m_pContext = &context;
    m_gridMeshes.reserve(NUM_GRID_MESHES);
    for (int i = 0, dim = 2; i < NUM_GRID_MESHES; i++, dim *= 2) {
        m_gridMeshes.emplace_back(context);
        m_gridMeshes.back().SetDimensions(dim);
    }
}
void CDLODRenderer::reset() {
    if (m_pContext != nullptr) {
        for (auto& gridMesh : m_gridMeshes) gridMesh.destroy();
        m_pContext = nullptr;
    }
}
//
const VkGridMesh* CDLODRenderer::PickGridMesh(int dimensions) const {
    for (auto& gridMesh : m_gridMeshes)
        if (gridMesh.GetDimensions() == dimensions) return &gridMesh;
    return NULL;
}
//
void CDLODRenderer::SetIndependentGlobalVertexShaderConsts(const CDLODQuadTree& cdlodQuadTree, PerQuadTreeData& data) const {
    const auto textureWorldSize = cdlodQuadTree.GetTextureWorldSize();
    const auto textureSize = cdlodQuadTree.GetTextureSize();
    const MapDimensions& mapDims = cdlodQuadTree.GetWorldMapDims();

    data = {};

    // Used to clamp edges to correct terrain size (only max-es needs clamping, min-s are clamped implicitly)
    data.data0.x = mapDims.MaxX();  // quadWorldMax
    data.data0.y = mapDims.MaxY();  // quadWorldMax

    data.terrainScale = {mapDims.SizeX, mapDims.SizeY, mapDims.SizeZ, 0.0f};
    data.terrainOffset = {mapDims.MinX, mapDims.MinY, mapDims.MinZ, 0.0f};
    // data.data0.z = (textureWidth - 1.0f) / (float)textureWidth;    // samplerWorldToTextureScale
    // data.data0.w = (textureHeight - 1.0f) / (float)textureHeight;  // samplerWorldToTextureScale
    data.heightmapTextureInfo = {textureSize.x, textureSize.y, 1.0f / textureWorldSize.x, 1.0f / textureWorldSize.y};
}
//
vk::Result CDLODRenderer::Render(const CDLODRendererBatchInfo& batchInfo, CDLODRenderStats* renderStats) const {
    // IDirect3DDevice9* device = GetD3DDevice();
    // HRESULT hr;

    const VkGridMesh* gridMesh = PickGridMesh(batchInfo.MeshGridDimensions);
    assert(gridMesh != NULL);
    if (gridMesh == NULL) return vk::Result::eErrorUnknown;

    if (renderStats != NULL) renderStats->Reset();

    CDLODRendererBatchInfo::PerDrawData perDrawData = {};  // CH

    //////////////////////////////////////////////////////////////////////////
    // Setup mesh
    // V(device->SetStreamSource(0, (IDirect3DVertexBuffer9*)gridMesh->GetVertexBuffer(), 0, sizeof(PositionVertex)));
    vk::DeviceSize offset = 0;
    batchInfo.renderData.cmd.bindVertexBuffers(0, 1, &gridMesh->GetVertexBuffer().buffer, &offset);
    // V(device->SetIndices((IDirect3DIndexBuffer9*)gridMesh->GetIndexBuffer()));
    batchInfo.renderData.cmd.bindIndexBuffer(gridMesh->GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);
    // V(device->SetFVF(PositionVertex::FVF));
    //{
    //    batchInfo.VertexShader->SetFloatArray(batchInfo.VSGridDimHandle, (float)gridMesh->GetDimensions(),
    //                                          gridMesh->GetDimensions() * 0.5f, 2.0f / gridMesh->GetDimensions(), 0.0f);
    //}
    perDrawData.data0 = {(float)gridMesh->GetDimensions(), gridMesh->GetDimensions() * 0.5f,
                         2.0f / gridMesh->GetDimensions(), 0.0};
    perDrawData.data3 = batchInfo.renderData.dbgCamData;
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
            batchInfo.CDLODSelection->GetMorphConsts(prevMorphConstLevelSet, perDrawData.data1);
            perDrawData.data0.w = (float)nodeSel.LODLevel;
            // batchInfo.VertexShader->SetFloatArray(batchInfo.VSMorphConstsHandle, v, 4);

            // bool useDetailMap = batchInfo.DetailMeshLODLevelsAffected > nodeSel.LODLevel;
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

        // V(batchInfo.VertexShader->SetFloatArray(batchInfo.VSQuadScaleHandle, (boundingBox.Max.x - boundingBox.Min.x),
        //                                        (boundingBox.Max.y - boundingBox.Min.y), (float)nodeSel.LODLevel, 0.0f));

        // V(batchInfo.VertexShader->SetFloatArray(batchInfo.VSQuadOffsetHandle, boundingBox.Min.x, boundingBox.Min.y,
        //                                        (boundingBox.Min.z + boundingBox.Max.z) * 0.5f, 0.0f));

        perDrawData.data1.w = (boundingBox.Min.z + boundingBox.Max.z) * 0.5f;
        perDrawData.data2 = {boundingBox.Min.x, boundingBox.Min.y, (boundingBox.Max.x - boundingBox.Min.x),
                             (boundingBox.Max.y - boundingBox.Min.y)};

        batchInfo.renderData.cmd.pushConstants(batchInfo.renderData.pipelineLayout, batchInfo.renderData.pushConstantStages,
                                               0, sizeof(CDLODRendererBatchInfo::PerDrawData), &perDrawData);

        const uint32_t numIdxPerQuad = gridMesh->GetNumIndiciesPerQuadrant();
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        bool haveDraw = false;

        if (nodeSel.TL) {
            haveDraw = true;
            indexCount = numIdxPerQuad;
        }

        if (nodeSel.TR) {
            indexCount = numIdxPerQuad + (haveDraw ? indexCount : 0);
            firstIndex = haveDraw ? firstIndex : gridMesh->GetIndexEndTL();
            haveDraw = true;
        } else if (haveDraw) {
            batchInfo.renderData.cmd.drawIndexed(indexCount, 1, firstIndex, 0, 0);
            haveDraw = false;
        }

        if (nodeSel.BL) {
            indexCount = numIdxPerQuad + (haveDraw ? indexCount : 0);
            firstIndex = haveDraw ? firstIndex : gridMesh->GetIndexEndTR();
            haveDraw = true;
        } else if (haveDraw) {
            batchInfo.renderData.cmd.drawIndexed(indexCount, 1, firstIndex, 0, 0);
            haveDraw = false;
        }

        if (nodeSel.BR) {
            indexCount = numIdxPerQuad + (haveDraw ? indexCount : 0);
            firstIndex = haveDraw ? firstIndex : gridMesh->GetIndexEndBL();
            haveDraw = true;
        }

        if (haveDraw) {
            batchInfo.renderData.cmd.drawIndexed(indexCount, 1, firstIndex, 0, 0);
        }

        // if (nodeSel.TL) {
        //    batchInfo.renderData.cmd.drawIndexed(gridMesh->GetNumIndiciesPerQuadrant(), 1, 0, 0, 0);
        //}
        // if (nodeSel.TR) {
        //    batchInfo.renderData.cmd.drawIndexed(gridMesh->GetNumIndiciesPerQuadrant(), 1, gridMesh->GetIndexEndTL(), 0,
        //    0);
        //}
        // if (nodeSel.BL) {
        //    batchInfo.renderData.cmd.drawIndexed(gridMesh->GetNumIndiciesPerQuadrant(), 1, gridMesh->GetIndexEndTR(), 0,
        //    0);
        //}
        // if (nodeSel.BR) {
        //    batchInfo.renderData.cmd.drawIndexed(gridMesh->GetNumIndiciesPerQuadrant(), 1, gridMesh->GetIndexEndBL(), 0,
        //    0);
        //}

        // int gridDim = gridMesh->GetDimensions();

        // int renderedTriangles = 0;

        // int totalVertices = (gridDim + 1) * (gridDim + 1);
        // int totalIndices = gridDim * gridDim * 2 * 3;
        // if (drawFull) {
        //    V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, totalIndices / 3));
        //    renderedTriangles += totalIndices / 3;
        //} else {
        //    int halfd = ((gridDim + 1) / 2) * ((gridDim + 1) / 2) * 2;

        //    can be optimized by combining calls if (nodeSel.TL) {
        //        V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, halfd));
        //        renderedTriangles += halfd;
        //    }
        //    if (nodeSel.TR) {
        //        V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndTL(), halfd));
        //        renderedTriangles += halfd;
        //    }
        //    if (nodeSel.BL) {
        //        V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndTR(), halfd));
        //        renderedTriangles += halfd;
        //    }
        //    if (nodeSel.BR) {
        //        V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, totalVertices, gridMesh->GetIndexEndBL(), halfd));
        //        renderedTriangles += halfd;
        //    }
        //}

        // if (renderStats != NULL) {
        //    renderStats->RenderedQuads[nodeSel.LODLevel]++;
        //    renderStats->TotalRenderedQuads++;
        //    renderStats->TotalRenderedTriangles += renderedTriangles;
        //}
    }

    return vk::Result::eSuccess;
}
