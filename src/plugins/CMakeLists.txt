cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

dd4hep_add_plugin(NPDetPlugins
  SOURCES
    src/EICInteractionVertexBoost.cxx
    src/EICInteractionVertexSmear.cxx
    src/OpticalPhotonEfficiencyStackingAction.cxx
    src/PerformanceProfileSteppingAction.cxx
  INCLUDES $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  USES DD4hep::DDCore DD4hep::DDG4 ROOT::Core
)

install(TARGETS NPDetPlugins
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
