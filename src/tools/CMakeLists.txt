cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# see https://rix0r.nl/blog/2015/08/13/cmake-guide/
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

find_package( DD4hep REQUIRED COMPONENTS DDCore DDG4 )

find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore)
include(${ROOT_USE_FILE})


set(exe_name npdet_fields)
add_executable(${exe_name} src/${exe_name}.cxx)
target_link_libraries(${exe_name}
  PUBLIC DD4hep::DDCore
  )

target_include_directories(${exe_name} PUBLIC
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Gpad)

install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )

message("COMPILE_LANGUAGE : ${COMPILE_LANGUAGE}"  )


# npdet_to_step needs ROOT's GeoCad library
if(TARGET ROOT::GeoCad)

  # get <experimental/filesystem> (will be there in c++17)
  add_library(stdc++fs UNKNOWN IMPORTED)
  set_property(TARGET stdc++fs PROPERTY IMPORTED_LOCATION "/usr/lib/gcc/x86_64-linux-gnu/7/libstdc++fs.a")

  set(exe_name npdet_to_step)
  add_executable(${exe_name} src/${exe_name}.cxx)
  target_include_directories(${exe_name} PUBLIC
    PRIVATE include )
  target_compile_features(${exe_name}
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PRIVATE cxx_variadic_templates 
    #PRIVATE cxx_std_17
    )
  target_link_libraries(${exe_name} 
    PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Gpad ROOT::GeoCad stdc++fs)
  install(TARGETS ${exe_name}
    EXPORT NPDetTargets
    RUNTIME DESTINATION bin )
endif(TARGET ROOT::GeoCad)


set(exe_name npdet_to_teve)
add_executable(${exe_name} src/${exe_name}.cxx)
#target_link_libraries(${exe_name}
#  PUBLIC DD4hep::DDG4
#  PUBLIC DD4hep::DDCore
#  )

target_include_directories(${exe_name} PUBLIC
  PRIVATE include )
target_compile_features(${exe_name}
  PUBLIC cxx_auto_type
  PUBLIC cxx_trailing_return_types
  PRIVATE cxx_variadic_templates 
  #PRIVATE cxx_std_17
  )
target_link_libraries(${exe_name} 
  PUBLIC DD4hep::DDCore DD4hep::DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Eve ROOT::Gpad stdc++fs ROOT::Imt)
install(TARGETS ${exe_name}
  EXPORT NPDetTargets
  RUNTIME DESTINATION bin )
