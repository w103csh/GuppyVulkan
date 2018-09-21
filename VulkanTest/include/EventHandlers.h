#pragma once

#include <GLFW/glfw3.h>
#include "GuppyApp.h"
#include "InputHandler.h"

static void FramebufferSize_callback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<GuppyApp*>(glfwGetWindowUserPointer(window));
    app->m_framebufferResized = true;
}

static void Key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto app = reinterpret_cast<GuppyApp*>(glfwGetWindowUserPointer(window));
    InputHandler::get().handleKeyInput(key, action);
}



// Unfortunately, because this function is an extension function, it is not automatically loaded. We have to look up
// its address ourselves using vkGetInstanceProcAddr. (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers)
static VkResult CreateDebugUtilsMessengerEXT(
    const VkInstance                                  instance,
    const VkDebugUtilsMessengerCreateInfoEXT*         pCreateInfo,
    const VkAllocationCallbacks*                      pAllocator,
    VkDebugUtilsMessengerEXT*                         pCallback)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    const VkInstance                                  instance,
    const VkDebugUtilsMessengerEXT                    callback,
    const VkAllocationCallbacks*                      pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, callback, pAllocator);
    }
}

// Debug callback for validation layer
static VKAPI_ATTR VkBool32 VKAPI_CALL Debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData)
{

    // example: see https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        // Message is important enough to show
    }

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}