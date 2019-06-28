cmake_minimum_required(VERSION 3.10)
project(monero-gui)
#project(monero-gui C CXX)

message(STATUS "Initiating compile using CMake ${CMAKE_VERSION}")
#include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

set(STATIC ON)
set(LINUX ON)
set(DEBUG ON)
set(CMAKE_BUILD_TYPE Release)

# GUI
set(VERSION_MAJOR "14")
set(VERSION_MINOR "0")
set(VERSION_REVISION "3")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")

set(USE_DEVICE_TREZOR OFF)
option(USE_DEVICE_TREZOR "Disable zxcvbn" OFF)
option(DISABLE_PASS_STRENGTH_METER "Disable zxcvbn" ON)
#option(WITH_SCANNER "Enable webcam QR scanner" OFF)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")
#set(CMAKE_CXX_FLAGS "-fPIC")
#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_INCLUDE_CURRENT_DIR ON) WTF - this is a very dangerous thing to do
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
if(DEBUG)
	set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

#set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt5.9/5.9.7/gcc_64)
set(BUILD_GUI_DEPS ON)
set(STATIC ON)
set(ARCH "x86-64")
set(BUILD_64 ON)
set(INSTALL_VENDORED_LIBUNBOUND=ON)
add_subdirectory(monero)
#add_custom_target(monero COMMAND echo "monero already built")
#execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_libwallet_api.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#ExternalProject_Add(SOURCE_DIR monero)
#set_property(TARGET monero PROPERTY FOLDER "monero")
add_library(MONERO_WALLET_LIBRARY STATIC monero)
#set(MONERO_WALLET_LIBRARY "${CMAKE_BINARY_DIR}/monero/lib/libwallet_merged.a")
#set(MONERO_WALLET_HEADERS "${CMAKE_SOURCE_DIR}/monero/src")

if(STATIC)
	message(STATUS "Initiating static build")
	set(Boost_USE_STATIC_LIBS ON)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Include password strength library
add_definitions(-DDISABLE_PASS_STRENGTH_METER)
#if(DISABLE_PASS_STRENGTH_METER)
#    add_definitions(-DDISABLE_PASS_STRENGTH_METER)
#else()
#    message(STATUS "1111")
#    list(APPEND SOURCE_FILES
#        "src/zxcvbn-c/*.h"
#        "src/zxcvbn-c/*.cpp"
#        )
#endif()

#include(CheckTrezor)  # Trezor support check
include(CMakePackageConfigHelpers)

# QT5
#set(RESOURCES
#    "resources/images.qrc"
#    )

# force version update

function (monero_gui_add_library_with_deps)
  cmake_parse_arguments(MONERO_ADD_LIBRARY "" "NAME" "DEPENDS;SOURCES" ${ARGN})
  source_group("${MONERO_ADD_LIBRARY_NAME}" FILES ${MONERO_ADD_LIBRARY_SOURCES})

  # Define a ("virtual") object library and an actual library that links those
  # objects together. The virtual libraries can be arbitrarily combined to link
  # any subset of objects into one library archive. This is used for releasing
  # libwallet, which combines multiple components.
  set(objlib obj_${MONERO_ADD_LIBRARY_NAME})
  add_library(${objlib} OBJECT ${MONERO_ADD_LIBRARY_SOURCES})
  add_library("${MONERO_ADD_LIBRARY_NAME}" $<TARGET_OBJECTS:${objlib}>)
  if (MONERO_ADD_LIBRARY_DEPENDS)
    add_dependencies(${objlib} ${MONERO_ADD_LIBRARY_DEPENDS})
  endif()
  set_property(TARGET "${MONERO_ADD_LIBRARY_NAME}" PROPERTY FOLDER "libs")
  target_compile_definitions(${objlib}
    PRIVATE $<TARGET_PROPERTY:${MONERO_ADD_LIBRARY_NAME},INTERFACE_COMPILE_DEFINITIONS>)
endfunction ()

function (monero_gui_add_library name)
    monero_gui_add_library_with_deps(NAME "${name}" SOURCES ${ARGN})
endfunction()

include(VersionGui)
#set_property(SOURCE version.h PROPERTY SKIP_AUTOMOC ON)
#set_property(SOURCE version.cpp.in PROPERTY SKIP_AUTOMOC ON)
#set_property(SOURCE version.cpp PROPERTY SKIP_AUTOMOC ON)
monero_gui_add_library(gui_version SOURCES version.js DEPENDS genversiongui)

find_package(Qt5Core REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Qml REQUIRED)
message(STATUS "${CMAKE_MODULE_PATH}")
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Qt5Svg REQUIRED)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# OpenGL
#set(OpenGL_GL_PREFERENCE)
#find_package(OpenGL)
#target_include_directories(monero-gui PUBLIC ${OPENGL_INCLUDE_DIR})
#message(STATUS "OpenGL: include dir at ${OPENGL_INCLUDE_DIR}")
#message(STATUS "OpenGL: libraries at ${OPENGL_LIBRARIES}")

# OpenSSL
find_package(OpenSSL REQUIRED)
#target_include_directories(monero-gui PUBLIC ${OPENSSL_INCLUDE_DIR})
message(STATUS "OpenSSL: Version ${OPENSSL_VERSION}")
message(STATUS "OpenSSL: include dir at ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL: libraries at ${OPENSSL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}")

# Zbar (for QR scanner)
#find_package(ZBar0)
#target_include_directories(monero-gui PUBLIC ${ZBAR_INCLUDE_DIR})
#message(STATUS "libzbar: include dir at ${ZBAR_INCLUDE_DIR}")
#message(STATUS "libzbar: libraries at ${ZBAR_LIBRARIES}")

# Unwind
find_package(Libunwind)
#target_include_directories(monero-gui PUBLIC ${LIBUNWIND_INCLUDE_DIR})
message(STATUS "libunwind: include dir at ${LIBUNWIND_INCLUDE_DIR}")
message(STATUS "libunwind: libraries at ${LIBUNWIND_LIBRARIES}")

# Sodium
#if(STATIC)
#	set(sodium_USE_STATIC_LIBS ON)
#endif()
#find_package(Sodium REQUIRED)
#target_include_directories(monero-gui PUBLIC ${sodium_INCLUDE_DIR})
#message(STATUS "libsodium: include dir at ${sodium_INCLUDE_DIR}")
#message(STATUS "libsodium: libraries at ${sodium_LIBRARY_DEBUG}")
#if(sodium_VERSION)
#	message(STATUS "libsodium: Version ${sodium_VERSION}")
#endif()
find_library(SODIUM_LIBRARY sodium)

# LibUSB
find_package(LibUSB)
#target_include_directories(monero-gui PUBLIC ${LibUSB_INCLUDE_DIRS})
message(STATUS "libusb: include dir at ${LibUSB_INCLUDE_DIRS}")
message(STATUS "libusb: libraries at ${LibUSB_LIBRARIES}")

# HIDApi
find_package(HIDAPI REQUIRED)
#target_include_directories(monero-gui PUBLIC ${HIDAPI_INCLUDE_DIRS})
message(STATUS "libhidapi: include dir at ${HIDAPI_INCLUDE_DIRS}")
message(STATUS "libhidapi: libraries at ${HIDAPI_LIBRARIES}")

# Boost
#if(DEBUG)
#    set(Boost_DEBUG ON)
#endif()
find_package(Boost 1.62 REQUIRED COMPONENTS
		system
		filesystem
		thread
		date_time
		chrono
		regex
		serialization
		program_options
		locale)

if(LINUX)
find_package(X11 REQUIRED)
message(STATUS "X11_FOUND = ${X11_FOUND}")
message(STATUS "X11_INCLUDE_DIR = ${X11_INCLUDE_DIR}")
message(STATUS "X11_LIBRARIES = ${X11_LIBRARIES}")
#include_directories(${X11_INCLUDE_DIR})
#link_directories(${X11_LIBRARIES})
endif()

#target_include_directories(monero-gui PUBLIC ${Boost_INCLUDE_DIRS})
message(STATUS "Using Boost include dir at ${Boost_INCLUDE_DIRS}")
message(STATUS "Using Boost libraries at ${Boost_LIBRARIES}")

add_subdirectory(src)

