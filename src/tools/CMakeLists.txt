cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# CMP0074: find_package() uses <PackageName>_ROOT variables
cmake_policy(SET CMP0074 NEW)

include(GNUInstallDirs)

# Required dependencies
find_package(fmt REQUIRED)
find_package(DD4hep REQUIRED COMPONENTS DDCore DDRec)
find_package(ROOT REQUIRED COMPONENTS Geom GenVector Gpad Hist MathCore)
include(${ROOT_USE_FILE})

# Optional dependencies
find_package(spdlog)

# ------------------------------------
# npdet_fields
# ------------------------------------
set(exe_name npdet_fields)
add_executable(${exe_name} src/${exe_name}.cxx src/settings.cxx)
target_include_directories(${exe_name}
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_std_17
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates )
target_link_libraries(${exe_name}
  PUBLIC DD4hep::DDCore ROOT::Core ROOT::Hist ROOT::Gpad)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )


# ------------------------------------
# npdet_to_step
# ------------------------------------
# npdet_to_step needs the (formerly part of ROOT) GeoCad library
if(TARGET GeoCad)
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
    PUBLIC DD4hep::DDCore ROOT::Core GeoCad)
  install(TARGETS ${exe_name}
    EXPORT NPDetTargets
    RUNTIME DESTINATION bin )
else()
  message(WARNING "npdet_to_step will not be built")
endif()

# ------------------------------------
# npdet_to_teve
# ------------------------------------
if(TARGET ROOT::Eve)
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
    PUBLIC DD4hep::DDCore ROOT::Core ROOT::Eve ROOT::Imt)
  install(TARGETS ${exe_name}
    EXPORT NPDetTargets
    RUNTIME DESTINATION bin )
else()
  message(WARNING "npdet_to_teve will not be built")
endif()

# ------------------------------------
# npdet_mat_budget
# ------------------------------------
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
  PUBLIC DD4hep::DDCore DD4hep::DDRec ROOT::Core fmt::fmt)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )

# ------------------------------------
# dd_web_display
# ------------------------------------
if(TARGET ROOT::RHTTP)
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
    PUBLIC DD4hep::DDCore ROOT::Core ROOT::RHTTP ROOT::Net fmt::fmt spdlog::spdlog)
  install(TARGETS ${exe_name}
    EXPORT NPDetTargets
    RUNTIME DESTINATION bin )
else()
  message(WARNING "dd_web_display will not be built")
endif()

# ------------------------------------
# npdet_info
# ------------------------------------
set(exe_name npdet_info)
add_executable(${exe_name} src/${exe_name}.cxx )
target_include_directories(${exe_name}
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates
  PUBLIC cxx_std_17)
target_link_libraries(${exe_name}
  PUBLIC DD4hep::DDCore ROOT::Core ROOT::Hist ROOT::GenVector fmt::fmt)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )
