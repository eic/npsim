cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# see https://rix0r.nl/blog/2015/08/13/cmake-guide/
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

find_package( podio REQUIRED)
find_package( DD4hep REQUIRED COMPONENTS DDCore DDG4 )
find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})

add_library(NPDetDD4pod SHARED
  src/CalorimeterHit.cc
  src/CalorimeterHitCollection.cc
  src/CalorimeterHitConst.cc
  src/CalorimeterHitObj.cc
  src/FourVector.cc
  src/FourVectorCollection.cc
  src/FourVectorConst.cc
  src/FourVectorObj.cc
  src/Geant4Particle.cc
  src/Geant4ParticleCollection.cc
  src/Geant4ParticleConst.cc
  src/Geant4ParticleObj.cc
  src/selection.xml
  src/TrackerHit.cc
  src/TrackerHitCollection.cc
  src/TrackerHitConst.cc
  src/TrackerHitObj.cc
  #src/EICInteractionVertexBoost.cxx
  #src/EICInteractionVertexSmear.cxx
  )

target_link_libraries(NPDetDD4pod
  PUBLIC podio::podio
  PUBLIC DDG4_PIDsdet
  PUBLIC DD4hep::DDCore
  )
target_include_directories(NPDetDD4pod
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

target_compile_features(NPDetDD4pod
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PRIVATE cxx_variadic_templates
    PRIVATE cxx_std_17
    )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dd4pod
  DESTINATION include
  FILES_MATCHING PATTERN *.h
    )

install(TARGETS NPDetDD4pod
    EXPORT NPDetTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
    )



