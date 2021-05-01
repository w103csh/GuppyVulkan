/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include <algorithm>
#include <iterator>

#include "OceanRenderer.h"

#include "Ocean.h"
#include "RenderPassManager.h"
#include "Tessellation.h"
// HANDLERS
#include "MaterialHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

namespace Ocean {

Renderer::Renderer(Scene::Handler& handler)
    : Cdlod::Renderer::Base(handler),
      pGraphicsWork(nullptr),
      surfaceInfo(),
      settings_(),
      dbgHeightmap_(),
      pPerQuadTreeItem_(nullptr),
      instMgr_("Instance Cdlod Ocean Manager Data", 128) {}

void Renderer::init() {
    const auto& ctx = handler().shell().context();

    instMgr_.init(ctx);

    useDebugCamera_ = true;

    // Surface info
    surfaceInfo = {};
    surfaceInfo.l = 1.0f;
    surfaceInfo.A = 2e-6f;
    surfaceInfo.Lx = surfaceInfo.Lz = 500.0f;
    // surfaceInfo.l = 0.001f;
    // surfaceInfo.A = 2e-6f;
    // surfaceInfo.Lx = surfaceInfo.Lz = 320.0f;
    // surfaceInfo.V = 12.8f;
    // surfaceInfo.omega = {0, 1};

    // Make the ocean resources owned by the texture handler.
    BufferView::Ocean::MakeResources(handler().textureHandler(), surfaceInfo);
    Texture::Ocean::MakeResources(handler().textureHandler(), surfaceInfo);

    // UNIFORM
    // NOTE: Do this here until there is a proper ocean place. This is because I want the SurfaceInfo to not
    // be a global for god knows what reason.
    UniformDynamic::Ocean::SimulationDispatch::CreateInfo simDpchInfo = {};
    simDpchInfo.info = surfaceInfo;
    handler().uniformHandler().ocnSimDpchMgr().insert(ctx.dev, &simDpchInfo);
    auto& pSimDpch = handler().uniformHandler().ocnSimDpchMgr().pItems.back();

    // MATERIAL
    Material::Default::CreateInfo matInfo = {};
    matInfo.flags |= Material::FLAG::MODE_FLAT_SHADE;
    matInfo.color = COLOR_BLUE;
    matInfo.shininess = 10;
    // matInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_NIGHT_ID);
    auto& pMaterial = handler().materialHandler().makeMaterial(&matInfo);

    // TESSELLATION
    UniformDynamic::Tessellation::Phong::DATA data = {};
    data.data0[0] = 2.0f;  // maxLevel
    data.data0[2] = 3.0f;  // innerLevel
    data.data0[3] = 2.0f;  // outerLevel
    handler().uniformHandler().tessPhongMgr().insert(ctx.dev, true, {data});
    auto& pTess = handler().uniformHandler().tessPhongMgr().pItems.back();

    {  // GRAPHICS WORK
        GraphicsWork::OceanCreateInfo ocnInfo = {};
        ocnInfo.name = "Ocean Surface Graphics Work";
        ocnInfo.surfaceInfo = surfaceInfo;
        ocnInfo.pipelineTypes = {
            GRAPHICS::OCEAN_WF_DEFERRED,
            GRAPHICS::OCEAN_SURFACE_DEFERRED,
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
            GRAPHICS::OCEAN_WF_TESS_DEFERRED,
            GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED,
#endif
        };
        ocnInfo.pDynDescs.push_back(pMaterial.get());
        ocnInfo.pDynDescs.push_back(pTess.get());

        pGraphicsWork = std::make_unique<GraphicsWork::OceanSurface>(handler().passHandler(), &ocnInfo, this);
        pGraphicsWork->onInit();
    }

    {  // CDLOD
        settings_.LeafQuadTreeNodeSize = 8;
        settings_.RenderGridResolutionMult = 4;  // 8;
        settings_.LODLevelCount = 8;
        settings_.MinViewRange = 35000.0f;
        settings_.MaxViewRange = 100000.0f;
        settings_.LODLevelDistanceRatio = 2.0f;

        // Initialize the quad tree uniform data.
        pPerQuadTreeItem_ = handler().uniformHandler().cdlodQdTrMgr().insert(ctx.dev, true);
    }
}

void Renderer::reset() {
    // TODO: This is not the typical pattern, and it doesn't work right. I don't care atm.
    // pGraphicsWork->destroy();
    pGraphicsWork.reset(nullptr);

    settings_ = {};
    dbgHeightmap_ = {};
    pPerQuadTreeItem_ = nullptr;
    instMgr_.destroy(handler().shell().context());
}

void Renderer::tick() {
    // assert(false);
    pGraphicsWork->onTick();
}

bool Renderer::shouldDraw(const PIPELINE type) const {
    // assert(false);
    return pGraphicsWork->shouldDraw(type);
}

void Renderer::record(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const vk::CommandBuffer& cmd) {
    // assert(false);
    pGraphicsWork->record(passType, pPipelineBindData, cmd);
}

std::shared_ptr<Instance::Cdlod::Ocean::Base>& Renderer::makeInstance(Instance::Cdlod::Ocean::CreateInfo* pInfo) {
    const auto& ctx = handler().shell().context();
    auto& pInstance = pInfo->pSharedData;
    if (pInstance == nullptr) {
        if (pInfo == nullptr || pInfo->data.empty())
            instMgr_.insert(ctx.dev, true);
        else
            instMgr_.insert(ctx.dev, pInfo->update, pInfo->data);
        pInstance = instMgr_.pItems.back();
    }
    return pInstance;
}

void Renderer::bindDescSetData(const vk::CommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                               const int lodLevel) const {
    assert(false);
    // const auto frameIndex = handler().passHandler().renderPassMgr().getFrameIndex();
    // const auto it = std::find(pBuffer_->GRAPHICS_TYPES.begin(), pBuffer_->GRAPHICS_TYPES.end(),
    //                          std::visit(Pipeline::GetGraphics{}, pPipelineBindData->type));
    // const auto i = std::distance(pBuffer_->GRAPHICS_TYPES.begin(), it);
    // pBuffer_->draw(RENDER_PASS::DEFERRED, pPipelineBindData,
    //               pBuffer_->getDescriptorSetBindData(RENDER_PASS::DEFERRED,
    //               pBuffer_->getGraphicsDescSetBindDataMaps()[i]), cmd, frameIndex);
}

void Renderer::setGlobalShaderSettings() {
    // The aspect of the texture is dealt with by the material. Material::SetDefaultTextureData() has an example of a
    // way to deal with non-square textures if this renderer needs to implement something similar.
    assert(false);
    pPerQuadTreeItem_->getData().data1.x = 0.5;
    pPerQuadTreeItem_->dirty = true;
    handler().uniformHandler().update(pPerQuadTreeItem_);
}

CDLODRenderer::PerQuadTreeData& Renderer::getPerQuadTreeData() {
    return handler().uniformHandler().cdlodQdTrMgr().getTypedItem(0).getData();
}

}  // namespace Ocean