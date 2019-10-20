cmake_minimum_required(VERSION 2.8.11)

IF(NOT IMGUI_REPO_DIR)
  SET(IMGUI_REPO_DIR $ENV{IMGUI_REPO_DIR})
ENDIF()

# MESSAGE(STATUS "ENV{IMGUI_REPO_DIR}: $ENV{IMGUI_REPO_DIR}")
# MESSAGE(STATUS "IMGUI_REPO_DIR: ${IMGUI_REPO_DIR}")

IF(NOT EXISTS ${IMGUI_REPO_DIR})
  MESSAGE(FATAL_ERROR "Could not find required ImGui repository location: ${IMGUI_REPO_DIR}")
ENDIF()
SET(IMGUI_INCLUDE_DIRS ${IMGUI_REPO_DIR})

IF(NOT EXISTS ${IMGUI_REPO_DIR}/examples)
  MESSAGE(FATAL_ERROR "Could not find required ImGui repository location: ${IMGUI_REPO_DIR}/examples")
ENDIF()
LIST(APPEND IMGUI_INCLUDE_DIRS ${IMGUI_REPO_DIR}/examples)

# MESSAGE(STATUS "IMGUI_INCLUDE_DIRS: ${IMGUI_INCLUDE_DIRS}")

SET(IMGUI_FILE_NAMES
  imconfig.h
  imgui.cpp
  imgui.h
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_internal.h
  imgui_widgets.cpp
  imstb_rectpack.h
  imstb_textedit.h
  imstb_truetype.h
  examples/imgui_impl_glfw.cpp
  examples/imgui_impl_glfw.h
  examples/imgui_impl_vulkan.cpp
  examples/imgui_impl_vulkan.h
)

# MESSAGE(STATUS "IMGUI_FILE_NAMES: ${IMGUI_FILE_NAMES}")

SET(IMGUI_HEADERS_AND_SOURCE "")

FOREACH(IMGUI_FILE_NAME ${IMGUI_FILE_NAMES})
  # MESSAGE(STATUS "IMGUI_FILE_NAME: ${IMGUI_FILE_NAME}")
  IF (NOT EXISTS ${IMGUI_REPO_DIR}/${IMGUI_FILE_NAME})
    MESSAGE(FATAL_ERROR "Could not find required ImGui file: ${IMGUI_REPO_DIR}/${IMGUI_FILE_NAME}")
  ENDIF()
  LIST(APPEND IMGUI_HEADERS_AND_SOURCE ${IMGUI_REPO_DIR}/${IMGUI_FILE_NAME})
ENDFOREACH(IMGUI_FILE_NAME)

# MESSAGE(STATUS "IMGUI_HEADERS_AND_SOURCE: ${IMGUI_HEADERS_AND_SOURCE}")
MESSAGE(STATUS "Using ImGui repository located at ${IMGUI_REPO_DIR}")

SET(IMGUI_LIB ImGui)

ADD_LIBRARY(${IMGUI_LIB} STATIC ${IMGUI_HEADERS_AND_SOURCE})

# External libraries
INCLUDE_DIRECTORIES(${IMGUI_LIB}
    ${Vulkan_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(${IMGUI_LIB}
    ${GLFW_LIBRARIES}
    # ${VULKAN_LOADER}
)