cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

include(GNUInstallDirs)

find_package(podio)
if(podio_FOUND)
find_package( DD4hep REQUIRED COMPONENTS DDCore DDG4 )
find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})

PODIO_GENERATE_DATAMODEL(dd4pod dd4hep.yaml headers sources OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})


add_library(DD4pod SHARED
  ${sources}
  #src/EICInteractionVertexBoost.cxx
  #src/EICInteractionVertexSmear.cxx
  )

target_link_libraries(DD4pod
  PUBLIC podio::podio
  PUBLIC DDG4_PIDsdet
  PUBLIC DD4hep::DDCore
  )
target_include_directories(DD4pod
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>)

target_compile_features(DD4pod
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates
  PRIVATE cxx_std_17)


PODIO_GENERATE_DICTIONARY(DD4pod ${headers} 
  SELECTION ${CMAKE_CURRENT_BINARY_DIR}/src/selection.xml
  OPTIONS --library ${CMAKE_SHARED_LIBRARY_PREFIX}DD4pod${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
set_target_properties(DD4pod-dictgen PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_sources(DD4pod PRIVATE DD4pod.cxx)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dd4pod
  DESTINATION include
  FILES_MATCHING PATTERN *.h
  )

install(TARGETS DD4pod
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )


endif()

