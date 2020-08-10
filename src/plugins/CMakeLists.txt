cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# see https://rix0r.nl/blog/2015/08/13/cmake-guide/
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

find_package( DD4hep REQUIRED COMPONENTS DDCore DDG4 )
find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})

dd4hep_add_plugin(NPDetPlugins
    SOURCES 
    src/EICInteractionVertexBoost.cxx
    src/EICInteractionVertexSmear.cxx
    NOINSTALL
    )

target_link_libraries(NPDetPlugins
  PUBLIC DD4hep::DDCore
  PUBLIC DD4hep::DDG4
  )
target_include_directories(NPDetPlugins
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

target_compile_features(NPDetPlugins
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PRIVATE cxx_variadic_templates
    PRIVATE cxx_std_17
    )

install(TARGETS NPDetPlugins
    EXPORT NPDetTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
    )



