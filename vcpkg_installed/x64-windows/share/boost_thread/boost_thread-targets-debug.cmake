#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Boost::thread" for configuration "Debug"
set_property(TARGET Boost::thread APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Boost::thread PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/boost_thread-vc143-mt-gd-x64-1_88.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/boost_thread-vc143-mt-gd-x64-1_88.dll"
  )

list(APPEND _cmake_import_check_targets Boost::thread )
list(APPEND _cmake_import_check_files_for_Boost::thread "${_IMPORT_PREFIX}/debug/lib/boost_thread-vc143-mt-gd-x64-1_88.lib" "${_IMPORT_PREFIX}/debug/bin/boost_thread-vc143-mt-gd-x64-1_88.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
