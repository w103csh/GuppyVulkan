
#include "RenderPassScreenSpace.h"

#include "Sampler.h"
#include "ScreenSpace.h"
// HANDLERS
#include "CommandHandler.h"     ///////////////////////////////
#include "DescriptorHandler.h"  ///////////////////////////////
#include "PipelineHandler.h"    ///////////////////////////////
#include "RenderPassHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace ScreenSpace {

// CREATE INFO

const CreateInfo CREATE_INFO = {
    PASS::SCREEN_SPACE,
    "Screen Space Swapchain Render Pass",
    {
        PIPELINE::SCREEN_SPACE_DEFAULT,
    },
    FLAG::SWAPCHAIN,
    {},
    {
        PASS::SCREEN_SPACE_BRIGHT,
        PASS::SCREEN_SPACE_BLUR_A,
        PASS::SCREEN_SPACE_BLUR_B,
    },
};

const CreateInfo SAMPLER_CREATE_INFO = {
    // THIS WAS BROKE LAST TIME I TRIED IT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    PASS::SAMPLER_SCREEN_SPACE,
    "Screen Space Sampler Render Pass",
    {
        PIPELINE::SCREEN_SPACE_DEFAULT,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::DEFAULT_2D_TEXTURE_ID)},
};

// BASE

Base::Base(RenderPass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo)
    : RenderPass::Base{handler, std::forward<const index>(offset), pCreateInfo} {
    status_ = STATUS::PENDING_MESH | STATUS::PENDING_PIPELINE;
}

// TODO: should something like this be on the base class????
void Base::init() {
    // Initialize the dependent pass types. Currently they all will come prior to the
    // main loop pass.
    for (const auto& [passType, offset] : dependentTypeOffsetPairs_) {
        // Boy is this going to be confusing if it ever doesn't work right.
        if (passType == TYPE) {
            RenderPass::Base::init();
        } else {
            const auto& pPass = handler().getPass(offset);
            if (!pPass->isIntialized()) const_cast<RenderPass::Base*>(pPass.get())->init();
        }
    }
}

void Base::record(const uint8_t frameIndex) {
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(priCmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    // TODO: use VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT since this will only update after
    // swapchain recreation.
    vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        assert(dependentTypeOffsetPairs_.size() == 4);

        // BRIGHT
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[0].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pc = {::ScreenSpace::BLOOM_BRIGHT};
                const auto& pPipelineBindData = pPass->getPipelineBindDataMap().begin()->second;
                vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                   sizeof(::ScreenSpace::PushConstant), &pc);

                handler().getScreenQuad()->draw(TYPE, pPass->getPipelineBindDataMap().begin()->second,
                                                pPass->getDescSetBindDataMap().begin()->second, priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        // BLUR A
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[1].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pc = {::ScreenSpace::BLOOM_BLUR_A};
                const auto& pPipelineBindData = pPass->getPipelineBindDataMap().begin()->second;
                vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                   sizeof(::ScreenSpace::PushConstant), &pc);

                handler().getScreenQuad()->draw(TYPE, pPipelineBindData, pPass->getDescSetBindDataMap().begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        // BLUR B
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[2].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pc = {::ScreenSpace::BLOOM_BLUR_B};
                const auto& pPipelineBindData = pPass->getPipelineBindDataMap().begin()->second;
                vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                   sizeof(::ScreenSpace::PushConstant), &pc);

                handler().getScreenQuad()->draw(TYPE, pPipelineBindData, pPass->getDescSetBindDataMap().begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        // SCREEN SPACE DEFAULT
        {
            beginPass(priCmd, frameIndex);

            auto& pPipelineBindData = pipelineTypeBindDataMap_.begin()->second;
            ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::PASS_FLAG::BLOOM};
            vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                               static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

            handler().getScreenQuad()->draw(TYPE, pPipelineBindData, descSetBindDataMap_.begin()->second, priCmd,
                                            frameIndex);

            endPass(priCmd);
        }
    }

    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void Base::update() {
    // Check the mesh status.
    if (handler().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;

        assert(pipelineTypeBindDataMap_.size() == 1);  // Not dealing with anything else atm.
        // Get pipeline bind data, and check the status
        auto it = pipelineTypeBindDataMap_.begin();
        auto& pPipeline = handler().pipelineHandler().getPipeline(it->first);

        // If the pipeline is not ready try to update once.
        if (pPipeline->getStatus() != STATUS::READY) pPipeline->updateStatus();

        if (pPipeline->getStatus() == STATUS::READY) {
            // Get or make descriptor bind data.
            if (descSetBindDataMap_.empty())
                handler().descriptorHandler().getBindData(it->first, descSetBindDataMap_, nullptr, nullptr);

            assert(descSetBindDataMap_.size() == 1);  // Not dealing with anything else atm.

            status_ ^= STATUS::PENDING_PIPELINE;
            assert(status_ == STATUS::READY);
        }
    }
}

// BRIGHT
const CreateInfo BRIGHT_CREATE_INFO = {
    PASS::SCREEN_SPACE_BRIGHT,
    "Screen Space Bright Render Pass",
    {
        PIPELINE::SCREEN_SPACE_BRIGHT,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID)},
};
Bright::Bright(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BRIGHT_CREATE_INFO} {}

// BLUR A
const CreateInfo BLUR_A_CREATE_INFO = {
    PASS::SCREEN_SPACE_BLUR_A,
    "Screen Space Blur A Render Pass",
    {
        PIPELINE::SCREEN_SPACE_BLUR_A,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_B_2D_TEXTURE_ID)},
};
BlurA::BlurA(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BLUR_A_CREATE_INFO} {}

// BLUR B
const CreateInfo BLUR_B_CREATE_INFO = {
    PASS::SCREEN_SPACE_BLUR_B,
    "Screen Space Blur B Render Pass",
    {
        PIPELINE::SCREEN_SPACE_BLUR_B,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID)},
};
BlurB::BlurB(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BLUR_B_CREATE_INFO} {}

// HDR LOG

const CreateInfo HDR_LOG_CREATE_INFO = {
    PASS::SCREEN_SPACE_HDR_LOG,
    "Screen Space HDR Log Render Pass",
    {
        PIPELINE::SCREEN_SPACE_HDR_LOG,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::HDR_LOG_2D_TEXTURE_ID)},
};

HdrLog::HdrLog(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::Base{handler, std::forward<const index>(offset), &HDR_LOG_CREATE_INFO},
      hasTrasfCmd_(false),
      wasSubmitted_(false),
      firstThing_(-1),
      transfWaitDstStgMask_(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT),
      blitBCurrentLayout_(VK_IMAGE_LAYOUT_UNDEFINED) {}

void HdrLog::init() {
    RenderPass::Base::init();

    // prepareDownSample(handler().shell().context().imageCount);

    //// THERE ONLY NEEDS TO BE ONE OF THESE RIGHT
    ///////?????????????????????????????????????????????????????????????????????????????????????????????????????????????

    //// Default render plane create info.
    // Mesh::CreateInfo meshInfo = {};
    // meshInfo.pipelineType = PIPELINE::SCREEN_SPACE_HDR_LOG;
    // meshInfo.selectable = false;
    // meshInfo.geometryCreateInfo.transform = helpers::affine(glm::vec3{2.0f});

    // Instance::Default::CreateInfo defInstInfo = {};
    // Material::Default::CreateInfo defMatInfo = {};
    // defMatInfo.flags = 0;

    //// Create the default render plane.
    // hdrLogRenderPlaneIndex_ =
    //    handler().meshHandler().makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo)->getOffset();
    // assert(hdrLogRenderPlaneIndex_ != Mesh::BAD_OFFSET);

    // finalLayout_ = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}  // namespace ScreenSpace

void HdrLog::record(const uint8_t frameIndex) {
    // beginInfo_.framebuffer = data.framebuffers[frameIndex];
    // auto& priCmd = data.priCmds[frameIndex];

    //// RESET BUFFERS
    // vkResetCommandBuffer(priCmd, 0);

    //// BEGIN BUFFERS
    // VkCommandBufferBeginInfo bufferInfo;
    //// PRIMARY
    // bufferInfo = {};
    // bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    // vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    //// const auto& blitSamplerA =
    ////    handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID,
    ////    frameIndex)->samplers[0];
    //// const auto& blitSamplerB =
    ////    handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_B_2D_TEXTURE_ID,
    ////    frameIndex)->samplers[0];

    //// if (true) {
    ////    {
    ////        barrierResource_.reset();
    ////        auto& resource = barrierResource_;
    ////        resource.imgBarriers.push_back({});
    ////        resource.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ////        resource.imgBarriers.back().pNext = nullptr;
    ////        resource.imgBarriers.back().srcAccessMask = 0;
    ////        resource.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    ////        resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ////        resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ////        resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ////        resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ////        resource.imgBarriers.back().image = blitSamplerA.image;
    ////        resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    ////        helpers::recordBarriers(barrierResource_, priCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    ////                                VK_PIPELINE_STAGE_TRANSFER_BIT);
    ////    }

    ////    {
    ////        barrierResource_.reset();
    ////        auto& resource = barrierResource_;
    ////        resource.imgBarriers.push_back({});
    ////        resource.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ////        resource.imgBarriers.back().pNext = nullptr;
    ////        resource.imgBarriers.back().srcAccessMask = 0;
    ////        resource.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ////        resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    ////        resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ////        resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ////        resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ////        resource.imgBarriers.back().image = blitSamplerB.image;
    ////        resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    ////        helpers::recordBarriers(barrierResource_, priCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    ////                                VK_PIPELINE_STAGE_TRANSFER_BIT);
    ////    }
    ////}

    //// PASS #1

    // beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    // auto& pRenderPlane = handler().meshHandler().getTextureMesh(hdrLogRenderPlaneIndex_);
    // if (false || pRenderPlane->getStatus() == STATUS::READY) {
    //    assert(pipelineTypeBindDataMap_.size());
    //    const auto& pipelineBindData = pipelineTypeBindDataMap_.begin()->second;

    //    // DRAW #1

    //    pRenderPlane->draw(TYPE, pipelineBindData, priCmd, frameIndex);

    //    vkResetCommandBuffer(transfCmds_[frameIndex], 0);
    //    VkCommandBufferBeginInfo bufferInfo2 = {};
    //    bufferInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //    bufferInfo2.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    //    vk::assert_success(vkBeginCommandBuffer(transfCmds_[frameIndex], &bufferInfo2));
    //    // downSample(transfCmds_[frameIndex], frameIndex);
    //    downSample2(transfCmds_[frameIndex], frameIndex);
    //}

    // endPass(frameIndex);

    // THE BLIT PROMISED LAND THAT IS NOT! AN OASIS
    {
        // const auto& renderSampler = pTextures_[frameIndex]->samplers[0];
        // downSample(priCmd, frameIndex);

        // VkImageBlit blit = {};
        //// source
        // blit.srcOffsets[0] = {0, 0, 0};
        // blit.srcOffsets[1] = {
        //    static_cast<int32_t>(renderSampler.extent.width),
        //    static_cast<int32_t>(renderSampler.extent.height),
        //    1,
        //};
        // blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // blit.srcSubresource.mipLevel = 0;
        // blit.srcSubresource.baseArrayLayer = 0;
        // blit.srcSubresource.layerCount = 1;
        //// destination
        // blit.dstOffsets[0] = {0, 0, 0};
        // blit.dstOffsets[1] = {
        //    // 1,  // static_cast<int32_t>(blitSampler.extent.width),
        //    // 1,  // static_cast<int32_t>(blitSampler.extent.height),
        //    // 1,
        //    static_cast<int32_t>(renderSampler.extent.width / 2),
        //    static_cast<int32_t>(renderSampler.extent.height / 2),
        //    1,
        //};
        // blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // blit.dstSubresource.mipLevel = 0;
        // blit.dstSubresource.baseArrayLayer = 0;
        // blit.dstSubresource.layerCount = 1;

        // vkCmdBlitImage(                            //
        //    priCmd,                                //
        //    renderSampler.image,                   //
        //    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  //
        //    blitSamplerA.image,                    //
        //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  //
        //    1,                                     //
        //    &blit,                                 //
        //    VK_FILTER_LINEAR                       //
        //);

        //{
        //    barrierResource_.reset();
        //    auto& resource = barrierResource_;
        //    resource.imgBarriers.push_back({});
        //    resource.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        //    resource.imgBarriers.back().pNext = nullptr;
        //    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        //    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        //    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //    resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        //    resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        //    resource.imgBarriers.back().image = blitSamplerA.image;
        //    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        //    helpers::recordBarriers(barrierResource_, priCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
        //                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        //}

        //{
        //    barrierResource_.reset();
        //    auto& resource = barrierResource_;
        //    resource.imgBarriers.push_back({});
        //    resource.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        //    resource.imgBarriers.back().pNext = nullptr;
        //    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        //    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        //    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        //    resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        //    resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        //    resource.imgBarriers.back().image = blitSamplerB.image;
        //    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        //    helpers::recordBarriers(barrierResource_, priCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
        //                            VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    //// PASS #2

    // beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    //// DRAW #2

    // pRenderPlane->draw(TYPE, pipelineBindData, priCmd, frameIndex);

    // endPass(frameIndex);
    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void HdrLog::prepareDownSample(const uint32_t imageCount) {
    assert(imageCount == 3);  // Potential imageCount problem
    // SUBMIT
    transfSubmitResources_.resize(imageCount);
    // FENCE
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    transfFences_.resize(imageCount);
    for (auto& fence : transfFences_)
        vk::assert_success(vkCreateFence(handler().shell().context().dev, &fenceInfo, nullptr, &fence));
    // COMMAND
    transfCmds_.resize(imageCount);
    handler().commandHandler().createCmdBuffers(handler().commandHandler().graphicsCmdPool(), transfCmds_.data(),
                                                VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(transfCmds_.size()));
    // SEMAPHORE
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (transfSemaphores_.empty()) transfSemaphores_.resize(imageCount);
    for (auto& semaphore : transfSemaphores_)
        vk::assert_success(vkCreateSemaphore(handler().shell().context().dev, &createInfo, nullptr, &semaphore));
}

void shaderToTransDstAll(const VkCommandBuffer& cmd, const Sampler::Base& sampler, BarrierResource& res) {
    res.reset();
    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = 0;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = sampler.image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, sampler.mipmapInfo.mipLevels, 0,
                                               sampler.arrayLayers};
    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}
void shaderToTransDst(const VkCommandBuffer& cmd, const VkImage& image, BarrierResource& res) {
    res.reset();
    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = 0;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}
void shaderToTransSrc(const VkCommandBuffer& cmd, const VkImage& image, BarrierResource& res) {
    res.reset();
    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = 0;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}
void transDstToTransSrc(const VkCommandBuffer& cmd, const VkImage& image, BarrierResource& res) {
    res.reset();
    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}
void transSrcToTransDst(const VkCommandBuffer& cmd, const VkImage& image, BarrierResource& res) {
    res.reset();
    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}
void transDstToShader(const VkCommandBuffer& cmd, const VkImage& image, BarrierResource& res) {
    res.reset();
    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    res.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void HdrLog::downSample(const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    uint8_t blitIndex = (frameIndex + 1) % handler().shell().context().imageCount;

    const auto& extent = handler().shell().context().extent;
    int32_t x = static_cast<int32_t>(extent.width), y = static_cast<int32_t>(extent.height);

    uint32_t iterations = static_cast<int32_t>(std::floor(std::log2(std::max(x, y))));

    // RENDER TARGET
    const auto& renderSampler = pTextures_[frameIndex]->samplers[0];
    // BLIT A
    // const auto& blitSamplerA =
    //    handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID,
    //    frameIndex)->samplers[0];
    const auto& blitSamplerA =
        handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID, blitIndex)->samplers[0];
    // BLIT B
    // const auto& blitSamplerB =
    //    handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_B_2D_TEXTURE_ID,
    //    frameIndex)->samplers[0];
    const auto& blitSamplerB =
        handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_B_2D_TEXTURE_ID, blitIndex)->samplers[0];

    VkImage src = renderSampler.image;
    VkImage dst = ((iterations) % 2 == 0) ? blitSamplerB.image : blitSamplerA.image;

    std::vector<std::pair<int, int>> resos;
    int count = 0;
    while (true) {
        // TRANSFER
        if (count == 0) {
            // A
            shaderToTransDst(cmd, blitSamplerA.image, barrierResource_);
            // B
            if (iterations > 1) {
                if (blitBCurrentLayout_ == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                    transSrcToTransDst(cmd, blitSamplerB.image, barrierResource_);
                else if (blitBCurrentLayout_ != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                    shaderToTransDst(cmd, blitSamplerB.image, barrierResource_);
                blitBCurrentLayout_ = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
        } else {
            transDstToTransSrc(cmd, src, barrierResource_);
            if (count > 1) transSrcToTransDst(cmd, dst, barrierResource_);
            blitBCurrentLayout_ = (blitBCurrentLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                                      ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                                      : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }

        // Blit
        VkImageBlit blit = {};
        // source
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {
            x,
            y,
            1,
        };

        x /= 2;
        y /= 2;

        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {
            x,
            y,
            1,
        };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        resos.push_back({x, y});

        // barrierResource_.reset();
        // barrierResource_.glblBarriers.push_back({});
        // barrierResource_.glblBarriers.back().sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        // barrierResource_.glblBarriers.back().pNext = nullptr;
        // barrierResource_.glblBarriers.back().srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT |
        // VK_ACCESS_TRANSFER_WRITE_BIT; barrierResource_.glblBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT
        // | VK_ACCESS_TRANSFER_WRITE_BIT; barrierResource_.glblBarriers.back().srcAccessMask =
        //    VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        //    VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
        //    VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        //    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        //    VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT |
        //    VK_ACCESS_HOST_WRITE_BIT;
        // barrierResource_.glblBarriers.back().dstAccessMask =
        //    VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        //    VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
        //    VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        //    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        //    VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT |
        //    VK_ACCESS_HOST_WRITE_BIT;
        // helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
        // VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkCmdBlitImage(                            //
            cmd,                                   //
            src,                                   //
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  //
            dst,                                   //
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  //
            1,                                     //
            &blit,                                 //
            VK_FILTER_LINEAR                       //
        );

        // if (count == 1) break;
        if (x == 1 && y == 1) {
            assert(dst == blitSamplerA.image);
            break;
        } else {
            if (count == 0) {
                if (src == blitSamplerA.image) {
                    src = blitSamplerB.image;
                } else {
                    src = blitSamplerA.image;
                }
            }
            std::swap(src, dst);
        }
        count++;
    }

    transDstToShader(cmd, dst, barrierResource_);
    // vk::assert_success(vkEndCommandBuffer(cmd));

    transfSubmitResources_[frameIndex].commandBufferCount = 1;
    transfSubmitResources_[frameIndex].commandBuffers[0] = cmd;
    transfSubmitResources_[frameIndex].waitSemaphoreCount = 0;
    // transfSubmitResources_[frameIndex].waitSemaphoreCount = 1;
    // transfSubmitResources_[frameIndex].waitSemaphores[0] = data.semaphores[frameIndex];
    // transfSubmitResources_[frameIndex].waitDstStageMasks[0] = transfWaitDstStgMask_;
    transfSubmitResources_[frameIndex].signalSemaphoreCount = 1;
    transfSubmitResources_[frameIndex].signalSemaphores[0] = transfSemaphores_[blitIndex];
    hasTrasfCmd_ = true;
}

void HdrLog::downSample2(const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    uint8_t blitIndex = (frameIndex + 1) % handler().shell().context().imageCount;
    // RENDER TARGET
    const auto& renderSampler = pTextures_[frameIndex]->samplers[0];
    // BLIT A
    const auto& blitSamplerA =
        handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID, blitIndex)->samplers[0];

    shaderToTransDstAll(cmd, blitSamplerA, barrierResource_);

    int32_t mipWidth = renderSampler.extent.width;
    int32_t mipHeight = renderSampler.extent.height;

    VkImageBlit blit = {};
    // source
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = blitSamplerA.arrayLayers;

    mipWidth /= 2;
    mipHeight /= 2;

    // destination
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth, mipHeight, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = blitSamplerA.arrayLayers;

    vkCmdBlitImage(cmd, renderSampler.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, blitSamplerA.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    //

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = blitSamplerA.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = blitSamplerA.arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    for (uint32_t i = 1; i < blitSamplerA.mipmapInfo.mipLevels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &barrier);

        blit = {};
        // source
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = blitSamplerA.arrayLayers;

        mipWidth /= 2;
        mipHeight /= 2;

        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth, mipHeight, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = blitSamplerA.arrayLayers;

        vkCmdBlitImage(cmd, blitSamplerA.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, blitSamplerA.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = blitSamplerA.mipmapInfo.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    transfSubmitResources_[frameIndex].commandBufferCount = 1;
    transfSubmitResources_[frameIndex].commandBuffers[0] = cmd;
    transfSubmitResources_[frameIndex].waitSemaphoreCount = 0;
    // transfSubmitResources_[frameIndex].waitSemaphoreCount = 1;
    // transfSubmitResources_[frameIndex].waitSemaphores[0] = data.semaphores[frameIndex];
    // transfSubmitResources_[frameIndex].waitDstStageMasks[0] = transfWaitDstStgMask_;
    transfSubmitResources_[frameIndex].signalSemaphoreCount = 1;
    transfSubmitResources_[frameIndex].signalSemaphores[0] = transfSemaphores_[blitIndex];
    hasTrasfCmd_ = true;
}

bool HdrLog::submitDownSample(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource) {
    if (hasTrasfCmd_) {
        std::memcpy(&submitResource, &transfSubmitResources_[frameIndex], sizeof(RenderPass::SubmitResource));
        firstThing_ = (firstThing_ == -1) ? static_cast<int8_t>(frameIndex) : firstThing_;
        wasSubmitted_ = true;
        return true;
    }
    return false;
}

void HdrLog::getSemaphore(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource) {
    if (wasSubmitted_) {
        submitResource.waitSemaphores[submitResource.waitSemaphoreCount] = transfSemaphores_[frameIndex];
        submitResource.waitDstStageMasks[submitResource.waitSemaphoreCount++] = transfWaitDstStgMask_;
    }
}

void HdrLog::read(const uint8_t frameIndex) {
    if (frameIndex == firstThing_) {
        uint8_t blitIndex = (frameIndex + 1) % handler().shell().context().imageCount;
        // BLIT A
        const auto& blitSamplerA = handler()
                                       .textureHandler()
                                       .getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID, blitIndex)
                                       ->samplers[0];

        const auto& dev = handler().shell().context().dev;

        void* data;
        vk::assert_success(vkMapMemory(dev, blitSamplerA.memory, 0, VK_WHOLE_SIZE, 0, &data));

        float min = FLT_MAX, max = FLT_MIN, current = 0.0, total = 0.0, average = 0.0, count = 0.0;

        for (uint32_t x = 0; x < blitSamplerA.extent.width; x++) {
            for (uint32_t y = 0; y < blitSamplerA.extent.height; y++) {
                std::memcpy(&current, &data, sizeof(float));
                min = (std::min)(current, min);
                max = (std::max)(current, max);
                total += current;
                count++;
            }
        }

        average = total / count;

        vkUnmapMemory(dev, blitSamplerA.memory);
    }
}

}  // namespace ScreenSpace

}  // namespace RenderPass
