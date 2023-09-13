cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(npsim
  VERSION 1.0.0
  LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set the C++ standard to be used")
if(NOT CMAKE_CXX_STANDARD MATCHES "17|20|23")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Install prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT FALSE)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Install prefix" FORCE)
  message(STATUS "Setting default install prefix: ${CMAKE_INSTALL_PREFIX}")
endif()

# Use GNU install dirs
include(GNUInstallDirs)

# Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type configuration" FORCE)
  message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif()

# Error on all warnings
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Export compile commands as json for run-clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Options

set(USE_GEOCAD ON CACHE BOOL "build the geocad library. Requires opencascade")

# ---------------------------------------------------------------------------
# Load additional OS dependent settings

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  ## OSX/homebrew version of root6 installs its cmake macros in a non-standard
  ## location. This might be an issue on other systems as well.
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROOTSYS}/etc/root/cmake)

  ## Get rid of rpath warning on OSX
  set(CMAKE_MACOSX_RPATH 1)
endif()

#----------------------------------------------------------------------------
# Find Libraries
#----------------------------------------------------------------------------

# DD4hep
find_package(DD4hep 1.18 REQUIRED COMPONENTS DDCore DDG4)
include(${DD4hep_DIR}/cmake/DD4hep.cmake)
include(${DD4hep_DIR}/cmake/DD4hepBuild.cmake)

dd4hep_configure_output()
dd4hep_set_compiler_flags()

# Opencascade
if(USE_GEOCAD)
  find_package(OpenCASCADE REQUIRED)
endif()

# libfmt
find_package(fmt REQUIRED)

# eAST
find_package(east)
message(STATUS "Found eAST: ${east_DIR}")

#-----------------------------------------
# add the library sub directories
add_subdirectory(src/plugins)
add_subdirectory(src/dd4pod)
add_subdirectory(src/geocad)
add_subdirectory(src/config)
add_subdirectory(src/tools)

if(TARGET east::eASTPhysicsListBase)
  add_subdirectory(src/east)
endif()

#----------------------------------------------------------------------------
# Install and export targets
install(EXPORT NPDetTargets
    FILE NPDetTargets.cmake
    NAMESPACE NPDet::
    DESTINATION lib/NPDet
    )

include(CMakePackageConfigHelpers)

set(TARGETS_INSTALL_PATH lib/NPDet/NPDetTargets.cmake)
CONFIGURE_PACKAGE_CONFIG_FILE(
    cmake/NPDetConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/NPDetConfig.cmake
    INSTALL_DESTINATION lib/NPDet
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR TARGETS_INSTALL_PATH
    )

write_basic_package_version_file("NPDetConfigVersion.cmake"
    VERSION ${NPDet_VERSION}
    COMPATIBILITY SameMajorVersion
    )

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/NPDetConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/NPDetConfigVersion.cmake
    DESTINATION lib/NPDet
    )

install(PROGRAMS
  scripts/sanitize_hepmc3.py DESTINATION bin RENAME sanitize_hepmc3
    )
