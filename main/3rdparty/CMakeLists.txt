# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the CMake script to build the third-party libraries contained within this directory.

include_directories(${CMAKE_CURRENT_LIST_DIR})

include("../cmake/debug_macros.cmake")
include("../cmake/utf_8.cmake")

# Add Dear ImGui and {fmt} source files:
file(GLOB DEAR_IMGUI_SOURCES "imgui/*.cpp")
file(GLOB FMT_SOURCES "fmt/*.cc")
add_library(terminalLibs ${DEAR_IMGUI_SOURCES} ${FMT_SOURCES})

if (NOT WIN32)
    # Use pkg-config
	find_package(PkgConfig REQUIRED)

    # DBus
    pkg_search_module(DBUS REQUIRED dbus-1)
    target_include_directories(terminalLibs PUBLIC ${DBUS_INCLUDE_DIRS})

    # GLib
    pkg_search_module(GLIB REQUIRED glib-2.0)
    target_include_directories(terminalLibs PUBLIC ${GLIB_INCLUDE_DIRS})
    target_link_directories(terminalLibs PUBLIC ${GLIB_LIBRARY_DIRS})

    # GDBus
    file(GLOB GDBUS_SOURCES "gdbus/*.c")

    target_sources(terminalLibs PRIVATE ${GDBUS_SOURCES})
    target_link_libraries(terminalLibs ${DBUS_STATIC_LIBRARIES} ${GLIB_STATIC_LIBRARIES})
endif()
