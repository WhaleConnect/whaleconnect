# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the CMake script to provide debug-mode macros when not on MSVC.

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(DEBUG _DEBUG)
    else()
        add_compile_definitions(NDEBUG)
    endif()
endif()
