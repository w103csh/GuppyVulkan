#pragma once

//#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan.h>
//#include "InputHandler.h"

// static void FramebufferSize_callback(GLFWwindow* window, int width, int height)
//{
//    auto app = reinterpret_cast<Guppy*>(glfwGetWindowUserPointer(window));
//    app->m_framebufferResized = true;
//}
//
// static void Key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    auto app = reinterpret_cast<Guppy*>(glfwGetWindowUserPointer(window));
//    InputHandler::get().handleKeyInput(key, action);
//}

// Debug callback for validation layer
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    // example: see https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
    }

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}