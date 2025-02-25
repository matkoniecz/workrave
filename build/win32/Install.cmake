message(STATUS "Resolving dependencies. This may take a while: ${SYS_ROOT}/bin;${CMAKE_INSTALL_PREFIX}/lib")
list(APPEND CMAKE_MODULE_PATH "${MODULE_PATH}")
include(Win32ResolveDependencies)

resolve_dependencies("${CMAKE_BINARY_DIR}/libs/hooks/harpoonHelper/src/WorkraveHelper.exe" dependencies resolved_dependencies "${SYS_ROOT}/bin;${CMAKE_INSTALL_PREFIX}/lib")

foreach(dependency ${resolved_dependencies})
  get_filename_component(file ${dependency} NAME)
  file (INSTALL ${dependency} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib32")
endforeach()
