# Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the main CMake script to build Network Socket Terminal.

cmake_minimum_required(VERSION 3.17)

project(terminal)

# C++ language standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# Source files to compile
file(GLOB_RECURSE SRCS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(terminal WIN32 ${SRCS})
target_include_directories(terminal PRIVATE src)

# Libraries
add_subdirectory(lib)
target_include_directories(terminal PRIVATE lib)
target_link_libraries(terminal terminalLibs)

# Copy GNU Unifont file next to built executable on post-build
add_custom_command(TARGET terminal POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/lib/unifont/unifont-14.0.03.otf
    $<TARGET_FILE_DIR:terminal>/unifont.otf)

# Compiler-dependent options
if (MSVC)
    # Warning level
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS "/W4")

    # UTF-8 charset (for libraries and app code)
    target_compile_options(terminal PRIVATE "/utf-8")

    # Enable Edit and Continue in debug builds
    target_link_options(terminal PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL>)
    target_compile_options(terminal PRIVATE $<$<CONFIG:Debug>:/ZI>)

    # Use MSVC's Unicode character set and prevent clashing macros
    target_compile_definitions(terminal PRIVATE UNICODE _UNICODE NOMINMAX)

    # GLFW lib file
    target_link_libraries(terminal ${PROJECT_SOURCE_DIR}/lib/GLFW/glfw3.lib)
else()
    # Warnings
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Wno-missing-field-initializers")

    # UTF-8
    target_compile_options(terminal PRIVATE "-finput-charset=UTF-8 -fexec-charset=UTF-8")
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
