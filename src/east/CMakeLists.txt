cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

find_package(east)

dd4hep_add_plugin(eASTPhysicsListPlugin
  SOURCES
    src/eASTPhysicsListPlugin.cxx
  INCLUDES $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  USES DD4hep::DDCore DD4hep::DDG4 east::eASTPhysicsListBase
)

install(TARGETS eASTPhysicsListPlugin
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
