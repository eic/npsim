cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})

if(BUILD_DATA_MODEL)
  set(dd4pod_include_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(dd4pod_source_dir ${CMAKE_CURRENT_BINARY_DIR}/src)
  podio_generate_datamodel(dd4pod dd4hep.yaml headers sources
    OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
  )
else()
  set(dd4pod_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
  set(dd4pod_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
  file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
  file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/include/dd4pod/*.h)
endif()

podio_add_datamodel_core_lib(DD4pod "${headers}" "${sources}"
  OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)

podio_add_root_io_dict(DD4podDict "${headers}"
  ${dd4pod$wource_dir}/selection.xml
  OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)

dd4hep_add_plugin(DD4podIO
  SOURCES
    plugins/Geant4Output2Podio.cxx
  USES
    DD4pod
    DDG4_PIDsdet
    DD4hep::DDCore DD4hep::DDG4
    podio::podio podio::podioRootIO
    ROOT::Tree ROOT::RIO
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dd4pod
  DESTINATION include
  FILES_MATCHING PATTERN *.h
)

configure_file(
    python/npsim.in
    ${CMAKE_CURRENT_BINARY_DIR}/npsim
    )
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/npsim"
  PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
  DESTINATION bin
  )
install(PROGRAMS
  python/npsim.py
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
