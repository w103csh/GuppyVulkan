cmake_minimum_required(VERSION 2.8.11)

set(TARGET_NAME imgui)
set(IMGUI ${TARGET_NAME} PARENT_SCOPE)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
# message(STATUS ${IMGUI_DIR})

file(GLOB IMGUI_SOURCE *.cpp)
file(GLOB IMGUI_HEADERS *.h *.hpp)

add_library(${TARGET_NAME} STATIC ${IMGUI_SOURCE} ${IMGUI_HEADERS})

# add_library(${TARGET_NAME} STATIC ${IMGUI_SOURCE})
# install(TARGETS ${TARGET_NAME} DESTINATION lib)
# install(FILES ${IMGUI_HEADERS} DESTINATION include)

# external libraries
include_directories(${TARGET_NAME}
    ${Vulkan_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    )

target_link_libraries(${TARGET_NAME} ${GLFW_LIBRARIES} ${VULKAN_LOADER})

