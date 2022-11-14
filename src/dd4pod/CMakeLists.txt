cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

configure_file(
    python/npsim.in
    ${CMAKE_CURRENT_BINARY_DIR}/npsim
    )
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/npsim"
  PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
  DESTINATION bin
  )
install(PROGRAMS
  python/npsim.py
  DESTINATION bin
  )
