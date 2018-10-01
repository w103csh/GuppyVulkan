
set(SOURCE
    Box.cpp
    Box.h
    Camera.cpp
    Camera.h
    CMakeLists.txt
    Constants.h
    EventHandlers.h
    Extensions.h
    FileLoader.cpp
    FileLoader.h
    Game.h
    Guppy.cpp
    Guppy.h
    Helpers.h
    InputHandler.cpp
    InputHandler.h
    Main.cpp
    Model.cpp
    Model.h
    MyShell.cpp
    MyShell.h
    Plane.cpp
    Plane.h
    Shell.cpp
    Shell.h
    StagingBufferHandler.cpp
    StagingBufferHandler.h
    Texture.cpp
    Texture.h
    Vertex.cpp
    Vertex.h
    )

# shader folder
file(GLOB SHADERS 
    shaders/*.vert # - vertex shader
    shaders/*.tesc # - tessellation control shader
    shaders/*.tese # - tessellation evaluation shader
    shaders/*.geom # - geometry shader
    shaders/*.frag # - fragment shader
    shaders/*.comp # - compute shader
    shaders/compile.bat
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
    ${TARGET_NAME}
    ${GLSLANG_LIBRARIES}
    ${VULKAN_LOADER}
    ${SPIRV_TOOLS_LIBRARIES}
    )

# message(STATUS ${SHADERS})


if(WIN32)
    list(APPEND definitions PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
    list(APPEND definitions PRIVATE -DWIN32_LEAN_AND_MEAN)

    list(APPEND SOURCE ShellWin32.cpp ShellWin32.h)
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
        ${SHADERS}
        )

    target_link_libraries(${TARGET_NAME} ${LINKS})

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
        ${SHADERS}
        )

    source_group("Shaders" FILES ${SHADERS})
    source_group("Utils" FILES ${UTILS})
    # source_group("Utils/Source Files" FILES ${UTILS})
    # source_group("Utils/Headers" FILES ${UTILS})

    # external libraries
    include_directories(${TARGET_NAME}
        utils
        # C:/libs/glfw-3.2.1.bin.WIN64/include
        ${GLM_LIB_DIR}
        C:/libs/stb
        C:/libs/tinyobjloader
        )

    target_link_libraries(${LINKS})

endif()

