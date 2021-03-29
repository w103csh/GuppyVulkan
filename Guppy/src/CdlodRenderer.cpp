/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "CdlodRenderer.h"

#include <Common/Helpers.h>

#include "Box.h"
#include "Cdlod.h"
#include "Instance.h"
#include "Material.h"
#include "MeshConstants.h"
#include "RenderPassManager.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

#define DEBUG_PRINT false
#if DEBUG_PRINT
#pragma warning(disable : 4996)
#include <stdio.h>
namespace {
constexpr bool LOG_TO_FILE = false;
static FILE* pLogFile = nullptr;
void logStart() {
    if (LOG_TO_FILE) {
        pLogFile = fopen("CdlodRenderer_render.log", "w+");
        assert(pLogFile);
    }
}
template <typename... Args>
void log(char* s, Args... args) {
    if (LOG_TO_FILE) {
        assert(pLogFile != nullptr);
        fprintf(pLogFile, s, args...);
    } else {
        printf(s, args...);
    }
}
void logEnd() {
    assert(pLogFile != nullptr);
    fclose(pLogFile);
}
}  // namespace
#else
namespace {
void logStart() {}
template <typename... Args>
void log(char* s, Args... args) {}
void logEnd() {}
}  // namespace
#endif

namespace Cdlod {
namespace Renderer {

Base::Base(Scene::Handler& handler)
    : Handlee<Scene::Handler>(handler),  //
      CDLODRenderer(),
      settings_(),
      maxRenderGridResolutionMult_(0),
      terrainGridMeshDims_(0),
      pHeightmap_(nullptr),
      cdlodQuadTree_(),
      enableDebug_(false),
      useDebugCamera_(true),
      useDebugBoxes_(false),
      useDebugWireframe_(false),
      useDebugTexture_(true),
      whiteBoxOffset_(Mesh::BAD_OFFSET),
      redBoxOffset_(Mesh::BAD_OFFSET),
      greenBoxOffset_(Mesh::BAD_OFFSET),
      blueBoxOffset_(Mesh::BAD_OFFSET) {}

void Base::init(const IHeightmapSource* pHeightmap, const MapDimensions* pMapDimensions) {
    reset();
    CDLODRenderer::init(handler().shell().context());

    if (pHeightmap == nullptr || pMapDimensions == nullptr) {
        // I didn't feel like doing proper inheritance so I just detect the debug renderer
        // by nullptr arguments.
        assert(pHeightmap == nullptr && pMapDimensions == nullptr);
        pHeightmap_ = &dbgHeighmap_;
        rasterWidth_ = pHeightmap_->GetSizeX();
        rasterHeight_ = pHeightmap_->GetSizeY();
        mapDims_ = dbgHeighmap_.mapDims;
    } else {
        pHeightmap_ = pHeightmap;
        rasterWidth_ = pHeightmap_->GetSizeX();
        rasterHeight_ = pHeightmap_->GetSizeY();
        mapDims_ = *pMapDimensions;
    }

    {  // DEFAULTS
        settings_.LeafQuadTreeNodeSize = 8;
        settings_.RenderGridResolutionMult = 4;  // 8;
        settings_.LODLevelCount = 8;
        settings_.MinViewRange = 35000.0f;
        settings_.MaxViewRange = 100000.0f;
        settings_.LODLevelDistanceRatio = 2.0f;
    }

    // GRID MESHES
    for (auto& gridMesh : m_gridMeshes) {
        auto pLdgRes = handler().loadingHandler().createLoadingResources();
        gridMesh.CreateBuffers(*pLdgRes.get());
        handler().loadingHandler().loadSubmit(std::move(pLdgRes));
    }

    maxRenderGridResolutionMult_ = 1;
    while (maxRenderGridResolutionMult_ * settings_.LeafQuadTreeNodeSize <= 128) maxRenderGridResolutionMult_ *= 2;
    maxRenderGridResolutionMult_ /= 2;

    {  // VALIDATION
        if (!helpers::isPowerOfTwo(settings_.LeafQuadTreeNodeSize) || (settings_.LeafQuadTreeNodeSize < 2) ||
            (settings_.LeafQuadTreeNodeSize > 1024)) {
            assert(false && "CDLOD:LeafQuadTreeNodeSize setting is incorrect");
            exit(EXIT_FAILURE);
        }

        if (!helpers::isPowerOfTwo(settings_.RenderGridResolutionMult) || (settings_.RenderGridResolutionMult < 1) ||
            (settings_.LeafQuadTreeNodeSize > maxRenderGridResolutionMult_)) {
            assert(false && "CDLOD:RenderGridResolutionMult setting is incorrect");
            exit(EXIT_FAILURE);
        }

        // Is less than 2 incorrect because the smallest grid size is 2x2 maybe? If this is the case then
        // c_maxLODLevels probably wouldn't be 15. CH
        if ((settings_.LODLevelCount < 2) || (settings_.LODLevelCount > CDLODQuadTree::c_maxLODLevels)) {
            assert(false && "CDLOD:LODLevelCount setting is incorrect");
            exit(EXIT_FAILURE);
        }

        if ((settings_.MinViewRange < 1.0f) || (settings_.MinViewRange > 10000000.0f)) {
            assert(false && "MinViewRange setting is incorrect");
            exit(EXIT_FAILURE);
        }
        if ((settings_.MaxViewRange < 1.0f) || (settings_.MaxViewRange > 10000000.0f) ||
            (settings_.MinViewRange > settings_.MaxViewRange)) {
            assert(false && "MaxViewRange setting is incorrect");
            exit(EXIT_FAILURE);
        }

        if ((settings_.LODLevelDistanceRatio < 1.5f) || (settings_.LODLevelDistanceRatio > 16.0f)) {
            assert(false && "LODLevelDistanceRatio setting is incorrect");
            exit(EXIT_FAILURE);
        }
    }

    terrainGridMeshDims_ = settings_.LeafQuadTreeNodeSize * settings_.RenderGridResolutionMult;

    // QUAD TREE
    CDLODQuadTree::CreateDesc createDesc = {};
    createDesc.pHeightmap = pHeightmap_;
    createDesc.LeafRenderNodeSize = settings_.LeafQuadTreeNodeSize;
    createDesc.LODLevelCount = settings_.LODLevelCount;
    createDesc.MapDims = mapDims_;
    assert(createDesc.pHeightmap);
    cdlodQuadTree_.Create(createDesc);
    // Initialize the quad tree uniform data.
    handler().uniformHandler().cdlodQdTrMgr().insert(handler().shell().context().dev, true);

    // DEBUG
    if (enableDebug_) {
        {  // Create four materials for debugging grid mesh lod levels.
            Material::Default::CreateInfo matInfo = {};
            matInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_FLAT_SHADE;
            matInfo.color = COLOR_WHITE;
            pMaterials_[0] = handler().materialHandler().makeMaterial(&matInfo).get();
            matInfo.color = COLOR_RED;
            pMaterials_[1] = handler().materialHandler().makeMaterial(&matInfo).get();
            matInfo.color = COLOR_GREEN;
            pMaterials_[2] = handler().materialHandler().makeMaterial(&matInfo).get();
            matInfo.color = COLOR_BLUE;
            pMaterials_[3] = handler().materialHandler().makeMaterial(&matInfo).get();
        }

        if (useDebugBoxes_) {
            Mesh::CreateInfo boxInfo = {};
            boxInfo.pipelineType = GRAPHICS::DEFERRED_MRT_WF_COLOR;
            boxInfo.settings.geometryInfo.doubleSided = true;

            Instance::Obj3d::CreateInfo instObj3dInfo = {};
            instObj3dInfo.data.assign(75, {});
            instObj3dInfo.update = false;

            Material::Default::CreateInfo defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_FLAT_SHADE;

            // WHITE
            defMatInfo.color = COLOR_WHITE;
            auto& pW = handler().meshHandler().makeColorMesh<Mesh::Box::Color>(&boxInfo, &defMatInfo, &instObj3dInfo);
            instObj3dInfo.pSharedData = nullptr;  // Make sure we don't share the instance data!
            pW->setActiveCount(0);
            handler().sceneHandler().getActiveScene()->addMeshIndex(MESH::COLOR, pW->getOffset());
            whiteBoxOffset_ = pW->getOffset();
            // RED
            defMatInfo.color = COLOR_RED;
            auto& pR = handler().meshHandler().makeColorMesh<Mesh::Box::Color>(&boxInfo, &defMatInfo, &instObj3dInfo);
            instObj3dInfo.pSharedData = nullptr;  // Make sure we don't share the instance data!
            pR->setActiveCount(0);
            handler().sceneHandler().getActiveScene()->addMeshIndex(MESH::COLOR, pR->getOffset());
            redBoxOffset_ = pR->getOffset();
            // GREEN
            defMatInfo.color = COLOR_GREEN;
            auto& pG = handler().meshHandler().makeColorMesh<Mesh::Box::Color>(&boxInfo, &defMatInfo, &instObj3dInfo);
            instObj3dInfo.pSharedData = nullptr;  // Make sure we don't share the instance data!
            pG->setActiveCount(0);
            handler().sceneHandler().getActiveScene()->addMeshIndex(MESH::COLOR, pG->getOffset());
            greenBoxOffset_ = pG->getOffset();
            // BLUE
            defMatInfo.color = COLOR_BLUE;
            auto& pB = handler().meshHandler().makeColorMesh<Mesh::Box::Color>(&boxInfo, &defMatInfo, &instObj3dInfo);
            instObj3dInfo.pSharedData = nullptr;  // Make sure we don't share the instance data!
            pB->setActiveCount(0);
            handler().sceneHandler().getActiveScene()->addMeshIndex(MESH::COLOR, pB->getOffset());
            blueBoxOffset_ = pB->getOffset();
        }

        if (useDebugTexture_) {
            // Create a material for debugging texture coordinates
            Material::Default::CreateInfo matInfo = {};
            matInfo = {};
            matInfo.pTexture = handler().textureHandler().getTexture(Texture::CIRCLES_ID);
            pMaterials_[4] = handler().materialHandler().makeMaterial(&matInfo).get();
            // Add the debug texture info settings
            settings_.dbgTexScale = 20.0f;
        }
    }
}

void Cdlod::Renderer::Base::update(const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                   const vk::CommandBuffer& cmd) {
    if (pPipelineBindData->type == PIPELINE{GRAPHICS::CDLOD_WF_DEFERRED}) {
        if (!enableDebug_ || !useDebugWireframe_) return;
    } else if (pPipelineBindData->type == PIPELINE{GRAPHICS::CDLOD_TEX_DEFERRED}) {
        if (!enableDebug_ || !useDebugTexture_) return;
    } else {
        return;
    }

    Camera::FrustumInfo frustumInfo;
    if (enableDebug_ && useDebugCamera_) {
        assert(handler().uniformHandler().hasDebugCamera());
        frustumInfo = handler().uniformHandler().getDebugCamera().getFrustumInfoZupLH();
    } else {
        frustumInfo = handler().uniformHandler().getMainCamera().getFrustumInfoZupLH();
    }

    // CDLOD uses z-up left-handed math for everything. CH
    CDLODQuadTree::LODSelectionOnStack<4096> cdlodSelection(frustumInfo.eye, frustumInfo.farDistance,
                                                            frustumInfo.planes.data(), settings_.LODLevelDistanceRatio);

    cdlodQuadTree_.LODSelect(&cdlodSelection);

    //
    // Check if we have too small visibility distance that causes morph between LOD levels to be incorrect.
    if (cdlodSelection.IsVisDistTooSmall()) {
        assert(false && "Visibility distance might be too low for LOD morph to work correctly!");
    }
    //////////////////////////////////////////////////////////////////////////

    renderTerrain(cdlodSelection, pPipelineBindData, cmd);
}

void Cdlod::Renderer::Base::reset() {
    settings_ = {};
    maxRenderGridResolutionMult_ = terrainGridMeshDims_ = 0;
    pHeightmap_ = nullptr;
    rasterWidth_ = rasterHeight_ = 0;
    mapDims_ = {};
    cdlodQuadTree_ = {};
    // DEBUG
    pMaterials_[0] = pMaterials_[1] = pMaterials_[2] = pMaterials_[3] = nullptr;
    wfDescSetBindDataMap_.clear();
    whiteBoxOffset_ = redBoxOffset_ = greenBoxOffset_ = blueBoxOffset_ = Mesh::BAD_OFFSET;
}

void Base::renderTerrain(const CDLODQuadTree::LODSelection& cdlodSelection,
                         const std::shared_ptr<Pipeline::BindData>& pPipelineBindData, const vk::CommandBuffer& cmd) {
    // HRESULT hr;
    // IDirect3DDevice9* device = GetD3DDevice();

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    // float dbgCl = 0.2f;
    // float dbgLODLevelColors[4][4] = {
    //    {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, dbgCl, dbgCl, 1.0f}, {dbgCl, 1.0f, dbgCl, 1.0f}, {dbgCl, dbgCl, 1.0f, 1.0f}};

    //////////////////////////////////////////////////////////////////////////
    // Local shaders
    // static DxPixelShader psWireframe("Shaders/misc.psh", "justOutputColor");
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Camera and matrices
    // D3DXMATRIX view = camera->GetViewMatrix();
    // D3DXMATRIX proj = camera->GetProjMatrix();
    //
    // minor wireframe offset!
    // D3DXMATRIX offProjWireframe;
    // camera->GetZOffsettedProjMatrix(offProjWireframe, 1.001f, 0.01f);
    //
    // D3DXMATRIX viewProj = view * proj;
    // D3DXMATRIX offViewProjWireframe = view * offProjWireframe;
    //
    // V(m_vsTerrainSimple.SetMatrix("g_viewProjection", viewProj));
    //
    //////////////////////////////////////////////////////////////////////////

    // V(device->SetVertexShader(m_vsTerrainSimple));

    // This contains all settings used to do rendering through CDLODRenderer
    CDLODRendererBatchInfo cdlodBatchInfo = {};

    // cdlodBatchInfo.VertexShader = &m_vsTerrainSimple;
    // cdlodBatchInfo.VSGridDimHandle = m_vsTerrainSimple.GetConstantTable()->GetConstantByName(NULL, "g_gridDim");
    // cdlodBatchInfo.VSQuadScaleHandle = m_vsTerrainSimple.GetConstantTable()->GetConstantByName(NULL, "g_quadScale");
    // cdlodBatchInfo.VSQuadOffsetHandle = m_vsTerrainSimple.GetConstantTable()->GetConstantByName(NULL, "g_quadOffset");
    // cdlodBatchInfo.VSMorphConstsHandle = m_vsTerrainSimple.GetConstantTable()->GetConstantByName(NULL, "g_morphConsts");
    // cdlodBatchInfo.VSUseDetailMapHandle = m_vsTerrainSimple.GetConstantTable()->GetConstantByName(NULL, "g_useDetailMap");
    cdlodBatchInfo.MeshGridDimensions = terrainGridMeshDims_;
    cdlodBatchInfo.DetailMeshLODLevelsAffected = 0;
    cdlodBatchInfo.renderData.cmd = cmd;
    cdlodBatchInfo.renderData.pipelineLayout = pPipelineBindData->layout;
    cdlodBatchInfo.renderData.pushConstantStages = pPipelineBindData->pushConstantStages;
    if (enableDebug_ && useDebugCamera_) {
        cdlodBatchInfo.renderData.dbgCamData = glm::vec4(handler().uniformHandler().getDebugCamera().getPosition(), 1.0f);
    }

    // D3DTEXTUREFILTERTYPE vertexTextureFilterType =
    //    (vaGetBilinearVertexTextureFilterSupport()) ? (D3DTEXF_LINEAR) : (D3DTEXF_POINT);

    //////////////////////////////////////////////////////////////////////////
    // Setup global shader settings
    //
    auto& quadTreeItem = handler().uniformHandler().cdlodQdTrMgr().getTypedItem(0);
    quadTreeItem.dirty = true;
    auto& quadTreeData = quadTreeItem.getData();
    // m_dlodRenderer.SetIndependentGlobalVertexShaderConsts(m_vsTerrainSimple, m_terrainQuadTree, viewProj,
    //                                                      camera->GetPosition());
    SetIndependentGlobalVertexShaderConsts(quadTreeData, cdlodQuadTree_);
    //
    // setup detail heightmap globals if any
    // if (m_settings.DetailHeightmapEnabled) {
    //    m_vsTerrainSimple.SetTexture("g_detailHMVertexTexture", m_detailHMTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP,
    //                                 vertexTextureFilterType, vertexTextureFilterType);
    //    m_psTerrainFlat.SetTexture("g_terrainDetailNMTexture", m_detailNMTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP,
    //                               D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);

    //    int dmWidth = m_detailHeightmap->Width();
    //    int dmHeight = m_detailHeightmap->Height();

    //    float sizeX = (m_mapDims.SizeX / (float)m_rasterWidth) * dmWidth / m_settings.DetailHeightmapXYScale;
    //    float sizeY = (m_mapDims.SizeY / (float)m_rasterHeight) * dmHeight / m_settings.DetailHeightmapXYScale;

    //    cdlodBatchInfo.DetailMeshLODLevelsAffected = m_settings.DetailMeshLODLevelsAffected;

    //    m_vsTerrainSimple.SetFloatArray("g_detailConsts", sizeX, sizeY, m_settings.DetailHeightmapZSize,
    //                                    (float)m_settings.DetailMeshLODLevelsAffected);
    //}
    if (useDebugTexture_) {
        // The aspect of the texture is dealt with by the material. Material::SetDefaultTextureData() has an example of a
        // way to deal with non-square textures if this renderer needs to implement something similar.
        quadTreeData.data1.x = settings_.dbgTexScale;
    }
    //
    // if (m_settings.ShadowmapEnabled) {
    //    m_psTerrainFlat.SetTexture("g_noiseTexture", m_noiseTexture, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_POINT,
    //                               D3DTEXF_POINT, D3DTEXF_POINT);
    //}

    //
    // setup light and fog globals
    {
        //
        // float fogStart	= pCamera->GetViewRange() * 0.75f;
        // float fogEnd	= pCamera->GetViewRange() * 0.98f;
        // V( m_vsTerrainSimple.SetVector( "g_fogConsts", fogEnd / ( fogEnd - fogStart ), -1.0f / ( fogEnd - fogStart ),
        // 0, 1.0f/fogEnd ) );
        // if (lightMgr != NULL) {
        //    D3DXVECTOR4 diffLightDir = D3DXVECTOR4(lightMgr->GetDirectionalLightDir(), 1.0f);
        //    V(m_vsTerrainSimple.SetVector("g_diffuseLightDir", diffLightDir));
        //}
    }
    //
    // vertex textures
    // V(m_vsTerrainSimple.SetTexture("g_terrainHMVertexTexture", m_terrainHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP,
    //                               vertexTextureFilterType, vertexTextureFilterType));
    // V( m_vsTerrainSimple.SetTexture( "g_terrainNMVertexTexture", m_terrainNMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP,
    // D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );
    //
    // global pixel shader consts
    // m_psTerrainFlat.SetFloatArray("g_lightColorDiffuse", c_lightColorDiffuse, 4);
    // m_psTerrainFlat.SetFloatArray("g_lightColorAmbient", c_lightColorAmbient, 4);
    // psTerrainFlatVersions[i]->SetFloatArray( device, "g_fogColor", c_fogColor, 4 );
    //
    // V(m_psTerrainFlat.SetTexture("g_terrainNMTexture", m_terrainNMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP,
    //                             D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR));
    //
    // end of global shader settings
    //////////////////////////////////////////////////////////////////////////
    handler().uniformHandler().update(quadTreeItem);

    //////////////////////////////////////////////////////////////////////////
    // Connect selection to our render batch info
    cdlodBatchInfo.CDLODSelection = &cdlodSelection;

    // m_renderStats.TerrainStats.Reset();

    // CDLODRenderStats stepStats;

    //////////////////////////////////////////////////////////////////////////
    // Debug view
    if (enableDebug_ && useDebugBoxes_) {
        log("\n");
        log("*************************************************************************************\n");
        log("**     Debug View (frame count: %d)\n", handler().game().getFrameCount());
        log("*************************************************************************************\n");
        debugResetBoxes();
        // const auto l = [](glm::vec3 t, AABB& aabb) {
        //    aabb.Min.x += t.x;
        //    aabb.Max.x += t.x;
        //    aabb.Min.y += t.y;
        //    aabb.Max.y += t.y;
        //    aabb.Min.z += t.z;
        //    aabb.Max.z += t.z;
        //};
        // AABB aabb;
        // aabb.Min.x = -1232.0f;
        // aabb.Min.y = 233.0f;
        // aabb.Min.z = 666.0f;
        // aabb.Max.x = 12.0f;
        // aabb.Max.y = 244.0f;
        // aabb.Max.z = 1023.0f;
        // l({-20, 16, -3}, aabb);
        // debugAddBox(3, aabb);
        // debugUpdateBoxes();
        // return;

        for (int i = 0; i < cdlodSelection.GetSelectionCount(); i++) {
            const CDLODQuadTree::SelectedNode& nodeSel = cdlodSelection.GetSelection()[i];
            int lodLevel = nodeSel.LODLevel;

            bool drawFull = nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR;

            // unsigned int penColor = (((int)(dbgLODLevelColors[lodLevel % 4][0] * 255.0f) & 0xFF) << 16) |
            //                        (((int)(dbgLODLevelColors[lodLevel % 4][1] * 255.0f) & 0xFF) << 8) |
            //                        (((int)(dbgLODLevelColors[lodLevel % 4][2] * 255.0f) & 0xFF) << 0) |
            //                        (((int)(dbgLODLevelColors[lodLevel % 4][3] * 255.0f) & 0xFF) << 24);

            AABB boundingBox;
            nodeSel.GetAABB(boundingBox, rasterWidth_, rasterHeight_, mapDims_);
            boundingBox.Expand(-0.003f);
            if (drawFull) {
                // GetCanvas3D()->DrawBox(boundingBox.Min, boundingBox.Max, penColor);
                debugAddBox(lodLevel, boundingBox);
            } else {
                float midX = boundingBox.Center().x;
                float midY = boundingBox.Center().y;

                if (nodeSel.TL) {
                    AABB bbSub = boundingBox;
                    bbSub.Max.x = midX;
                    bbSub.Max.y = midY;
                    bbSub.Expand(-0.002f);
                    // GetCanvas3D()->DrawBox(bbSub.Min, bbSub.Max, penColor);
                    debugAddBox(lodLevel, boundingBox);
                }
                if (nodeSel.TR) {
                    AABB bbSub = boundingBox;
                    bbSub.Min.x = midX;
                    bbSub.Max.y = midY;
                    bbSub.Expand(-0.002f);
                    // GetCanvas3D()->DrawBox(bbSub.Min, bbSub.Max, penColor);
                    debugAddBox(lodLevel, boundingBox);
                }
                if (nodeSel.BL) {
                    AABB bbSub = boundingBox;
                    bbSub.Max.x = midX;
                    bbSub.Min.y = midY;
                    bbSub.Expand(-0.002f);
                    // GetCanvas3D()->DrawBox(bbSub.Min, bbSub.Max, penColor);
                    debugAddBox(lodLevel, boundingBox);
                }
                if (nodeSel.BR) {
                    AABB bbSub = boundingBox;
                    bbSub.Min.x = midX;
                    bbSub.Min.y = midY;
                    bbSub.Expand(-0.002f);
                    // GetCanvas3D()->DrawBox(bbSub.Min, bbSub.Max, penColor);
                    debugAddBox(lodLevel, boundingBox);
                }
            }
        }
        debugUpdateBoxes();
        log("*************************************************************************************\n");
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Render
    //
    for (int i = cdlodSelection.GetMinSelectedLevel(); i <= cdlodSelection.GetMaxSelectedLevel(); i++) {
        // if (m_debugView) {
        //    float whiten = 0.8f;
        //    m_psTerrainFlat.SetFloatArray("g_colorMult", whiten + (1.0f - whiten) * dbgLODLevelColors[i % 4][0],
        //                                  whiten + (1.0f - whiten) * dbgLODLevelColors[i % 4][1],
        //                                  whiten + (1.0f - whiten) * dbgLODLevelColors[i % 4][2],
        //                                  whiten + (1.0f - whiten) * dbgLODLevelColors[i % 4][3]);
        //} else
        //    m_psTerrainFlat.SetFloatArray("g_colorMult", 1.0f, 1.0f, 1.0f, 1.0f);

        cdlodBatchInfo.FilterLODLevel = i;
        // cdlodBatchInfo.PixelShader = &m_psTerrainFlat;

        // DWORD texFilter = D3DTEXF_LINEAR;

        // if (m_settings.ShadowmapEnabled) {
        //    const CascadedShadowMap::CSMLayer& csmLayer = m_shadowsRenderer.GetCascadeLayerForDLODLevel(i);

        //    V(m_vsTerrainSimple.SetMatrix("g_shadowView", csmLayer.ShadowView));
        //    V(m_vsTerrainSimple.SetMatrix("g_shadowProj", csmLayer.ShadowProj));
        //    V(m_psTerrainFlat.SetTexture("g_shadowMapTexture", csmLayer.ShadowMapDepth, D3DTADDRESS_CLAMP,
        //    D3DTADDRESS_CLAMP,
        //                                 texFilter, texFilter, texFilter));
        //    float csmL2LayerSamplingTexelRadius = csmLayer.SamplingTexelRadius;

        //    if (i + 1 < m_shadowsRenderer.GetCascadeLayerCount()) {
        //        const CascadedShadowMap::CSMLayer& csmL2Layer = m_shadowsRenderer.GetCascadeLayerForDLODLevel(i + 1);
        //        V(m_vsTerrainSimple.SetMatrix("g_shadowL2View", csmL2Layer.ShadowView));
        //        V(m_vsTerrainSimple.SetMatrix("g_shadowL2Proj", csmL2Layer.ShadowProj));
        //        m_psTerrainFlat.SetTexture("g_shadowL2MapTexture", csmL2Layer.ShadowMapDepth, D3DTADDRESS_CLAMP,
        //                                   D3DTADDRESS_CLAMP, texFilter, texFilter, texFilter);
        //        csmL2LayerSamplingTexelRadius = csmL2Layer.SamplingTexelRadius;
        //    } else {
        //        V(m_vsTerrainSimple.SetMatrix("g_shadowL2View", csmLayer.ShadowView));
        //        V(m_vsTerrainSimple.SetMatrix("g_shadowL2Proj", csmLayer.ShadowProj));
        //        m_psTerrainFlat.SetTexture("g_shadowL2MapTexture", csmLayer.ShadowMapDepth, D3DTADDRESS_CLAMP,
        //                                   D3DTADDRESS_CLAMP, texFilter, texFilter, texFilter);
        //    }
        //    m_psTerrainFlat.SetFloatArray("g_shadowMapConsts", (float)m_shadowsRenderer.GetTextureResolution(),
        //                                  csmLayer.SamplingTexelRadius, csmL2LayerSamplingTexelRadius, 0.0f);

        //    float dummyNoiseScaleModifier = 0.04f;
        //    float depthNoiseScale = ::max(csmLayer.WorldSpaceTexelSizeX, csmLayer.WorldSpaceTexelSizeY) * 3.0f;
        //    V(m_vsTerrainSimple.SetFloatArray("g_shadowNoiseConsts", csmLayer.NoiseScaleX * dummyNoiseScaleModifier,
        //                                      csmLayer.NoiseScaleY * dummyNoiseScaleModifier, depthNoiseScale, 0.0f));

        //    if (vaGetShadowMapSupport() == smsATIShadows) {
        //        int sms0 = m_psTerrainFlat.GetTextureSamplerIndex("g_shadowMapTexture");
        //        int sms1 = m_psTerrainFlat.GetTextureSamplerIndex("g_shadowL2MapTexture");
        //        device->SetSamplerState(sms0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4);
        //        if (sms1 != -1) device->SetSamplerState(sms1, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4);
        //    }
        //}

        {  // CH
            if (pPipelineBindData->type == PIPELINE{GRAPHICS::CDLOD_WF_DEFERRED}) {
                auto debugLodLevel = i % 4;
                handler().descriptorHandler().getBindData(pPipelineBindData->type, wfDescSetBindDataMap_,
                                                          {pMaterials_[debugLodLevel]});
            } else if (pPipelineBindData->type == PIPELINE{GRAPHICS::CDLOD_TEX_DEFERRED}) {
                handler().descriptorHandler().getBindData(pPipelineBindData->type, wfDescSetBindDataMap_, {pMaterials_[4]});
            } else {
                assert(false);
            }
            assert(wfDescSetBindDataMap_.size() == 1);
            const auto& descSetBindData = wfDescSetBindDataMap_.begin()->second;
            const auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1),
                                             handler().passHandler().renderPassMgr().getFrameIndex());
            cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                   descSetBindData.descriptorSets[setIndex], descSetBindData.dynamicOffsets);
        }

        // V(device->SetPixelShader(*cdlodBatchInfo.PixelShader));
        // m_dlodRenderer.Render(cdlodBatchInfo, &stepStats);
        Render(cdlodBatchInfo, nullptr);  // CH
        // m_renderStats.TerrainStats.Add(stepStats);

        // if (m_settings.ShadowmapEnabled && vaGetShadowMapSupport() == smsATIShadows) {
        //    int sms0 = m_psTerrainFlat.GetTextureSamplerIndex("g_shadowMapTexture");
        //    int sms1 = m_psTerrainFlat.GetTextureSamplerIndex("g_shadowL2MapTexture");
        //    device->SetSamplerState(sms0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1);
        //    if (sms1 != -1) device->SetSamplerState(sms1, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1);
        //}
    }
    cdlodBatchInfo.FilterLODLevel = -1;
    ////
    // if (m_settings.ShadowmapEnabled) {
    //    m_psTerrainFlat.SetTexture("g_shadowMapTexture", NULL);
    //    m_psTerrainFlat.SetTexture("g_shadowL2MapTexture", NULL);
    //}
    ////
    // if (m_wireframe) {
    //    V(m_vsTerrainSimple.SetMatrix("g_viewProjection", offViewProjWireframe));
    //    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    //    psWireframe.SetFloatArray("g_justOutputColorValue", 0.0f, 0.0f, 0.0f, 1.0f);

    //    cdlodBatchInfo.PixelShader = &psWireframe;
    //    V(device->SetPixelShader(*cdlodBatchInfo.PixelShader));
    //    m_dlodRenderer.Render(cdlodBatchInfo, &stepStats);
    //    m_renderStats.TerrainStats.Add(stepStats);

    //    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    //}
    ////////////////////////////////////////////////////////////////////////////

    // V(device->SetStreamSource(0, NULL, 0, 0));
    // V(device->SetIndices(NULL));

    // V(device->SetVertexShader(NULL));
    // V(device->SetPixelShader(NULL));
}

void Cdlod::Renderer::Base::debugResetBoxes() {
    handler().meshHandler().getColorMesh(whiteBoxOffset_)->setActiveCount(0);
    handler().meshHandler().getColorMesh(redBoxOffset_)->setActiveCount(0);
    handler().meshHandler().getColorMesh(greenBoxOffset_)->setActiveCount(0);
    handler().meshHandler().getColorMesh(blueBoxOffset_)->setActiveCount(0);
}

void Cdlod::Renderer::Base::debugUpdateBoxes() {
    handler().meshHandler().updateMesh(handler().meshHandler().getColorMesh(whiteBoxOffset_));
    handler().meshHandler().updateMesh(handler().meshHandler().getColorMesh(redBoxOffset_));
    handler().meshHandler().updateMesh(handler().meshHandler().getColorMesh(greenBoxOffset_));
    handler().meshHandler().updateMesh(handler().meshHandler().getColorMesh(blueBoxOffset_));
}

void Cdlod::Renderer::Base::debugAddBox(const int lodLevel, const AABB& aabb) {
    const auto printInfo = [&](const glm::vec3& t, const glm::vec3& s) {  // CH
#if DEBUG_PRINT
        log(" lod: %d aabb: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f) translate: %s scale: %s\n", lodLevel, aabb.Min.x,
            aabb.Min.y, aabb.Min.z, aabb.Max.x, aabb.Max.y, aabb.Max.z, helpers::toString(t).c_str(),
            helpers::toString(s).c_str());
#endif
    };

    Mesh::index offset;
    // Determine the color based on LOD level.
    switch (lodLevel % 4) {
        case 0: {
            offset = whiteBoxOffset_;
            log("%*c w(%d)- ", (lodLevel + 1) * 4, ' ', handler().meshHandler().getColorMesh(offset)->getInstanceCount());
        } break;
        case 1: {
            offset = redBoxOffset_;
            log("%*c r(%d)- ", (lodLevel + 1) * 4, ' ', handler().meshHandler().getColorMesh(offset)->getInstanceCount());
        } break;
        case 2: {
            offset = greenBoxOffset_;
            log("%*c g(%d)- ", (lodLevel + 1) * 4, ' ', handler().meshHandler().getColorMesh(offset)->getInstanceCount());
        } break;
        case 3: {
            offset = blueBoxOffset_;
            log("%*c b(%d)- ", (lodLevel + 1) * 4, ' ', handler().meshHandler().getColorMesh(offset)->getInstanceCount());
        } break;
        default: {
            assert(false);
            abort();
        }
    }

    // CDLOD uses z-up left-handed math for everything, so do the conversion on the incoming aabb.
    glm::vec3 translate = helpers::convertToZupLH({
        (aabb.Max.x + aabb.Min.x) / 2.0f,
        (aabb.Max.y + aabb.Min.y) / 2.0f,
        (aabb.Max.z + aabb.Min.z) / 2.0f,
    });
    glm::vec3 scale =
        helpers::convertToZupLH({(aabb.Max.x - aabb.Min.x), (aabb.Max.y - aabb.Min.y), (aabb.Max.z - aabb.Min.z)});
    glm::mat4 model = helpers::affine(scale, translate);

    printInfo(translate, scale);

    auto& pBox = handler().meshHandler().getColorMesh(offset);
    pBox->setModel(model, pBox->getInstanceCount());
    pBox->setActiveCount(pBox->getActiveCount() + 1);
}

}  // namespace Renderer
}  // namespace Cdlod