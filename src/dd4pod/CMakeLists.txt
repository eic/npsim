cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

include(GNUInstallDirs)

message(" ======================= ")
message(" ${podio_INCLUDE_DIR}")
#find_package(podio 0.11.0 REQUIRED )
#if(NOT podio_FOUND)
find_package(DD4hep REQUIRED COMPONENTS DDCore DDG4 )
find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})

if(BUILD_DATA_MODEL) 
  set(dd4pod_include_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(dd4pod_source_dir ${CMAKE_CURRENT_BINARY_DIR}/src)
PODIO_GENERATE_DATAMODEL(dd4pod dd4hep.yaml headers sources OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
else()
  set(dd4pod_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
  set(dd4pod_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
  file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
  file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/include/dd4pod/*.h)
endif()

add_library(DD4pod SHARED
  ${sources}
  #src/EICInteractionVertexBoost.cxx
  #src/EICInteractionVertexSmear.cxx
  )

target_link_libraries(DD4pod
  PUBLIC podio::podio
  PUBLIC podio::podioRootIO
  PUBLIC DD4hep::DDCore
  PUBLIC DD4hep::DDG4
  )
target_include_directories(DD4pod
  PUBLIC  ${podio_INCLUDE_DIR}
  PUBLIC $<BUILD_INTERFACE:${dd4pod_include_dir}>
  $<INSTALL_INTERFACE:include>)

target_compile_features(DD4pod
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates
  PRIVATE cxx_std_17)


message("headers: ${headers}")
PODIO_GENERATE_DICTIONARY(DD4pod ${headers} 
  SELECTION ${dd4pod_source_dir}/selection.xml
  OPTIONS --library ${CMAKE_SHARED_LIBRARY_PREFIX}DD4pod${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
set_target_properties(DD4pod-dictgen PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_sources(DD4pod PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/DD4pod.cxx)

dd4hep_add_plugin(DD4podIO
    SOURCES 
    plugins/Geant4Output2Podio.cxx
    NOINSTALL
    )
target_link_libraries(DD4podIO
  PUBLIC DD4pod
  PUBLIC DDG4_PIDsdet
  #PUBLIC podio::podio
  #PUBLIC podio::podioRootIO
  #PUBLIC DD4hep::DDCore
  #PUBLIC DD4hep::DDG4
  #PUBLIC ROOT::Core ROOT::Tree ROOT::RIO
  )
target_include_directories(DD4pod
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins>
  )
target_compile_features(NPDetPlugins
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PRIVATE cxx_variadic_templates
    PRIVATE cxx_std_17
    )


install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dd4pod
  DESTINATION include
  FILES_MATCHING PATTERN *.h
  )
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/DD4podDict.rootmap"
  "${CMAKE_CURRENT_BINARY_DIR}/libDD4pod_rdict.pcm"
  DESTINATION lib)


configure_file(
    python/npsim.in
    ${CMAKE_CURRENT_BINARY_DIR}/npsim
    )
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/npsim"
  PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ 
  DESTINATION bin)

install(FILES
  python/npsim/DDG4.py
  DESTINATION share/npdet/python
  )
install(FILES
  python/npsim/DDSim/DD4hepSimulation.py
  DESTINATION share/npdet/python/DDSim
  )
install(DIRECTORY
  python/npsim/DDSim
  DESTINATION share/npdet/python
  )

install(PROGRAMS
  python/ddsim.py
  DESTINATION bin
  )




install(TARGETS DD4pod DD4podIO
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )

#endif()

