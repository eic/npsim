# Need to have found opencascade or forced USE_GEOCAD to ON
if(NOT USE_GEOCAD)
  return()
endif()

find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
find_package(OpenCASCADE REQUIRED)


ROOT_GENERATE_DICTIONARY(G__NPDetGeoCad
  include/TGeoToStep.h
  OPTIONS -I${OpenCASCADE_INCLUDE_DIR} 
  OPTIONS -I${CMAKE_CURRENT_SOURCE_DIR} 
  OPTIONS -I${CMAKE_CURRENT_SOURCE_DIR}/include 
  OPTIONS -I${CMAKE_CURRENT_SOURCE_DIR}/src 
  LINKDEF include/LinkDef.h
  )
add_custom_target(G__NPDetGeoCad_ROOTDICT DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/G__NPDetGeoCad.cxx)


add_library(GeoCad SHARED
  src/TGeoToOCC.cxx
  src/TGeoToStep.cxx
  src/TOCCToStep.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/G__NPDetGeoCad.cxx
  )
add_dependencies(GeoCad G__NPDetGeoCad_ROOTDICT)

target_compile_features(GeoCad
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PUBLIC cxx_std_17
    PRIVATE cxx_variadic_templates
    )

target_compile_options(GeoCad PRIVATE 
  -Wno-extra
  -Wno-ignored-qualifiers
  -Wno-overloaded-virtual
  -Wno-shadow)

target_link_libraries(GeoCad 
  PUBLIC ${OpenCASCADE_LIBRARIES}
  PUBLIC ROOT::Geom)

target_include_directories(GeoCad 
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include/npdet>
  PUBLIC ${OpenCASCADE_INCLUDE_DIR})

install(FILES 
  include/TGeoToStep.h
  DESTINATION include
  )
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/libNPDetGeoCad.rootmap"
  "${CMAKE_CURRENT_BINARY_DIR}/libNPDetGeoCad_rdict.pcm"
  DESTINATION lib)


install(TARGETS GeoCad
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )
