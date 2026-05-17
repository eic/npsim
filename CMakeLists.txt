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
include(CheckCXXCompilerFlag)

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
option(USE_ASAN "Compile with address sanitizer" OFF)
option(USE_TSAN "Compile with thread sanitizer" OFF)
option(USE_UBSAN "Compile with undefined behavior sanitizer" OFF)

if(USE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O1)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
  # Clang does not link the full ASAN runtime into shared/module libs by default;
  # it only adds bootstrap stubs that require the runtime from the main binary.
  # Since DD4hep binaries are not ASAN-instrumented, link the shared ASAN DSO directly.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -shared-libasan")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared-libasan")
    # Clang's ASAN DSO has an architecture-specific name; use it for LD_PRELOAD
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libclang_rt.asan-${CMAKE_SYSTEM_PROCESSOR}.so
      OUTPUT_VARIABLE PRELOAD_SANITIZER_LIB_ASAN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libasan.so
      OUTPUT_VARIABLE PRELOAD_SANITIZER_LIB_ASAN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  list(PREPEND PRELOAD_SANITIZER_LIB ${PRELOAD_SANITIZER_LIB_ASAN})
endif()

if(USE_TSAN)
  add_compile_options(-fsanitize=thread -g -O1)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=thread")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -shared-libsan")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared-libsan")
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libclang_rt.tsan-${CMAKE_SYSTEM_PROCESSOR}.so
      OUTPUT_VARIABLE PRELOAD_SANITIZER_LIB_TSAN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libtsan.so
      OUTPUT_VARIABLE PRELOAD_SANITIZER_LIB_TSAN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  list(PREPEND PRELOAD_SANITIZER_LIB ${PRELOAD_SANITIZER_LIB_TSAN})
endif()

if(USE_UBSAN)
  foreach(sanitizer undefined float-divide-by-zero unsigned-integer-overflow implicit-conversion local-bounds nullability)
    check_cxx_compiler_flag("-fsanitize=${sanitizer}" CXX_COMPILER_HAS_sanitize_${sanitizer})
    if(CXX_COMPILER_HAS_sanitize_${sanitizer})
      add_compile_options(-fsanitize=${sanitizer})
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${sanitizer}")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=${sanitizer}")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=${sanitizer}")
    endif()
  endforeach()
  add_compile_options(-g -O1)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -shared-libsan")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared-libsan")
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libclang_rt.ubsan_standalone-${CMAKE_SYSTEM_PROCESSOR}.so
      OUTPUT_VARIABLE PRELOAD_SANITIZER_LIB_UBSAN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libubsan.so
      OUTPUT_VARIABLE PRELOAD_SANITIZER_LIB_UBSAN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  list(PREPEND PRELOAD_SANITIZER_LIB ${PRELOAD_SANITIZER_LIB_UBSAN})
endif()

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

function(dd4hep_generate_rootmap library)
  if(APPLE)
    set(ENV_VAR DYLD_LIBRARY_PATH)
  else()
    set(ENV_VAR LD_LIBRARY_PATH)
  endif()
  set(${ENV_VAR}_VALUE $<TARGET_FILE_DIR:${library}>:$<TARGET_FILE_DIR:DD4hep::DD4hepGaudiPluginMgr>:$ENV{${ENV_VAR}})

  set(rootmapfile ${CMAKE_SHARED_MODULE_PREFIX}${library}.components)
  string(JOIN ":" LD_PRELOAD ${PRELOAD_SANITIZER_LIB})

  add_custom_command(OUTPUT ${rootmapfile}
    DEPENDS ${library}
    COMMAND ${ENV_VAR}=${${ENV_VAR}_VALUE} LD_PRELOAD=${LD_PRELOAD} $<TARGET_FILE:DD4hep::listcomponents> -o ${rootmapfile} $<TARGET_FILE_NAME:${library}>
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
    COMMAND_EXPAND_LISTS
  )

  add_custom_target(Components_${library} ALL DEPENDS ${rootmapfile})
  set(install_destination "lib")
  if(CMAKE_INSTALL_LIBDIR)
    set(install_destination ${CMAKE_INSTALL_LIBDIR})
  endif()
  install(FILES $<TARGET_FILE_DIR:${library}>/${rootmapfile}
    DESTINATION ${install_destination}
  )
endfunction()

dd4hep_configure_output()
dd4hep_set_compiler_flags()

# Opencascade
if(USE_GEOCAD)
  find_package(OpenCASCADE REQUIRED)
endif()

# libfmt
find_package(fmt REQUIRED)

#-----------------------------------------
# add the library sub directories
add_subdirectory(src/plugins)
add_subdirectory(src/dd4pod)
add_subdirectory(src/geocad)
add_subdirectory(src/config)
add_subdirectory(src/tools)

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
