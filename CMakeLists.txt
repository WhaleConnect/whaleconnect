# Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the main CMake script to build Network Socket Terminal.

cmake_minimum_required(VERSION 3.20)

project(terminal)

# C++ language standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# Source files to compile
file(GLOB_RECURSE SRCS src/*.cpp)
add_executable(terminal WIN32 ${SRCS})
target_include_directories(terminal PRIVATE src)

# Libraries (installed with vcpkg)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
target_link_libraries(terminal PRIVATE imgui::imgui glfw fmt::fmt magic_enum::magic_enum)

# Copy GNU Unifont file next to built executable on post-build
add_custom_command(TARGET terminal POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/unifont/unifont-14.0.03.otf
    $<TARGET_FILE_DIR:terminal>/unifont.otf)

# Compiler-dependent options
if (MSVC)
    # Warnings
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS /W4)

    # UTF-8 charset
    target_compile_options(terminal PRIVATE /utf-8)

    # Use MSVC's Unicode character set and prevent clashing macros
    target_compile_definitions(terminal PRIVATE UNICODE _UNICODE NOMINMAX)
else()
    # Warnings
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS -Wall -Wextra -Wno-missing-field-initializers)

    # UTF-8
    target_compile_options(terminal PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

if (NOT WIN32)
    # DBus
    find_package(PkgConfig REQUIRED)
    pkg_search_module(DBUS REQUIRED dbus-1)
    target_include_directories(terminal PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_directories(terminal PRIVATE ${DBUS_LIBRARY_DIRS})

    # POSIX threads
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    # Link to libraries
    target_link_libraries(terminal ${DBUS_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} -lbluetooth)
endif()

# Build source documentation with Doxygen in release builds
find_package(Doxygen)
option(BUILD_DOCS "Build documentation for sources" OFF)
if (DOXYGEN_FOUND AND BUILD_DOCS)
    set(DOXYGEN_PROJECT_NAME "Network Socket Terminal")
    set(DOXYGEN_PROJECT_BRIEF "Cross-platform network communication software")
    set(DOXYGEN_STRIP_FROM_PATH src)
    set(DOXYGEN_STRIP_FROM_INC_PATH src)

    set(DOXYGEN_FORCE_LOCAL_INCLUDES YES)
    set(DOXYGEN_SORT_MEMBERS_CTORS_1ST YES)
    set(DOXYGEN_SORT_GROUP_NAMES YES)

    set(DOXYGEN_WARN_NO_PARAMDOC YES)

    set(DOXYGEN_HTML_OUTPUT src-docs)
    set(DOXYGEN_GENERATE_TREEVIEW YES)

    doxygen_add_docs(docs ALL
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        COMMENT "Generating HTML source documentation")
endif()
