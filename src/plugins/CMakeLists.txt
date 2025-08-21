cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Build the plugin library
dd4hep_add_plugin(NPDetPlugins
  SOURCES
    src/EICInteractionVertexBoost.cxx
    src/EICInteractionVertexSmear.cxx
    src/OpticalPhotonEfficiencyStackingAction.cxx
    src/DetectorVersionRunAction.cxx
  INCLUDES $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  USES DD4hep::DDCore DD4hep::DDG4
)

# Create components file template
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libNPDetPlugins.components.in
"v2::libNPDetPlugins.so:EICInteractionVertexBoost
v2::libNPDetPlugins.so:EICInteractionVertexSmear
v2::libNPDetPlugins.so:OpticalPhotonEfficiencyStackingAction
v2::libNPDetPlugins.so:DetectorVersionRunAction")

# Copy components file to lib directory
add_custom_command(TARGET NPDetPlugins POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/libNPDetPlugins.components.in
            ${CMAKE_BINARY_DIR}/lib/libNPDetPlugins.components
    COMMENT "Installing components file"
)

# Install the components file
install(FILES ${CMAKE_BINARY_DIR}/lib/libNPDetPlugins.components
        DESTINATION lib
)

install(TARGETS NPDetPlugins
  EXPORT NPDetTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
