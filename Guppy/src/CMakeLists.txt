cmake_minimum_required(VERSION 2.8.11)

set(SOURCE
    Camera.cpp
    Camera.h
    CommandHandler.cpp
    CommandHandler.h
    CMakeLists.txt
    Constants.cpp
    Constants.h
    ConstantsAll.h
    Enum.h
    EventHandlers.h
    Extensions.h
    Face.cpp
    Face.h
    FileLoader.cpp
    FileLoader.h
    Game.cpp
    Game.h
    Geometry.h
    Guppy.cpp
    Guppy.h
    Handlee.h
    Helpers.cpp
    Helpers.h
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
    Obj3d.cpp
    Obj3d.h
    Parallax.cpp
    Parallax.h
    PBR.cpp
    PBR.h
    Scene.cpp
    Scene.h
    SceneHandler.cpp
    SceneHandler.h
    SelectionManager.cpp
    SelectionManager.h
    Singleton.h
    Storage.cpp
    Storage.h
    Types.h
    UIHandler.cpp
    UIHandler.h
    Vertex.cpp
    Vertex.h
    # Buffer
    BufferItem.h
    BufferManager.h
    # Compute
    Compute.cpp
    Compute.h
    ComputeHandler.cpp
    ComputeHandler.h
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
    ObjDrawInst3d.h
    ObjInst3d.cpp
    ObjInst3d.h
    # Mesh
    Axes.h
    Axes.cpp
    Box.cpp
    Box.h
    FaceMesh.cpp
    FaceMesh.h
    Mesh.cpp
    Mesh.h
    MeshHandler.cpp
    MeshHandler.h
    Plane.cpp
    Plane.h
    VisualHelper.cpp
    VisualHelper.h
    # Model
    Model.cpp
    Model.h
    ModelHandler.cpp
    ModelHandler.h
    ModelMesh.cpp
    ModelMesh.h
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
    RenderPassHandler.cpp
    RenderPassHandler.h
    # Screen Space
    ScreenSpace.cpp
    ScreenSpace.h
    RenderPassScreenSpace.cpp
    RenderPassScreenSpace.h
    # Shader
    Shader.cpp
    Shader.h
    ShaderConstants.cpp
    ShaderConstants.h
    ShaderHandler.cpp
    ShaderHandler.h
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

# message(STATUS ${IMGUI})

# target links
set(LINKS
    ${GLFW_LIBRARIES}
    ${IMGUI}
    )
    
# target includes
set(INCLUDES
    utils
    ${GLM_LIB_DIR}
    ${EXT_LIB_DIR}
    ${GLFW_INCLUDE_DIR}
    ${IMGUI_DIR}
    )

# message(STATUS "LINKS: ${LINKS}")
# message(STATUS "INCLUDES: ${INCLUDES}")

if(WIN32)
    list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
    list(APPEND definitions PRIVATE -DWIN32_LEAN_AND_MEAN)

    list(APPEND SOURCE ShellWin32.cpp ShellWin32.h)

    list(APPEND LINKS
        ${GLSLANG_LIBRARIES}
        ${VULKAN_LOADER}
        ${SPIRV_TOOLS_LIBRARIES}
        )

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
            Main.cpp
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

endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    )

    