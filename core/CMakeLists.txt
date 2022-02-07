# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the main CMake script to build Network Socket Terminal.

cmake_minimum_required(VERSION 3.17)

project(terminal VERSION 1.0)

include("cmake/utf_8.cmake")

# C++ language standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json under build directory
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# Source files to compile
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(terminal WIN32 ${SOURCES})

# Bundled libraries location
add_subdirectory(lib)
target_include_directories(terminal PRIVATE lib src)

# Link third-party library sources
target_link_libraries(terminal terminalLibs)

# Compiler-dependent options
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC options
    set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS "/W4")

    # Use MSVC's Unicode character set
    target_compile_definitions(terminal PRIVATE UNICODE _UNICODE)

    # Windows - GLFW lib file
    target_link_libraries(terminal ${PROJECT_SOURCE_DIR}/lib/GLFW/glfw3.lib)

    # This prevents LNK4098 in Debug mode, but causes LNK2001 in Release mode.
    # This is because the GLFW pre-built binaries were built in Release mode.
    # Look for the "Rel" substring indicating a release build.
    if(NOT CMAKE_BUILD_TYPE MATCHES "Rel")
        target_link_options(terminal PRIVATE /NODEFAULTLIB:MSVCRT)
    endif()
else()
    # GCC/clang options
    set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Wno-missing-field-initializers")
endif()

if (NOT WIN32)
    # Use pkg-config
    find_package(PkgConfig REQUIRED)

    # GLFW
    pkg_search_module(GLFW REQUIRED glfw3)
    include_directories(${GLFW_INCLUDE_DIRS})

    # DBus
    pkg_search_module(DBUS REQUIRED dbus-1)
    target_include_directories(terminal PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_directories(terminal PRIVATE ${DBUS_LIBRARY_DIRS})

    # POSIX threads
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    # Link to libraries
    target_link_libraries(terminal ${GLFW_STATIC_LIBRARIES} ${DBUS_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} -lbluetooth)
endif()
