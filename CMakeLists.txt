cmake_minimum_required (VERSION 2.8.11)
project(Monero-Gui)

set(Monero-Gui_VERSION_MAJOR 1)
set(Monero-Gui_VERSION_MINOR 0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

#if(NOT MANUAL_SUBMODULES)
#  find_package(Git)
#  if(GIT_FOUND)
#    function (check_submodule relative_path)
#      execute_process(COMMAND git rev-parse "HEAD" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${relative_path} OUTPUT_VARIABLE localHead)
#      execute_process(COMMAND git rev-parse "HEAD:${relative_path}" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE checkedHead)
#      string(COMPARE EQUAL "${localHead}" "${checkedHead}" upToDate)
#      if (upToDate)
#        message(STATUS "Submodule '${relative_path}' is up-to-date")
#      else()
#        message(FATAL_ERROR "Submodule '${relative_path}' is not up-to-date. Please update with\ngit submodule update --init --force ${relative_path}\nor run cmake with -DMANUAL_SUBMODULES=1")
#      endif()
#    endfunction ()
#
#    message(STATUS "Checking submodules")
#    check_submodule(monero)
#  endif()
#endif()

find_package(Qt5Core)
get_target_property(QtCore_location Qt5::Core Location)
#set_target_property(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

find_package(Qt5Network)
get_target_property(QtNetwork_location Qt5::Network Location)
#set_target_property(Qt5::Network PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

find_package(Qt5Gui)
get_target_property(QtGui_location Qt5::Gui Location)
#set_target_property(Qt5::Gui PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

find_package(Qt5Widgets)
get_target_property(QtWidgets_location Qt5::Widgets Location)
#set_target_property(Qt5::Widgets PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

foreach(plugin ${Qt5Network_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  message("Plugin ${plugin} is at location ${_loc}")
endforeach()

foreach(plugin ${Qt5Gui_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  message("Plugin ${plugin} is at location ${_loc}")
endforeach()

foreach(plugin ${Qt5Widgets_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  message("Plugin ${plugin} is at location ${_loc}")
endforeach()

add_subdirectory(components)
add_subdirectory(fonts)
add_subdirectory(images)
add_subdirectory(js)
add_subdirectory(monero)
add_subdirectory(pages)
add_subdirectory(src)
add_subdirectory(tabs)
add_subdirectory(translations)
add_subdirectory(wizard)





