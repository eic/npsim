#----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(EicDet CXX)

# Add library source directory name that appear in ./src
set(EicDet_LIB_NAMES
  simple_example
  JLEIC
  #clas12
  SoLID
  )

# Add executable source directory name that appear in ./src
set(EicDet_EXE_NAMES
  #EicDet
  #runtest
  )

# The version number.
set(EicDet_MAJOR_VERSION 0)
set(EicDet_MINOR_VERSION 1)
set(EicDet_PATCH_VERSION 0)
SET(EicDet_VERSION "${EicDet_MAJOR_VERSION}.${EicDet_MINOR_VERSION}.${EicDet_PATCH_VERSION}")

# ---------------------------------------------------------------------------
# load additional OS dependent settings
include(cmake/os.cmake)

# ---------------------------------------------------------------------------
# Configure the find_package outputs for this library
# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR     lib     CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR     bin     CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
   set(DEF_INSTALL_CMAKE_DIR CMake)
else()
   set(DEF_INSTALL_CMAKE_DIR lib/${PROJECT_NAME})
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

set(CMAKE_MODULE_PATH
   ${CMAKE_MODULE_PATH}
   ${PROJECT_SOURCE_DIR}/cmake
   ${PROJECT_SOURCE_DIR}/cmake/Modules 
   )

if(NOT CMAKE_INSTALL_PREFIX)
   SET(EicDet_PREFIX "/usr/local")
else() 
   SET(EicDet_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

# ---------------------------------------------------------------------------
#
set(EicDet_CXX_FLAGS  " " CACHE PATH "CXX compile flags")
set(EicDet_CXX_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_SHARED_LINKER_FLAGS}" CACHE PATH "Linking flags")
set(EicDet_CXX_INC_DIR "-I${CMAKE_INSTALL_PREFIX}/include/EicDet "          CACHE PATH "include dir")
set(EicDet_CXX_LIBS    "" CACHE PATH "libraries")
set(EicDet_FC_FLAGS   "" CACHE PATH "fortran compile flags")
set(EicDet_DATA_DIR /noarch/data CACHE PATH "data directory")

#set (EicDet_COMPILER_FLAGS "-Wall")
set(EicDet_COMPILER_FLAGS   "-Wno-unused-variable -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS         "-std=c++14 ${CMAKE_CXX_FLAGS} ${EicDet_COMPILER_FLAGS} ")
set(EicDet_PDF_GRID_DIR     "${PROJECT_SOURCE_DIR}/grid")
set(EicDet_GRID_DATA_DIR    "${EicDet_GRID_DATA_DIR} ${EicDet_PDF_GRID_DIR}")
set(CMAKE_USE_ROOT6        ON)

# ---------------------------------------------------------------------------
# Must be out of source build
macro(EicDet_CHECK_OUT_OF_SOURCE_BUILD)
  get_filename_component(bindir_parent ${CMAKE_BINARY_DIR} PATH)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/Testing)
    file(REMOVE ${CMAKE_SOURCE_DIR}/DartConfiguration.tcl)
    message(FATAL_ERROR "EicDet should be built as an out of source build, to keep the source directory clean. Please create a extra build directory and run the command 'cmake <path_to_source_dir>' in this newly created directory.  You have also to delete the directory CMakeFiles and the file CMakeCache.txt in the source directory. Otherwise cmake will complain even if you run it from an out-of-source directory.")
  elseif(IS_SYMLINK ${CMAKE_BINARY_DIR} AND CMAKE_SOURCE_DIR STREQUAL bindir_parent)
    message(FATAL_ERROR "EicDet cannot be built from a sub-directory of the source tree that is a symlink. This is a current limitation of CMake. Please create a real build directory and run the command 'cmake <path_to_source_dir>' in this newly created directory.")
  endif()
endmacro()
# NOTE: Doesn't work with older cmake versions:
EicDet_CHECK_OUT_OF_SOURCE_BUILD()

# Check the compiler and set the compile and link flags
#set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH    "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
include_directories(APPEND ${PROJECT_BINARY_DIR})

#----------------------------------------------------------------------------
# Find Libraries

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS}/etc/cmake)
find_package(ROOT REQUIRED COMPONENTS MathCore RIO Hist MathMore Graf Graf3d
   Gpad GX11 GeomPainter X3d Gviz3d RGL
   Eve EG Foam GenVector Geom GLEW Gui HistPainter MathCore Matrix MLP Net
   Physics Spectrum Thread Tree TreeViewer)
include(${ROOT_USE_FILE})
include(cmake/root.cmake)

FIND_PACKAGE( ILCUTIL REQUIRED COMPONENTS ILCSOFT_CMAKE_MODULES )

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )


FIND_PACKAGE( LCIO ) #REQUIRED COMPONENTS DDRec 
include_directories(${LCIO_INCLUDE_DIRS})


set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}  ${DD4hep_ROOT}/cmake )
set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}  ${DD4hep_ROOT}/lib/DD4hep )

FIND_PACKAGE( DD4hep REQUIRED COMPONENTS DDCore DDRec DDG4 )
INCLUDE_DIRECTORIES( SYSTEM ${DD4hep_INCLUDE_DIRS} )
LINK_LIBRARIES( ${DD4hep_LIBRARIES} )
ADD_DEFINITIONS( ${DD4hep_DEFINITIONS} )


find_package( ROOT REQUIRED )
set( ROOT_COMPONENT_LIBRARIES Geom)


# ----------
# CLHEP
#set (CLHEP_CONFIG_DEBUG 1)
#find_package(CLHEP  REQUIRED)
##  CLHEP_INCLUDE_DIRS - include directories for CLHEP
##  CLHEP_DEFINITIONS  - compile definitions needed to use CLHEP
##  CLHEP_LIBRARIES    - libraries to link against
#set(CLHEP_LIBRARY_DIR $ENV{JLAB_PREFIX}/lib)
#include_directories(${CLHEP_INCLUDE_DIR})
##message(" CLHEP_DEFINITIONS = ${CLHEP_DEFINITIONS} " )

# ----------
## EVIO
##set(EVIO_INCLUDE_DIR $ENV{JLAB_PREFIX}/include)
##set(EVIO_LIBRARY_DIR $ENV{JLAB_PREFIX}/lib)
##set(EVIO_LIBRARIES  evio evioxx expat)

set(LINK_LIBRARIES 
   ${LINK_LIBRARIES}
   ${ROOT_LIBRARIES}
   ${LCIO_LIBRARIES}
   ${DD4hep_LIBRARIES}
   #${EVIO_LIBRARIES}
   #${CLHEP_LIBRARIES}
   )

#----------------------------------------------------------------------------
# not sure this is the best way to do this....
set(EicDet_FC_FLAGS "${EicDet_FC_FLAGS} ${CMAKE_Fortran_FLAGS}")
#set(CMAKE_Fortran_FLAGS " -D\"EicDet_PDF_GRID_DIR='${EicDet_PDF_GRID_DIR}'\" -g -O2 -Wall -fPIC -Iinclude -ffixed-line-length-none -x f77-cpp-input ")
set(CMAKE_Fortran_FLAGS " -D\"EicDet_PDF_GRID_DIR='${EicDet_PDF_GRID_DIR}'\" -g -O2 -Wall  -ffixed-line-length-none -x f77-cpp-input ")
set(CMAKE_Fortran_FLAGS_RELEASE "${CMAKE_Fortran_FLAGS}")
set(CMAKE_Fortran_FLAGS " -D\"EicDet_PDF_GRID_DIR='${EicDet_PDF_GRID_DIR}'\" -g -O2 -Wall  -ffixed-line-length-none -x f77-cpp-input ")
set(EicDet_CXX_LDFLAGS   "${EicDet_CXX_LDFLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")

# -------------------------------------------------------
# Make string of libs for commandline config tool
set(EicDet_SHARED_LIB_NAMES)
foreach(aSharedLib ${EicDet_LIB_NAMES})
   set(EicDet_CXX_LIBS          "${EicDet_CXX_LIBS} -l${aSharedLib} ")
   set(EicDet_SHARED_LIB_NAMES   ${EicDet_SHARED_LIB_NAMES} ${aSharedLib})
endforeach(aSharedLib)

set(EicDet_SHARED_LIBS)
set(EicDet_LIBRARIES)
set(EicDet_ROOT_DICTS)
set(EicDet_PCM_FILES)

#---------------------------------------
foreach(aSharedLib ${EicDet_LIB_NAMES})
   set(EicDet_LIBRARIES ${EicDet_LIBRARIES} ${aSharedLib})
   #set(EicDet_SHARED_LIBS ${EicDet_SHARED_LIBS} "${CMAKE_BINARY_DIR}/lib/EicDet${aSharedLib}.so")
   set(EicDet_ROOT_DICTS ${EicDet_ROOT_DICTS} "EicDet${aSharedLib}Dict.h")
   set(EicDet_ROOT_DICT_SRCS ${EicDet_ROOT_DICT_SRCS} "EicDet${aSharedLib}Dict.cxx")
   set(EicDet_PCM_FILES ${EicDet_PCM_FILES} "EicDet${aSharedLib}Dict_rdict.pcm")
   include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/${aSharedLib}/include)
endforeach(aSharedLib)

# -------------------------------------------------------
# configure a header file to pass some of the CMake settings to the source code
configure_file (
   "${PROJECT_SOURCE_DIR}/src/EicDetConfig.h.in"
   "${PROJECT_BINARY_DIR}/EicDetConfig.h"
   )

#----------------------------------------------------------------------------
# Add the config tool directory
#add_executable(EicDet-config EicDet-config.cxx )
add_subdirectory(src/config)

#-----------------------------------------
# add the library sub directories
foreach(aSharedLib ${EicDet_LIB_NAMES})
   #include(${aSharedLib})
   add_subdirectory("src/${aSharedLib}")
endforeach(aSharedLib)

# add the executable sub directories
foreach(anExeName ${EicDet_EXE_NAMES})
  #include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/${anExeName}/include)
  add_subdirectory("src/${anExeName}")
endforeach(anExeName)

FILE(GLOB GENERATED_HEADERS 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h" 
  "${PROJECT_BINARY_DIR}/EicDetConfig.h")
FILE(GLOB PCM_FILES "${PROJECT_BINARY_DIR}/*.pcm")

# -----------------------------------------------------------
# Set the library version in the main CMakeLists.txt
SET(EicDet_LIBRARY_PROPERTIES ${EicDet_LIBRARY_PROPERTIES}
  VERSION "${EicDet_VERSION}"
  SOVERSION "${EicDet_MAJOR_VERSION}"
  SUFFIX ".so"
  )

#----------------------------------------------------------------------------
# Configure and install "share" files.  
# These include mostly root scripts and examples.
set(SHARE_SCRIPTS
  #examples/example1.cxx
  #share/plot_field.cxx
  #share/plot_field_xy.cxx
  )
foreach(_script ${SHARE_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()
install(FILES ${SHARE_SCRIPTS} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}" )

# install examples
set(EicDet_EXAMPLES 
  examples/example1.cxx 
  )
install(FILES ${EicDet_EXAMPLES} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/examples" )

# ----------------------------------------------------------------------------
# The library's cmake files
# Add all targets to the build-tree export set
#export(TARGETS EicDet FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
# TODO: replace "EicDet" with smaller libraries

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ${PROJECT_NAME})

set(CONF_LIBRARIES 
   ${EicDet_SHARED_LIB_NAMES}  # TODO: Break big library into smaller ones
   )

set(CONF_EXECUTABLES 
   EicDet-config
   )

set(CONF_INCLUDE_DIRS 
   ${CMAKE_INSTALL_PREFIX}/include
   ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
   )

set(CONF_LIBRARY_DIRS 
   ${CMAKE_INSTALL_PREFIX}/lib
   )

configure_file(src/config/${PROJECT_NAME}Config.cmake.in 
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}Config.cmake" @ONLY)
configure_file(src/config/${PROJECT_NAME}ConfigVersion.cmake.in 
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)
configure_file(src/config/${PROJECT_NAME}UseFile.cmake.in 
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}UseFile.cmake" @ONLY)

# Install the FooBarConfig.cmake and FooBarConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}Config.cmake"
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}ConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}UseFile.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

# Install the export set for use with the install-tree
install(EXPORT ${PROJECT_NAME}Targets DESTINATION
  "${INSTALL_CMAKE_DIR}" COMPONENT dev)

#------------------------------------
# Debug
MESSAGE("          EicDet_CXX_LIBS: ${EicDet_CXX_LIBS}")
MESSAGE("       EicDet_CXX_LDFLAGS: ${EicDet_CXX_LDFLAGS}")
MESSAGE("       EicDet_CXX_INC_DIR: ${EicDet_CXX_INC_DIR}")
MESSAGE("             CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
MESSAGE("              FC_COMPILER: ${CMAKE_FC_COMPILER}")
MESSAGE("      CMAKE_Fortran_FLAGS: ${CMAKE_Fortran_FLAGS}")
MESSAGE("                CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
MESSAGE("         CMAKE_LINK_FLAGS: ${CMAKE_LINK_FLAGS}")
MESSAGE("CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")

message("EicDet_MAJOR_VERSION   ${EicDet_MAJOR_VERSION}")
message("EicDet_MINOR_VERSION   ${EicDet_MINOR_VERSION}")
message("EicDet_PATCH_VERSION   ${EicDet_PATCH_VERSION}")
message("EicDet_DATA_DIR      ${EicDet_DATA_DIR}")
message("EicDet_CXX_FLAGS    ${EicDet_CXX_FLAGS}")
message("EicDet_CXX_LDFLAGS   ${EicDet_CXX_LDFLAGS}")
message("EicDet_CXX_LIBS      ${EicDet_CXX_LIBS}")
message("EicDet_CXX_INC_DIR   ${EicDet_CXX_INC_DIR}")
message("EicDet_PREFIX        ${CMAKE_INSTALL_PREFIX}")

