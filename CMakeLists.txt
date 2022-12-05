# Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the main CMake script to build Network Socket Terminal.

cmake_minimum_required(VERSION 3.20)

project(terminal)

# Generate compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection
# TODO: Remove in CMake 3.25
if (UNIX AND NOT APPLE)
    set(LINUX 1 CACHE INTERNAL "")
else()
    set(LINUX 0 CACHE INTERNAL "")
endif()

# Source files to compile
set(SRCS
    "src/main.cpp"
    "src/gui/app.cpp"
    "src/gui/connwindow.cpp"
    "src/gui/console.cpp"
    "src/gui/imguiext.cpp"
    "src/gui/newconnbt.cpp"
    "src/gui/newconnip.cpp"
    "src/gui/notifications.cpp"
    "src/gui/sdpwindow.cpp"
    "src/os/async.cpp"
    "src/os/error.cpp"
    "src/sockets/clientsocket.cpp"
    "src/utils/strings.cpp")

if (WIN32)
    list(APPEND SRCS
        "src/os/async_windows.cpp"
        "src/os/btutils_windows.cpp"
        "src/sockets/clientsocket_windows.cpp"
        "src/sockets/socket_windows.cpp")
elseif (APPLE)
    list(APPEND SRCS
        "src/os/async_macos.cpp"
        "src/os/btutils_macos.mm"
        "src/sockets/clientsocket_macos.mm"
        "src/sockets/socket_macos.mm")
elseif (LINUX)
    list(APPEND SRCS
        "src/os/async_linux.cpp"
        "src/os/btutils_linux.cpp"
        "src/sockets/clientsocket_linux.cpp"
        "src/sockets/socket_linux.cpp")
endif()

add_executable(terminal WIN32 ${SRCS})
set_target_properties(terminal
    PROPERTIES
    CXX_STANDARD 23
    CXX_EXTENSIONS OFF)

target_include_directories(terminal PRIVATE src conan_installed/backends)

# Libraries (installed with Conan)
find_package(imgui CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

find_package(liburing CONFIG)
find_package(out_ptr CONFIG)

# OpenGL
find_package(OpenGL REQUIRED)

# Dear ImGui backend files
add_library(imguiBackends STATIC
    "conan_installed/backends/imgui_impl_sdl.cpp"
    "conan_installed/backends/imgui_impl_opengl3.cpp")

target_link_libraries(imguiBackends PUBLIC imgui::imgui SDL2::SDL2)
target_link_libraries(terminal PRIVATE imguiBackends magic_enum::magic_enum SDL2::SDL2main OpenGL::GL)

# Check for additional package presence by looking for their version strings
if (DEFINED liburing_VERSION_STRING)
    target_link_libraries(terminal PRIVATE liburing::liburing)
endif()

if (DEFINED out_ptr_VERSION_STRING)
    target_link_libraries(terminal PRIVATE out_ptr::out_ptr)
endif()

# Copy GNU Unifont file next to built executable on post-build
add_custom_command(TARGET terminal POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/unifont/unifont-15.0.01.otf
    $<TARGET_FILE_DIR:terminal>/unifont.otf)

target_compile_definitions(terminal PRIVATE
    OS_WINDOWS=$<BOOL:${WIN32}>
    OS_APPLE=$<BOOL:${APPLE}>
    OS_LINUX=$<BOOL:${LINUX}>)

# Compiler-dependent options
if (MSVC)
    # Warnings
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS /W4)

    target_compile_options(terminal PRIVATE
        /EHsc                   # Exception handling model
        /utf-8                  # UTF-8 charset
        /external:anglebrackets # Specify which headers are installed libraries
        /external:W0)           # Suppress warnings in libraries

    # Use MSVC's Unicode character set and prevent clashing macros
    target_compile_definitions(terminal PRIVATE UNICODE _UNICODE NOMINMAX)
else()
    # Warnings
    set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS -Wall -Wextra -Wno-missing-field-initializers)

    target_compile_options(terminal PRIVATE
        -finput-charset=UTF-8   # UTF-8 for input charset
        -fexec-charset=UTF-8    # UTF-8 for execution charset
        -fexperimental-library) # Enable Clang experimental library

    target_link_options(terminal PRIVATE -fexperimental-library)
endif()

if (WIN32)
    # Windows API static libraries
    target_link_libraries(terminal PRIVATE Ws2_32 Bthprops)
elseif (APPLE)
    # Objective-C frameworks
    target_link_libraries(terminal PRIVATE "-framework Foundation" "-framework IOBluetooth")

    # Objective-C ARC
    target_compile_options(terminal PRIVATE "-fobjc-arc")
elseif (LINUX)
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
    target_link_libraries(terminal PRIVATE ${DBUS_LIBRARIES} Threads::Threads -lbluetooth)
endif()
