cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# see https://rix0r.nl/blog/2015/08/13/cmake-guide/
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

find_package( DD4hep REQUIRED COMPONENTS DDCore DDG4 )

find_package(ROOT REQUIRED COMPONENTS Geom GenVector MathCore )
include(${ROOT_USE_FILE})

add_executable(npdet_fields src/npdet_fields.cxx)

target_include_directories(npdet_fields PUBLIC
    PRIVATE include
    )

target_compile_features(npdet_fields
    PUBLIC cxx_auto_type
    PUBLIC cxx_trailing_return_types
    PRIVATE cxx_variadic_templates
    )

target_link_libraries(npdet_fields PUBLIC DDCore DDG4 ROOT::Core ROOT::Hist ROOT::GenVector ROOT::Gpad)

install(TARGETS npdet_fields
    EXPORT NPDetTargets
    RUNTIME DESTINATION bin
    )


