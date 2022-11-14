cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# CMP0074: find_package() uses <PackageName>_ROOT variables
cmake_policy(SET CMP0074 NEW)

project(NPDet
  VERSION 1.2.0
  LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set the C++ standard to be used")
if(NOT CMAKE_CXX_STANDARD MATCHES "17|20|23")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type configuration" FORCE)
  message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif()

# Error on all warnings
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  #FIXME not ready for this...
  #add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Export compile commands as json for run-clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_DATA_MODEL "build the dd4pod datamodel" ON)

set(USE_GEOCAD ON CACHE BOOL "build the geocad library. Requires opencascade")

# ---------------------------------------------------------------------------
# set default install prefix, build type, C++ standard
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT FALSE)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "..." FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "..." FORCE)
endif()

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "...." FORCE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# load additional OS dependent settings
include(cmake/os.cmake)
include(cmake/root.cmake)

set(PODIO "$ENV{PODIO}/lib/podio/cmake")
set(CMAKE_MODULE_PATH CMAKE_MODULE_PATH PODIO)


#----------------------------------------------------------------------------
# Find Libraries
#----------------------------------------------------------------------------
# PODIO
find_package(podio 0.14.1 REQUIRED )
include_directories(${podio_INCLUDE_DIR})

# DD4hep
find_package( DD4hep 1.18 REQUIRED COMPONENTS DDCore DDG4 )
include(${DD4hep_DIR}/cmake/DD4hep.cmake)
include(${DD4hep_DIR}/cmake/DD4hepBuild.cmake)
dd4hep_configure_output()
dd4hep_set_compiler_flags()

# Opencascade
if(USE_GEOCAD)
  find_package(OpenCASCADE)
endif()

# libfmt
find_package(fmt REQUIRED)

#-----------------------------------------
# catch2 unit testing
enable_testing()
Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.1)
FetchContent_MakeAvailable(Catch2)
#target_link_libraries(tests Catch2::Catch2)

#-----------------------------------------
# add the library sub directories
add_subdirectory(src/plugins)
add_subdirectory(src/dd4pod)
add_subdirectory(src/geocad)
add_subdirectory(src/config)
add_subdirectory(src/tools)

#----------------------------------------------------------------------------
# install examples
#set(NPDet_EXAMPLES
#    )
#install(FILES ${NPDet_EXAMPLES}
#    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/examples"
#    )

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
