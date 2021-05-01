cmake_minimum_required(VERSION 2.8.11)

set(SOURCE
    Camera.cpp
    Camera.h
    Cloth.cpp
    Cloth.h
    CommandHandler.cpp
    CommandHandler.h
    CMakeLists.txt
    Constants.cpp
    Constants.h
    ConstantsAll.h
    GraphicsWork.cpp
    GraphicsWork.h
    Enum.h
    EventHandlers.h
    Face.cpp
    Face.h
    FileLoader.cpp
    FileLoader.h
    FFT.cpp
    FFT.h
    Game.cpp
    Game.h
    Geometry.cpp
    Geometry.h
    Guppy.cpp
    Guppy.h
    Handlee.h
    HeightFieldFluid.cpp
    HeightFieldFluid.h
    InputHandler.cpp
    InputHandler.h
    Light.cpp
    Light.h
    LoadingHandler.cpp
    LoadingHandler.h
    Main.cpp
    Material.cpp
    Material.h
    MaterialHandler.cpp
    MaterialHandler.h
    Parallax.cpp
    Parallax.h
    PassConstants.h
    PassHandler.cpp
    PassHandler.h
    PBR.cpp
    PBR.h
    Random.h
    RenderPassCubeMap.cpp
    RenderPassCubeMap.h
    Scene.cpp
    Scene.h
    SceneHandler.cpp
    SceneHandler.h
    SelectionManager.cpp
    SelectionManager.h
    Singleton.h
    SoundHandler.cpp
    SoundHandler.h
    SoundFModHandler.cpp
    SoundFmodHandler.h
    Stars.cpp
    Stars.h
    Storage.cpp
    Storage.h
    StorageConstants.h
    Tessellation.cpp
    Tessellation.h
    UIHandler.cpp
    UIHandler.h
    Vertex.cpp
    Vertex.h
    # Buffer
    BufferItem.h
    BufferManager.h
    # Cdlod
    Cdlod.cpp
    Cdlod.h
    CdlodRenderer.cpp
    CdlodRenderer.h
    # Compute
    Compute.cpp
    Compute.h
    ComputeWork.cpp
    ComputeWork.h
    ComputeWorkConstants.h
    ComputeWorkManager.cpp
    ComputeWorkManager.h
    # Deferred
    Deferred.cpp
    Deferred.h
    RenderPassDeferred.cpp
    RenderPassDeferred.h
    # Descriptor
    Descriptor.cpp
    Descriptor.h
    DescriptorManager.h
    DescriptorConstants.cpp
    DescriptorConstants.h
    DescriptorHandler.cpp
    DescriptorHandler.h
    DescriptorSet.cpp
    DescriptorSet.h
    # ImGui
    UIImGuiHandler.cpp
    UIImGuiHandler.h
    RenderPassImGui.cpp
    RenderPassImGui.h
    # Instance
    Instance.cpp
    Instance.h
    InstanceManager.h
    # Mesh
    Arc.cpp
    Arc.h
    Axes.h
    Axes.cpp
    Box.cpp
    Box.h
    FaceMesh.cpp
    FaceMesh.h
    Mesh.cpp
    Mesh.h
    MeshConstants.cpp
    MeshConstants.h
    MeshHandler.cpp
    MeshHandler.h
    Plane.cpp
    Plane.h
    Torus.cpp
    Torus.h
    VisualHelper.cpp
    VisualHelper.h
    # Model
    Model.cpp
    Model.h
    ModelConstants.cpp
    ModelConstants.h
    ModelHandler.cpp
    ModelHandler.h
    ModelMesh.cpp
    ModelMesh.h
    # Object 3d
    Obj3d.cpp
    Obj3d.h
    Obj3dDrawInst.h
    Obj3dInst.h
    # Ocean
    Ocean.cpp
    Ocean.h
    OceanComputeWork.cpp
    OceanComputeWork.h
    OceanRenderer.cpp
    OceanRenderer.h
    # Particle
    Particle.cpp
    Particle.h
    ParticleBuffer.cpp
    ParticleBuffer.h
    ParticleConstants.h
    ParticleHandler.cpp
    ParticleHandler.h
    # Pipeline
    Pipeline.cpp
    Pipeline.h
    PipelineConstants.cpp
    PipelineConstants.h
    PipelineHandler.cpp
    PipelineHandler.h
    # Render Pass
    RenderPass.cpp
    RenderPass.h
    RenderPassConstants.cpp
    RenderPassConstants.h
    RenderPassManager.cpp
    RenderPassManager.h
    # Screen Space
    ScreenSpace.cpp
    ScreenSpace.h
    RenderPassScreenSpace.cpp
    RenderPassScreenSpace.h
    # Shader
    ShaderConstants.cpp
    ShaderConstants.h
    ShaderHandler.cpp
    ShaderHandler.h
    # Shadow
    RenderPassShadow.cpp
    RenderPassShadow.h
    Shadow.cpp
    Shadow.h
    # Shell
    Shell.cpp
    Shell.h
    ShellGLFW.cpp
    ShellGLFW.h
    # Texture
    Sampler.cpp
    Sampler.h
    SamplerConstants.cpp
    SamplerConstants.h
    Texture.cpp
    Texture.h
    TextureConstants.cpp
    TextureConstants.h
    TextureHandler.cpp
    TextureHandler.h
    # Uniform
    Uniform.cpp
    Uniform.h
    UniformConstants.cpp
    UniformConstants.h
    UniformOffsetsManager.cpp
    UniformOffsetsManager.h
    UniformHandler.cpp
    UniformHandler.h
    )

# utils folder
file(GLOB UTILS
    utils/*.cpp
    utils/*.h
    utils/*.hpp
    )

# # utils folder (mimice directory structure?)
# file(GLOB UTILS_SOURCE
#     utils/*.cpp
#     )
# file(GLOB UTILS_HEADERS
#     utils/*.h
#     utils/*.hpp
#     )

# target links
set(LINKS
    ${GLFW_LIBRARIES}
    ${IMGUI_LIB}
    ${CDLOD_LIB}
    ${COMMON_LIB}
    )

# target includes
set(INCLUDES
    utils
    ${GLM_LIB_DIR}
    ${EXT_LIB_DIR}
    ${GLFW_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIRS}
    )

# message(STATUS "INCLUDES: ${INCLUDES}")
# message(STATUS ${CDLOD_LIB})

if(FMOD_FOUND)
    list(APPEND LINKS ${FMOD_LIBRARIES})
    list(APPEND INCLUDES ${FMOD_INCLUDE_DIRS})
endif()

# message(STATUS "LINKS: ${LINKS}")
# message(STATUS "INCLUDES: ${INCLUDES}")

if(WIN32)
    list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
    list(APPEND definitions PRIVATE -DWIN32_LEAN_AND_MEAN)

    list(APPEND SOURCE ShellWin32.cpp ShellWin32.h)

    list(APPEND LINKS
        ${GLSLANG_LIBRARIES}
        ${Vulkan_LIBRARY}
        XInput
        )

    IF(TRUE) # Not sure if a flag is needed yet
        add_definitions(-DUSE_XINPUT)
        list(APPEND SOURCE XInputHelper.h)
        list(APPEND LINKS XInput)
    ENDIF()

elseif(APPLE)
    list(APPEND SOURCE ShellMac.cpp ShellMac.h)

    list(APPEND LINKS
        ${Vulkan_LIBRARY}
        # CoreServices: directory listener
        "-framework CoreServices"
        ${MVK_PACKAGE_DIR}/Latest/MoltenVKShaderConverter/MoltenVKGLSLToSPIRVConverter/macOS/dynamic/libMoltenVKGLSLToSPIRVConverter.dylib
        )

    list(APPEND INCLUDES
        ${MVK_PACKAGE_DIR}/Latest/MoltenVKShaderConverter/include
        ${MVK_PACKAGE_DIR}/Latest/MoltenVKShaderConverter/MoltenVKGLSLToSPIRVConverter/macOS/dynamic
        )

else()
    list(APPEND libraries PRIVATE -ldl)

    if(BUILD_WSI_XCB_SUPPORT AND DEMOS_WSI_SELECTION STREQUAL "XCB")
        find_package(XCB REQUIRED)

        list(APPEND SOURCE ShellXcb.cpp ShellXcb.h)
        list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_XCB_KHR)
        list(APPEND includes PRIVATE ${XCB_INCLUDES})
        list(APPEND libraries PRIVATE ${XCB_LIBRARIES})
    elseif(BUILD_WSI_WAYLAND_SUPPORT AND DEMOS_WSI_SELECTION STREQUAL "WAYLAND")
        find_package(Wayland REQUIRED)

        list(APPEND SOURCE ShellWayland.cpp ShellWayland.h)
        list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_WAYLAND_KHR)
        list(APPEND includes PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIR})
        list(APPEND libraries PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    endif()

endif()

if(UNIX)
    if(APPLE)

        # apple specific links
        list(APPEND ${LINKS}
            #something
            )

        add_executable(${TARGET_NAME}
            ${SOURCE}
            ${UTILS}
            )

        source_group("Utils" FILES ${UTILS})
        # source_group("Utils/Source Files" FILES ${UTILS})
        # source_group("Utils/Headers" FILES ${UTILS})

        include_directories(${TARGET_NAME} ${INCLUDES})

        # We do this so vulkaninfo is linked to an individual library and NOT a framework.
        target_link_libraries(${TARGET_NAME} ${LINKS})

    else()
        # unix specific links
        list(APPEND ${LINKS}
            ${XCB_LIBRARIES}
            ${WAYLAND_CLIENT_LIBRARIES}
            ${PTHREAD}
            )

        add_executable(${TARGET_NAME}
            main.cpp
            ${SOURCE}
            ${UTILS}
            )

        target_link_libraries(${TARGET_NAME} ${LINKS})
    endif()
else()

    # windows specific links
    list(APPEND ${LINKS}
        ${WINLIBS}
        )

    # message(STATUS ${LINKS})

    add_executable(${TARGET_NAME} WIN32
        main.cpp
        ${SOURCE}
        ${UTILS}
        # ${UTILS_SOURCE}
        # ${UTILS_HEADERS}
        )

    # This is needed for dynamic casting
    # target_compile_options(${TARGET_NAME} PRIVATE "/GR")

    source_group("Utils" FILES ${UTILS})
    # source_group("Utils/Source Files" FILES ${UTILS})
    # source_group("Utils/Headers" FILES ${UTILS})

    # external libraries
    include_directories(${TARGET_NAME} ${INCLUDES})

    target_link_libraries(${TARGET_NAME} ${LINKS})

    if(FMOD_FOUND)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FMOD_LIBRARY_DIR}/fmod${FMOD_SUFFIX}.dll      # file(s) to copy
            "$(OutDir)"                                     # destination
            )
    endif()

endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    )

