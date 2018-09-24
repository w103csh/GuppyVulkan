
#pragma once

#include <glm/glm.hpp>
#include <map>
#include <util.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "Vertex.h"

constexpr auto APP_SHORT_NAME = "Guppy Application";
constexpr auto DEPTH_PRESENT = true;

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation",
};

const std::vector<const char*> INSTANCE_EXTENSIONS = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME, in init_swapchain_extension now
};

const VkClearColorValue CLEAR_VALUE = {};  // { 1.0f, 1.0f, 1.0f, 1.0f };

// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
typedef uint32_t VB_INDEX_TYPE;

// Application wide up vector
static glm::vec3 UP_VECTOR = glm::vec3(0.0f, 1.0f, 0.0f);

const auto ENABLE_SAMPLE_SHADING = VK_TRUE;
const float MIN_SAMPLE_SHADING = 0.2f;