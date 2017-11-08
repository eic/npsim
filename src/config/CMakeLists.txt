# Set what is returned by NPDet-config tool
set(NPDet_CXX_INC_DIR "-I${CMAKE_INSTALL_PREFIX}/include")
set(NPDet_CXX_FLAGS "")
set(NPDet_CXX_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_SHARED_LINKER_FLAGS}")

set(NPDet_CXX_LIBS)
set(NPDet_SHARED_LIB_NAMES)
foreach(aSharedLib ${NPDet_LIB_NAMES} ${NPDet_CONCEPT_NAMES})
    set(NPDet_CXX_LIBS "${NPDet_CXX_LIBS} -l${aSharedLib}")
    set(NPDet_SHARED_LIB_NAMES ${NPDet_SHARED_LIB_NAMES} ${aSharedLib})
endforeach(aSharedLib)

set(NPDet_DATA_DIR /noarch/data)

# -------------------------------------------------------
# configure a header file to pass some of the CMake settings to the source code
configure_file(
    "NPDetConfig.h.in"
    "NPDetConfig.h"
    )

# -------------------------------------------------------
# build and install NPDet-config
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(exename "${PROJECT_NAME}-config")
add_executable(${exename} ${exename}.cxx)

install(
    TARGETS ${exename}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION bin
    COMPONENT bin
    )


#install(EXPORT NPDetTargets
#  FILE GenDetectorsTargets.cmake
#  NAMESPACE NPDet::
#  DESTINATION lib/NPDet )
#
#include(CMakePackageConfigHelpers)
#
#CONFIGURE_PACKAGE_CONFIG_FILE(
#  cmake/GenDetectorsConfig.cmake.in  
#  ${CMAKE_CURRENT_BINARY_DIR}/GenDetectorsConfig.cmake
#  INSTALL_DESTINATION lib/NPDet )
##[PATH_VARS <var1> <var2> ... <varN>]
##[NO_SET_AND_CHECK_MACRO]
##[NO_CHECK_REQUIRED_COMPONENTS_MACRO])
#
#write_basic_package_version_file("GenDetectorsConfigVersion.cmake"
#  VERSION ${NPDet_VERSION}
#  COMPATIBILITY SameMajorVersion
#)
#install(FILES
#  ${CMAKE_CURRENT_BINARY_DIR}/GenDetectorsConfig.cmake
#  ${CMAKE_CURRENT_BINARY_DIR}/GenDetectorsConfigVersion.cmake
#  DESTINATION lib/NPDet
#)

