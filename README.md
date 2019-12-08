# Guppy (Vulkan)
  This repository is test ground for a basic game engine using Vulkan C++. The repository was based off a sample in the [LunarG VulkanSamples repository](https://github.com/LunarG/VulkanSamples), specifically the [Hologram sample](https://github.com/LunarG/VulkanSamples/tree/master/Sample-Programs/Hologram). Very little from the sample remains except for some cross platform code, and CMake build files.

  The main focus of the code herein has been on the rendering side, but some other tiny features have been added along the way as well. There is a list of things you can find inside the repo below. 

## How to Build and Run
[BUILD.md](BUILD.md) has directions for building and running the code.

## Features
* Windows build
  * Native & [GLFW](https://www.glfw.org/) window/input support
  * Directory listener for recompiling/loading shaders on file change at runtime.
* macOS build ([MoltenVK](https://github.com/KhronosGroup/MoltenVK))
  * [GLFW](https://www.glfw.org/) window/input support
  * Directory listener for recompiling/loading loading shaders on file change at runtime.
* [Dear ImGui](https://github.com/ocornut/imgui) integration
* [FMOD](https://www.fmod.com/) integration
* [XInput](https://docs.microsoft.com/en-us/windows/win32/xinput/xinput-game-controller-apis-portal) integration
* [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) for image loading
* [tinyobjloader](https://github.com/syoyo/tinyobjloader) for mesh loading with material support
* Basic CPU selection
* Other things I'm forgetting...

## Rendering
Recently in my experimenting with Vulkan pipelines/passes/shaders I decided to switch from forward shading to deferred. Below you can see a list of things I have implemented and which pipeline they were developed for. Ideally I would have enough time to make shaders for parity between all the pipelines, but there is only so much time in the day (ray tracing pipelines are coming soon hopefully!). I have tried to start making demo videos for some of the more interesting pipelines/shaders. More to come soon...

> **Note: The demo videos were taken on a MacBook Pro running Bootcamp Windows drivers using a Radeon Pro 560X at 1920x1080 in debug mode. Optimization has not been the goal of this project. The focus has been learning the math, and gaining a thorough understanding of the Vulkan API.**

* Forward shading
  * Multi-sampling and resolve attachments
  * PBR (Physically based rendering)
  * Fog
  * Positional lights
  * Parallax texture mapping
  * Normal (bump) mapping
  * Offscreen render to texture
    * Subsequent pass (same frame) use projective texture mapping onto geometry in the main scene.
    * Screen space post-processing (compute)
      * Gaussian blurring (glow)
      * HDR w/ bloom (doesn't work well)
* Deferred shading
  * Multi-sampling for output attachment passes, and resolve attachment for combine/screenspace pass with "by region" dependency.
  * Tessellation
    * Lines
  * Particles (compute)
    * Fountains
      * Recyclable with parameterized position/acceleration/rotation (euler integration).
        * Billboarded quads
        * Meshes  [![video]({https://i9.ytimg.com/vi/57DorBKAc1Q/mq2.jpg?sqp=CP3Rse8F&rs=AOn4CLDXIeQ9RV6h9qcm5v-IwzsW-bKZ9g})]({https://youtu.be/57DorBKAc1Q} "Guppy - recyclable particle fountain meshes with shadows")
    * Attractors
      * Recyclable parameterized position/acceleration/rotation (euler integration).
    * Cloth
    * Height field fluids
  * Shadows
    * ~~One direction with PCF softening~~ removed in favor of below. Hopefully have a better solution soon.
    * Point lights that generate a depth cube map array (geometry) [video](https://youtu.be/ri3ZodRF7VY)
* Both
  * Materials
  * Instanced drawing
  * Triple buffering
  * Blinn Phong
  * Directional lights
  * Spot lights

## Contact Information
* [Colin Hughes](mailto:colin.s.hughes@gmail.com)