#----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(NPDet CXX)

# Add library source directory name that appear in ./src
set(NPDet_LIB_NAMES
  simple_example
  GenericDetectors
  ConceptDetectors/JLEIC
  #clas12
  ConceptDetectors/SoLID
  ConceptDetectors/eRHIC
  )

# Add executable source directory name that appear in ./src
set(NPDet_EXE_NAMES
  #NPDet
  #runtest
  )

# The version number.
set(NPDet_MAJOR_VERSION 0)
set(NPDet_MINOR_VERSION 1)
set(NPDet_PATCH_VERSION 0)
SET(NPDet_VERSION "${NPDet_MAJOR_VERSION}.${NPDet_MINOR_VERSION}.${NPDet_PATCH_VERSION}")

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
   SET(NPDet_PREFIX "/usr/local")
else() 
   SET(NPDet_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

# ---------------------------------------------------------------------------
#
set(NPDet_CXX_FLAGS  " " CACHE PATH "CXX compile flags")
set(NPDet_CXX_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_SHARED_LINKER_FLAGS}" CACHE PATH "Linking flags")
set(NPDet_CXX_INC_DIR "-I${CMAKE_INSTALL_PREFIX}/include/NPDet "          CACHE PATH "include dir")
set(NPDet_CXX_LIBS    "" CACHE PATH "libraries")
set(NPDet_FC_FLAGS   "" CACHE PATH "fortran compile flags")
set(NPDet_DATA_DIR /noarch/data CACHE PATH "data directory")

#set (NPDet_COMPILER_FLAGS "-Wall")
set(NPDet_COMPILER_FLAGS   "-Wno-unused-variable -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS         "-std=c++14 ${CMAKE_CXX_FLAGS} ${NPDet_COMPILER_FLAGS} ")
set(NPDet_PDF_GRID_DIR     "${PROJECT_SOURCE_DIR}/grid")
set(NPDet_GRID_DATA_DIR    "${NPDet_GRID_DATA_DIR} ${NPDet_PDF_GRID_DIR}")
set(CMAKE_USE_ROOT6        ON)

# ---------------------------------------------------------------------------
# Must be out of source build
macro(NPDet_CHECK_OUT_OF_SOURCE_BUILD)
  get_filename_component(bindir_parent ${CMAKE_BINARY_DIR} PATH)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/Testing)
    file(REMOVE ${CMAKE_SOURCE_DIR}/DartConfiguration.tcl)
    message(FATAL_ERROR "NPDet should be built as an out of source build, to keep the source directory clean. Please create a extra build directory and run the command 'cmake <path_to_source_dir>' in this newly created directory.  You have also to delete the directory CMakeFiles and the file CMakeCache.txt in the source directory. Otherwise cmake will complain even if you run it from an out-of-source directory.")
  elseif(IS_SYMLINK ${CMAKE_BINARY_DIR} AND CMAKE_SOURCE_DIR STREQUAL bindir_parent)
    message(FATAL_ERROR "NPDet cannot be built from a sub-directory of the source tree that is a symlink. This is a current limitation of CMake. Please create a real build directory and run the command 'cmake <path_to_source_dir>' in this newly created directory.")
  endif()
endmacro()
# NOTE: Doesn't work with older cmake versions:
NPDet_CHECK_OUT_OF_SOURCE_BUILD()

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

FIND_PACKAGE( LCIO ) #REQUIRED COMPONENTS DDRec 
include_directories(${LCIO_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}  ${DD4hep_ROOT}/cmake )
set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}  ${DD4hep_ROOT}/lib/DD4hep )

FIND_PACKAGE( DD4hep REQUIRED COMPONENTS DDCore DDRec DDG4 )
INCLUDE_DIRECTORIES( SYSTEM ${DD4hep_INCLUDE_DIRS} )
LINK_LIBRARIES( ${DD4hep_LIBRARIES} )
ADD_DEFINITIONS( ${DD4hep_DEFINITIONS} )




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
set(NPDet_FC_FLAGS "${NPDet_FC_FLAGS} ${CMAKE_Fortran_FLAGS}")
#set(CMAKE_Fortran_FLAGS " -D\"NPDet_PDF_GRID_DIR='${NPDet_PDF_GRID_DIR}'\" -g -O2 -Wall -fPIC -Iinclude -ffixed-line-length-none -x f77-cpp-input ")
set(CMAKE_Fortran_FLAGS " -D\"NPDet_PDF_GRID_DIR='${NPDet_PDF_GRID_DIR}'\" -g -O2 -Wall  -ffixed-line-length-none -x f77-cpp-input ")
set(CMAKE_Fortran_FLAGS_RELEASE "${CMAKE_Fortran_FLAGS}")
set(CMAKE_Fortran_FLAGS " -D\"NPDet_PDF_GRID_DIR='${NPDet_PDF_GRID_DIR}'\" -g -O2 -Wall  -ffixed-line-length-none -x f77-cpp-input ")
set(NPDet_CXX_LDFLAGS   "${NPDet_CXX_LDFLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")

# -------------------------------------------------------
# Make string of libs for commandline config tool
set(NPDet_SHARED_LIB_NAMES)
foreach(aSharedLib ${NPDet_LIB_NAMES})
   set(NPDet_CXX_LIBS          "${NPDet_CXX_LIBS} -l${aSharedLib} ")
   set(NPDet_SHARED_LIB_NAMES   ${NPDet_SHARED_LIB_NAMES} ${aSharedLib})
endforeach(aSharedLib)

set(NPDet_SHARED_LIBS)
set(NPDet_LIBRARIES)
set(NPDet_ROOT_DICTS)
set(NPDet_PCM_FILES)

#---------------------------------------
foreach(aSharedLib ${NPDet_LIB_NAMES})
   set(NPDet_LIBRARIES ${NPDet_LIBRARIES} ${aSharedLib})
   #set(NPDet_SHARED_LIBS ${NPDet_SHARED_LIBS} "${CMAKE_BINARY_DIR}/lib/NPDet${aSharedLib}.so")
   set(NPDet_ROOT_DICTS ${NPDet_ROOT_DICTS} "NPDet${aSharedLib}Dict.h")
   set(NPDet_ROOT_DICT_SRCS ${NPDet_ROOT_DICT_SRCS} "NPDet${aSharedLib}Dict.cxx")
   set(NPDet_PCM_FILES ${NPDet_PCM_FILES} "NPDet${aSharedLib}Dict_rdict.pcm")
   include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/${aSharedLib}/include)
endforeach(aSharedLib)

# -------------------------------------------------------
# configure a header file to pass some of the CMake settings to the source code
configure_file (
   "${PROJECT_SOURCE_DIR}/src/NPDetConfig.h.in"
   "${PROJECT_BINARY_DIR}/NPDetConfig.h"
   )

#----------------------------------------------------------------------------
# Add the config tool directory
#add_executable(NPDet-config NPDet-config.cxx )
add_subdirectory(src/config)

#-----------------------------------------
# add the library sub directories
foreach(aSharedLib ${NPDet_LIB_NAMES})
   #include(${aSharedLib})
   add_subdirectory("src/${aSharedLib}")
endforeach(aSharedLib)

# add the executable sub directories
foreach(anExeName ${NPDet_EXE_NAMES})
  #include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/${anExeName}/include)
  add_subdirectory("src/${anExeName}")
endforeach(anExeName)

FILE(GLOB GENERATED_HEADERS 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h" 
  "${PROJECT_BINARY_DIR}/NPDetConfig.h")
FILE(GLOB PCM_FILES "${PROJECT_BINARY_DIR}/*.pcm")

# -----------------------------------------------------------
# Set the library version in the main CMakeLists.txt
SET(NPDet_LIBRARY_PROPERTIES ${NPDet_LIBRARY_PROPERTIES}
  VERSION "${NPDet_VERSION}"
  SOVERSION "${NPDet_MAJOR_VERSION}"
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
set(NPDet_EXAMPLES 
  #examples/example1.cxx 
  )
install(FILES ${NPDet_EXAMPLES} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/examples" )

# ----------------------------------------------------------------------------
# The library's cmake files
# Add all targets to the build-tree export set
#export(TARGETS NPDet FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
# TODO: replace "NPDet" with smaller libraries

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ${PROJECT_NAME})

set(CONF_LIBRARIES 
   ${NPDet_SHARED_LIB_NAMES}  # TODO: Break big library into smaller ones
   )

set(CONF_EXECUTABLES 
   NPDet-config
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
MESSAGE("          NPDet_CXX_LIBS: ${NPDet_CXX_LIBS}")
MESSAGE("       NPDet_CXX_LDFLAGS: ${NPDet_CXX_LDFLAGS}")
MESSAGE("       NPDet_CXX_INC_DIR: ${NPDet_CXX_INC_DIR}")
MESSAGE("             CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
MESSAGE("              FC_COMPILER: ${CMAKE_FC_COMPILER}")
MESSAGE("      CMAKE_Fortran_FLAGS: ${CMAKE_Fortran_FLAGS}")
MESSAGE("                CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
MESSAGE("         CMAKE_LINK_FLAGS: ${CMAKE_LINK_FLAGS}")
MESSAGE("CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")

message("NPDet_MAJOR_VERSION   ${NPDet_MAJOR_VERSION}")
message("NPDet_MINOR_VERSION   ${NPDet_MINOR_VERSION}")
message("NPDet_PATCH_VERSION   ${NPDet_PATCH_VERSION}")
message("NPDet_DATA_DIR      ${NPDet_DATA_DIR}")
message("NPDet_CXX_FLAGS    ${NPDet_CXX_FLAGS}")
message("NPDet_CXX_LDFLAGS   ${NPDet_CXX_LDFLAGS}")
message("NPDet_CXX_LIBS      ${NPDet_CXX_LIBS}")
message("NPDet_CXX_INC_DIR   ${NPDet_CXX_INC_DIR}")
message("NPDet_PREFIX        ${CMAKE_INSTALL_PREFIX}")

