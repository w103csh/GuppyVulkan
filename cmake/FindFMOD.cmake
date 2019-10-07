
IF(NOT FMOD_DIR)
  SET(FMOD_DIR $ENV{FMOD_DIR})
ENDIF()

# MESSAGE(STATUS "ENV{FMOD_DIR}: $ENV{FMOD_DIR}")

IF(TRUE)
  # The "L" appears to control message logging. I like the log messages so I'll
  # leave it for now.
  SET(FMOD_SUFFIX L)
ENDIF()
# MESSAGE(STATUS "FMOD_SUFFIX: ${FMOD_SUFFIX}")

# MESSAGE(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
IF(WIN32)
  IF (${CMAKE_GENERATOR_PLATFORM} STREQUAL Win32)
    SET(FMOD_ARCH_DIR x86)
  ELSEIF(${CMAKE_GENERATOR_PLATFORM} STREQUAL x64)
    SET(FMOD_ARCH_DIR x64)
  ENDIF()
ENDIF()
# MESSAGE(STATUS "FMOD_ARCH_DIR: ${FMOD_ARCH_DIR}")

FIND_PATH(FMOD_INCLUDE_DIR fmod.h
  HINTS
  ${FMOD_DIR}
  PATH_SUFFIXES
    api/core/inc
)
# MESSAGE(STATUS "FMOD_INCLUDE_DIR: ${FMOD_INCLUDE_DIR}")

FIND_PATH(FMOD_LIBRARY_DIR fmod${FMOD_SUFFIX}.dll
  HINTS
  ${FMOD_DIR}
  PATH_SUFFIXES
    api/core/lib/${FMOD_ARCH_DIR}
)
# MESSAGE(STATUS "FMOD_LIBRARY_DIR: ${FMOD_LIBRARY_DIR}")

IF (WIN32)
  SET(FMOD_LIB_NAME fmod${FMOD_SUFFIX}_vc)
  SET(FMOD_PACKAGE_ARGS FMOD_INCLUDE_DIR FMOD_LIBRARY_DIR)
ELSEIF(APPLE)
  SET(FMOD_LIB_NAME libfmod${FMOD_SUFFIX}.dylib)
  SET(FMOD_PACKAGE_ARGS FMOD_INCLUDE_DIR)
ENDIF()
# MESSAGE(STATUS "FMOD_LIB_NAME: ${FMOD_LIB_NAME}")

FIND_LIBRARY(FMOD_LIBRARY ${FMOD_LIB_NAME}
  HINTS
  ${FMOD_DIR}
  PATH_SUFFIXES
    api/core/lib/${FMOD_ARCH_DIR}
)
# MESSAGE(STATUS "FMOD_LIBRARY: ${FMOD_LIBRARY}")

LIST(APPEND FMOD_PACKAGE_ARGS FMOD_LIBRARY)
# MESSAGE(STATUS "FMOD_PACKAGE_ARGS: ${FMOD_PACKAGE_ARGS}")

# handle the QUIETLY and REQUIRED arguments and set FMOD_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FMOD DEFAULT_MSG
    ${FMOD_PACKAGE_ARGS}
)

IF (FMOD_FOUND)
    MESSAGE(STATUS "Using FMOD library located at ${FMOD_LIBRARY}")
ELSE (FMOD_FOUND)
    MESSAGE(ERROR "FMOD not (entirely) found!")
ENDIF (FMOD_FOUND)

set(FMOD_INCLUDE_DIRS
    ${FMOD_INCLUDE_DIR}
)

macro(_FMOD_APPEND_LIBRARIES _list _release)
   set(_debug ${_release}_DEBUG)
   if(${_debug})
      set(${_list} ${${_list}} optimized ${${_release}} debug ${${_debug}})
   else()
      set(${_list} ${${_list}} ${${_release}})
   endif()
endmacro()

if(FMOD_FOUND)
   _FMOD_APPEND_LIBRARIES(FMOD_LIBRARIES FMOD_LIBRARY)
   add_definitions(-DUSE_FMOD)
endif()