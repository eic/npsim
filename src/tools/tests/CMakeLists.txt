# ----------------------------------------------
# Tests

# InitialState
set(test_name simple)
add_executable(${test_name} tests/${test_name}.cxx)
target_include_directories(${test_name} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/core>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  )
target_link_libraries(${test_name}
  PRIVATE insanePhysics
  PRIVATE Catch)
add_test(NAME ${test_name} COMMAND ${test_name})
