/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef COMPUTE_WORK_CONSTANTS_H
#define COMPUTE_WORK_CONSTANTS_H

#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <Common/Types.h>

#include "Enum.h"

namespace ComputeWork {

using index = uint32_t;

constexpr index BAD_OFFSET = UINT32_MAX;

extern const std::set<COMPUTE_WORK> ALL;
extern const std::vector<COMPUTE_WORK> ACTIVE;

struct SubmitResource {
    std::vector<vk::Semaphore> waitSemaphores;
    vk::PipelineStageFlags waitDstStageMask;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Semaphore> signalSemaphores;
    vk::Fence fence;
};

}  // namespace ComputeWork

#endif  // !COMPUTE_WORK_CONSTANTS_H