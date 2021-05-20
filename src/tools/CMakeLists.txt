cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# see https://rix0r.nl/blog/2015/08/13/cmake-guide/
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(DD4hep REQUIRED COMPONENTS DDCore DDG4 )

find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})

set(exe_name npdet_fields)
add_executable(${exe_name} src/${exe_name}.cxx src/settings.cxx)
target_link_libraries(${exe_name}
  PUBLIC DD4hep::DDCore
  )
target_include_directories(${exe_name} 
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_std_17
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Gpad)

install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )

# npdet_to_step needs ROOT's GeoCad library
if(USE_GEOCAD)

  set(exe_name npdet_to_step)
  add_executable(${exe_name} src/${exe_name}.cxx src/settings.cxx)
  target_include_directories(${exe_name} 
    PRIVATE include )
  target_compile_features(${exe_name}
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PRIVATE cxx_variadic_templates 
    PRIVATE cxx_std_17)
  target_link_libraries(${exe_name} 
    PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Gpad GeoCad)
  install(TARGETS ${exe_name}
    EXPORT NPDetTargets
    RUNTIME DESTINATION bin )
endif()

set(exe_name npdet_to_teve)
add_executable(${exe_name} src/${exe_name}.cxx src/settings.cxx)
target_include_directories(${exe_name}
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_std_17
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates 
  )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Eve ROOT::Gpad ROOT::Imt)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )

set(exe_name npdet_mat_budget)
add_executable(${exe_name} src/${exe_name}.cxx src/settings.cxx)
target_include_directories(${exe_name}
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_std_17
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates 
  )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDRec DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Eve ROOT::Gpad ROOT::Imt)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )



# ------------------------------------
#
set(exe_name dd_web_display)
add_executable(${exe_name} src/${exe_name}.cxx src/settings.cxx)
target_include_directories(${exe_name}
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates 
  PUBLIC cxx_std_17
  )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Eve ROOT::Gpad ROOT::Imt ROOT::RHTTP ROOT::Net fmt::fmt spdlog::spdlog)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )


# ------------------------------------
#
set(exe_name npdet_info)
add_executable(${exe_name} src/${exe_name}.cxx )
target_include_directories(${exe_name}
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates 
  PUBLIC cxx_std_17
  )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Eve ROOT::Gpad ROOT::Imt ROOT::RHTTP ROOT::Net 
  fmt::fmt 
  spdlog::spdlog)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )


# ----------------------------------------------
# Tests

## InitialState
#set(test_name simple)
#add_executable(${test_name} tests/${test_name}.cxx src/settings.cxx)
##target_include_directories(${test_name} PUBLIC
##  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/core>
##  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
##  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
##  )
#target_link_libraries(${test_name} Catch2::Catch2)
#add_test(NAME ${test_name} COMMAND ${test_name} -r junit -o test_result_${test_name}.xml 
#  WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )



