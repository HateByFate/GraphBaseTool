# Generated by BoostInstall.cmake for boost_logic-1.88.0

if(Boost_VERBOSE OR Boost_DEBUG)
  message(STATUS "Found boost_logic ${boost_logic_VERSION} at ${boost_logic_DIR}")
endif()

include(CMakeFindDependencyMacro)

if(NOT boost_config_FOUND)
  find_dependency(boost_config 1.88.0 EXACT HINTS "${CMAKE_CURRENT_LIST_DIR}/..")
endif()
if(NOT boost_core_FOUND)
  find_dependency(boost_core 1.88.0 EXACT HINTS "${CMAKE_CURRENT_LIST_DIR}/..")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/boost_logic-targets.cmake")
