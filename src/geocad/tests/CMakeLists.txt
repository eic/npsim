set(geocad_unit_test npdet_geocad_unit_partial_tree)
add_executable(${geocad_unit_test} unit_partial_tree.cxx)
target_link_libraries(${geocad_unit_test}
  PRIVATE GeoCad ROOT::Geom
)
add_test(NAME ${geocad_unit_test} COMMAND ${geocad_unit_test})

set(geocad_integration_test npdet_geocad_integration_create_step)
add_executable(${geocad_integration_test} integration_create_step.cxx)
target_link_libraries(${geocad_integration_test}
  PRIVATE GeoCad ROOT::Geom
)
add_test(
  NAME ${geocad_integration_test}
  COMMAND ${geocad_integration_test} ${CMAKE_CURRENT_BINARY_DIR}/npdet_geocad_test_geometry.stp
)
