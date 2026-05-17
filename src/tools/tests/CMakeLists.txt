set(test_name npdet_tools_unit_settings)
add_executable(${test_name} unit_settings.cxx)
target_include_directories(${test_name}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(${test_name}
  PRIVATE ROOT::Geom
)
add_test(NAME ${test_name} COMMAND ${test_name})

if(TARGET npdet_to_step)
  add_test(
    NAME tools_integration_npdet_to_step_help
    COMMAND $<TARGET_FILE:npdet_to_step> help
  )
  set_tests_properties(tools_integration_npdet_to_step_help
    PROPERTIES PASS_REGULAR_EXPRESSION "Geometry tool for converting compact files to STEP")

  add_test(
    NAME tools_integration_npdet_to_step_missing_input
    COMMAND $<TARGET_FILE:npdet_to_step> list /tmp/npsim_missing_compact.xml
  )
  set_tests_properties(tools_integration_npdet_to_step_missing_input
    PROPERTIES WILL_FAIL TRUE PASS_REGULAR_EXPRESSION "does not exist")
endif()
