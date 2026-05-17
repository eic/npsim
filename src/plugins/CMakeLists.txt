cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

dd4hep_add_plugin(NPDetPlugins
  SOURCES
    src/EICInteractionVertexBoost.cxx
    src/EICInteractionVertexSmear.cxx
    src/OpticalPhotonEfficiencyStackingAction.cxx
  INCLUDES $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  USES DD4hep::DDCore DD4hep::DDG4
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  if(USE_ASAN)
    target_link_options(NPDetPlugins PRIVATE -fsanitize=address)
  endif()
  if(USE_UBSAN)
    target_link_options(NPDetPlugins PRIVATE -fsanitize=undefined)
  endif()
endif()

install(TARGETS NPDetPlugins
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
