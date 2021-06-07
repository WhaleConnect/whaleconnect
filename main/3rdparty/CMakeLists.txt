# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the CMake script to build the third-party libraries contained within this directory.

include_directories(${CMAKE_CURRENT_LIST_DIR})

# Add Dear ImGui source files:
file(GLOB DEAR_IMGUI_SOURCES "imgui/*.cpp")
add_library(terminalLibs ${DEAR_IMGUI_SOURCES})
