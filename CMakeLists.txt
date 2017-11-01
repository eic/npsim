#----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(NPDet CXX)

# Add library source directory name that appear in ./src
set(NPDet_LIB_NAMES
  GenericDetectors
  )

# Add executable source directory name that appear in ./src
set(NPDet_EXE_NAMES
  )

# Add concept detector names
set(NPDet_CONCEPT_NAMES
  JLEIC
  SoLID
  eRHIC
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
   )

if(NOT CMAKE_INSTALL_PREFIX)
   SET(NPDet_PREFIX "/usr/local")
else() 
   SET(NPDet_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

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

#----------------------------------------------------------------------------
# Find Libraries
FIND_PACKAGE( DD4hep REQUIRED COMPONENTS DDCore DDG4 )

#-----------------------------------------
# add the library sub directories
foreach(aSharedLib ${NPDet_LIB_NAMES})
   add_subdirectory("src/${aSharedLib}")
endforeach(aSharedLib)

# add the executable sub directories
foreach(anExeName ${NPDet_EXE_NAMES})
  add_subdirectory("src/${anExeName}")
endforeach(anExeName)

# add the concept detector sub directories
foreach(aConceptName ${NPDet_CONCEPT_NAMES})
    add_subdirectory("src/ConceptDetectors/${aConceptName}")
endforeach(aConceptName)

#----------------------------------------------------------------------------
# install examples
set(NPDet_EXAMPLES 
  )
install(FILES ${NPDet_EXAMPLES} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/examples" )

# -----------------------------------------------------------
# Set the library version in the main CMakeLists.txt
SET(NPDet_LIBRARY_PROPERTIES ${NPDet_LIBRARY_PROPERTIES}
  VERSION "${NPDet_VERSION}"
  SOVERSION "${NPDet_MAJOR_VERSION}"
  SUFFIX ".so"
  )

#----------------------------------------------------------------------------
# Add the config tool directory
add_subdirectory(src/config)

