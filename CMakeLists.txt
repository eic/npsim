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
    eRHIC
    JLEIC
    SiEIC
    SoLID
    )

# The version number.
set(NPDet_MAJOR_VERSION 0)
set(NPDet_MINOR_VERSION 1)
set(NPDet_PATCH_VERSION 0)
set(NPDet_VERSION "${NPDet_MAJOR_VERSION}.${NPDet_MINOR_VERSION}.${NPDet_PATCH_VERSION}")

# ---------------------------------------------------------------------------
# set default install prefix and build type
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT FALSE)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "..." FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "..." FORCE)
endif()

# ---------------------------------------------------------------------------
# load additional OS dependent settings
include(cmake/os.cmake)

#----------------------------------------------------------------------------
# Find Libraries
find_package( DD4hep REQUIRED COMPONENTS DDCore DDG4 )

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
install(FILES ${NPDet_EXAMPLES}
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/examples"
    )

#----------------------------------------------------------------------------
# Add the config tool/script directory
add_subdirectory(src/config)

