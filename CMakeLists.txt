# The name of our project is "GuppyVulkan". CMakeLists files in this project can
# refer to the root source directory of the project as ${GUPPY_SOURCE_DIR} and
# to the root binary directory of the project as ${GUPPY_BINARY_DIR}.
if (APPLE)
    cmake_minimum_required(VERSION 3.13.4)
    set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
else()
    cmake_minimum_required(VERSION 2.8.11)
endif()

project (GuppyVulkan)
# set (CMAKE_VERBOSE_MAKEFILE 1)

# The API_NAME allows renaming builds to avoid conflicts with installed SDKs
# The MAJOR number of the version we're building, used in naming
# <api-name>-<major>.dll (and other files).
set(API_NAME "Vulkan" CACHE STRING "API name to use when building")
set(MAJOR "1")
string(TOLOWER ${API_NAME} API_LOWERCASE)

set(GLM_LIB_DIR "GLM-NOTFOUND" CACHE PATH "Absolute path to a GLM library directory")
set(GLFW_INCLUDE_DIR "GLFW-INCLUDE-NOTFOUND" CACHE PATH "Absolute path to a GLFW include directory")
set(GLFW_LIB "GLFW-LIB-NOTFOUND" CACHE PATH "Absolute path to a GLFW library directory")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};${VULKAN_HEADERS_INSTALL_DIR};${VULKAN_LOADER_INSTALL_DIR};${GLM_INSTALL_DIR};
                       $ENV{VULKAN_HEADERS_INSTALL_DIR};$ENV{VULKAN_LOADER_INSTALL_DIR};$ENV{GLM_INSTALL_DIR})

set(EXT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(APPLE)
    message(STATUS "Using find_package to locate Python3")
    find_package(PythonInterp 3 REQUIRED)
endif()
message(STATUS "Using find_package to locate Vulkan")
find_package(Vulkan)
message(STATUS "Vulkan FOUND = ${Vulkan_FOUND}")
message(STATUS "Vulkan Include = ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan Lib = ${Vulkan_LIBRARY}")

option(USE_CCACHE "Use ccache" OFF)
if (USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)
endif()

include(GNUInstallDirs)
# Set a better default install location for Windows only if the user did not provide one.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE)
endif()

# Enable cmake folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FALLBACK_CONFIG_DIRS "/etc/xdg" CACHE STRING
        "Search path to use when XDG_CONFIG_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant.")
    set(FALLBACK_DATA_DIRS "/usr/local/share:/usr/share" CACHE STRING
        "Search path to use when XDG_DATA_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant.")

    include(FindPkgConfig)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)
    option(BUILD_WSI_MIR_SUPPORT "Build Mir WSI support" OFF)
    set(DEMOS_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for demos (XCB, XLIB, WAYLAND, MIR, DISPLAY)")
    set(SAMPLES_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for api-samples (XCB, WAYLAND, DISPLAY)")

    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
    endif()

    if (BUILD_WSI_XLIB_SUPPORT)
        find_package(X11 REQUIRED)
    endif()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        find_package(Wayland REQUIRED)
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
    endif()

    if (BUILD_WSI_MIR_SUPPORT)
        find_package(Mir REQUIRED)
    endif()
endif()

if(WIN32)
    # Treat warnings as errors
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/WX>")
    # Disable RTTI
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/GR->")
    # Warn about nested declarations
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34456>")
    # Warn about potentially uninitialized variables
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34701>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34703>")
    # Warn about different indirection types.
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34057>")
    # Warn about signed/unsigned mismatch.
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w34245>")
endif()

if(NOT WIN32 AND NOT APPLE)
    set (BUILDTGT_DIR build)
    set (BINDATA_DIR Bin)
    set (LIBSOURCE_DIR Lib)
else()
    # is WIN32
    option(DISABLE_BUILD_PATH_DECORATION "Disable the decoration of the gslang and SPIRV-Tools build path with MSVC build type info" OFF)
    option(DISABLE_BUILDTGT_DIR_DECORATION "Disable the decoration of the gslang and SPIRV-Tools build path with target info" OFF)

    # For Windows, since 32-bit and 64-bit items can co-exist, we build each in its own build directory.
    # 32-bit target data goes in build32, and 64-bit target data goes into build.  So, include/link the
    # appropriate data at build time.
    if (DISABLE_BUILDTGT_DIR_DECORATION)
        set (BUILDTGT_DIR "")
        set (BINDATA_DIR "")
        set (LIBSOURCE_DIR "")
    elseif (CMAKE_CL_64)
        set (BUILDTGT_DIR build)
        set (BINDATA_DIR Bin)
        set (LIBSOURCE_DIR Lib)
    else()
        set (BUILDTGT_DIR build32)
        set (BINDATA_DIR Bin32)
        set (LIBSOURCE_DIR Lib32)
    endif()
endif()

add_definitions(-DAPI_NAME="${API_NAME}")

if (WIN32)
    if(DISABLE_BUILD_PATH_DECORATION)
        set (DEBUG_DECORATION "")
        set (RELEASE_DECORATION "")
    else()
        set (DEBUG_DECORATION "Debug")
        set (RELEASE_DECORATION "Release")
    endif()
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")

    # For GCC version 7.1 or greater, we need to disable the implicit fallthrough warning since
    # there's no consistent way to satisfy all compilers until they all accept the C++17 standard
    if (CMAKE_COMPILER_IS_GNUCC AND NOT (CMAKE_CXX_COMPILER_VERSION LESS 7.1))
        set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -Wimplicit-fallthrough=0")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS} -std=c++11 -fno-rtti")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

############# GLM
set(GLM_LIB_DIR "GLM-NOTFOUND" CACHE PATH "Absolute path to a GLM install directory")
if(NOT GLM_LIB_DIR AND NOT DEFINED ENV{GLM_LIB_DIR})
    message(FATAL_ERROR "Must define location of glm -- see BUILD.md")
endif()
# Cmake command line option overrides environment variable
if(NOT GLM_LIB_DIR)
    set(GLM_LIB_DIR $ENV{GLM_LIB_DIR})
endif()
message(STATUS "Using GLM library located at ${GLM_LIB_DIR}")

############# GLFW INCLUDE
set(GLFW_INCLUDE_DIR "GLFW-INCLUDE-NOTFOUND" CACHE PATH "Absolute path to a GLFW include directory")
if(NOT GLFW_INCLUDE_DIR AND NOT DEFINED ENV{GLFW_INCLUDE_DIR})
    message(FATAL_ERROR "Must define location of glfw include directory -- see BUILD.md")
endif()
# Cmake command line option overrides environment variable
if(NOT GLFW_INCLUDE_DIR)
    set(GLFW_INCLUDE_DIR $ENV{GLFW_INCLUDE_DIR})
endif()
message(STATUS "Using GLFW include directory located at ${GLFW_INCLUDE_DIR}")

############# GLFW LIB
set(GLFW_LIB "GLFW-LIB-NOTFOUND" CACHE PATH "Absolute path to a GLFW library file")
if(NOT GLFW_LIB AND NOT DEFINED ENV{GLFW_LIB})
    message(FATAL_ERROR "Must define location of glfw library file -- see BUILD.md")
endif()
# Cmake command line option overrides environment variable
if(NOT GLFW_LIB)
    set(GLFW_LIB $ENV{GLFW_LIB})
endif()
message(STATUS "Using GLFW library located at ${GLFW_LIB}")
set(GLFW_LIBRARIES ${GLFW_LIB})

############# FMOD
include(FindFMOD)

function(make_static_imported_library LIB_NAME LIB_DIR LIB_LIST)
    add_library(${LIB_NAME} STATIC IMPORTED)
    set_target_properties(${LIB_NAME} PROPERTIES
        IMPORTED_LOCATION ${LIB_DIR}/${LIB_NAME}.lib
        IMPORTED_LOCATION_DEBUG ${LIB_DIR}/${LIB_NAME}d.lib)
    list(APPEND ${LIB_LIST} ${LIB_NAME})
    set(${LIB_LIST} ${${LIB_LIST}} PARENT_SCOPE)
endfunction()

if(NOT APPLE)
    ############# GLSLANG
    set(GLSLANG_INSTALL_DIR "GLSLANG-NOTFOUND" CACHE PATH "Absolute path to a glslang install directory")
    if(NOT GLSLANG_INSTALL_DIR AND NOT DEFINED ENV{GLSLANG_INSTALL_DIR})
        message(FATAL_ERROR "Must define location of glslang binaries -- see BUILD.md")
    endif()
    # Cmake command line option overrides environment variable
    if(NOT GLSLANG_INSTALL_DIR)
        set(GLSLANG_INSTALL_DIR $ENV{GLSLANG_INSTALL_DIR})
    endif()
    message(STATUS "Using glslang install directory located at ${GLSLANG_INSTALL_DIR}")

    make_static_imported_library(GenericCodeGen     ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(glslang            ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(HLSL               ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(MachineIndependent ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(OGLCompiler        ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(OSDependent        ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(SPIRV-Tools        ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(SPIRV-Tools-opt    ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(SPIRV              ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)
    make_static_imported_library(SPVRemapper        ${GLSLANG_INSTALL_DIR}/lib GLSLANG_LIBRARIES)

    # Set the include directory here like this to avoid using the headers in the Vulkan SDK. I could not get
    # a debug build to work with the .lib's in that come with SDK so I still need the dependency on the
    # glslang github repo.
    target_include_directories(glslang INTERFACE ${GLSLANG_INSTALL_DIR}/include/glslang)
else()
    ############# MOLTEN VK
    set(MVK_PACKAGE_DIR "MVK-PACKAGE-NOTFOUND" CACHE PATH "Absolute path to a MoltenVK Package directory")
    if(NOT MVK_PACKAGE_DIR AND NOT DEFINED ENV{MVK_PACKAGE_DIR})
        message(FATAL_ERROR "Must define location of MoltenVK Package directory -- see BUILD.md")
    endif()
    # Cmake command line option overrides environment variable
    if(NOT MVK_PACKAGE_DIR)
        set(MVK_PACKAGE_DIR $ENV{MVK_PACKAGE_DIR})
    endif()
endif()

set(PYTHON_CMD ${PYTHON_EXECUTABLE})

set (SHARED_NAME shared)
set (UTILS_NAME vsamputils)

get_filename_component(GLMINC_PREFIX "${GLM_LIB_DIR}" ABSOLUTE)
if(NOT EXISTS ${GLMINC_PREFIX})
    message(FATAL_ERROR "Necessary glm headers do not exist: " ${GLMINC_PREFIX})
endif()

add_definitions(-DGUPPY_BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# Custom targets
add_subdirectory(Common)
add_subdirectory(shaders)
add_subdirectory(libs)
add_subdirectory(CDLOD)

# TODO: make this optional
include(MakeImGuiLibrary)

add_subdirectory(Guppy)

