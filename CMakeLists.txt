# Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the main CMake script to build Network Socket Terminal.

cmake_minimum_required(VERSION 3.20)

# Toolchain file for vcpkg
if (DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
else()
    message(FATAL_ERROR "VCPKG_ROOT environment variable not set. See docs/BUILDING.md for details.")
endif()

project(terminal)

# Generate compilation database for SonarLint
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ language standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# Source files to compile
add_executable(terminal WIN32
    "src/main.cpp"
    "src/app/mainhandle.cpp"
    "src/async/async.cpp"
    "src/core/newconnbt.cpp"
    "src/core/newconnip.cpp"
    "src/gui/connwindow.cpp"
    
    "src/gui/sdpwindow.cpp"
    "src/gui/console.cpp"
    "src/net/btutils.cpp"
    "src/net/sockets.cpp"
    "src/sys/error.cpp"
    "src/sys/filesystem.cpp"
    "src/util/imguiext.cpp"
    "src/util/strings.cpp")

target_include_directories(terminal PRIVATE src)

# Libraries (installed with vcpkg)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
target_link_libraries(terminal PRIVATE imgui::imgui glfw fmt::fmt magic_enum::magic_enum)

# Copy GNU Unifont file next to built executable on post-build
add_custom_command(TARGET terminal POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/unifont/unifont-14.0.04.otf
    $<TARGET_FILE_DIR:terminal>/unifont.otf)

# Compiler-dependent options
if (MSVC)
    # Warnings
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS /W4)

    target_compile_options(terminal PRIVATE
        /utf-8                  # UTF-8 charset
        /external:anglebrackets # Specify which headers are installed libraries
        /external:W0)           # Suppress warnings in libraries

    # Use MSVC's Unicode character set and prevent clashing macros
    target_compile_definitions(terminal PRIVATE UNICODE _UNICODE NOMINMAX)

    # Windows API static libraries
    target_link_libraries(terminal PRIVATE Ws2_32 Bthprops)
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
option(BUILD_DOCS "Build documentation for sources" OFF)
if (BUILD_DOCS)
    find_package(Doxygen)

    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is required to build source documentation")
    endif()

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
