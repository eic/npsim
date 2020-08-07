cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(NPDet VERSION 0.2.0
  LANGUAGES CXX)


# Add library source directory name that appear in ./src
set(NPDet_LIB_NAMES
    GenericDetectors
    plugins
    dd4pod
    )

# Add executable source directory name that appear in ./src
set(NPDet_EXE_NAMES
    )

# Add concept detector names
set(NPDet_CONCEPT_NAMES
    erhic
    #jleic
    #sieic
    solid
    topside
    )

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
include(cmake/root.cmake)

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
    PATH_VARS TARGETS_INSTALL_PATH
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
