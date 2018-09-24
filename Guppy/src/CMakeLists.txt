
set(NAME Guppy)

file(GLOB SOURCE *.cpp)
file(GLOB HEADERS *.h)

if(UNIX)

    add_executable(${NAME} main.cpp ${SOURCE} ${HEADERS})

    target_link_libraries(
        ${NAME}
        ${UTILS_NAME}
        ${GLSLANG_LIBRARIES}
        ${XCB_LIBRARIES}
        ${WAYLAND_CLIENT_LIBRARIES}
        ${VULKAN_LOADER}
        ${PTHREAD}
        ${SPIRV_TOOLS_LIBRARIES}
        )

else()

    add_executable(${NAME} WIN32 main.cpp ${SOURCE} ${HEADERS})

    target_include_directories(${NAME} PUBLIC
        # C:/libs/glfw-3.2.1.bin.WIN64/include
        C:/libs/stb
        C:/libs/tinyobjloader
        )

    target_link_libraries(
        ${NAME}
        ${UTILS_NAME}
        ${GLSLANG_LIBRARIES}
        ${VULKAN_LOADER}
        ${WINLIBS}
        ${SPIRV_TOOLS_LIBRARIES}
        )

endif()

