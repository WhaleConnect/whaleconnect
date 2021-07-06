# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the main CMake script to build Network Socket Terminal.

cmake_minimum_required(VERSION 3.17)

project(terminal VERSION 1.0)

# C++ language standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json under build directory
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# source files to compile
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(terminal WIN32 ${SOURCES})

# Bundled libraries location
add_subdirectory(3rdparty)
include_directories(3rdparty)

# Link bundled third-party libraries
target_link_libraries(terminal terminalLibs)

# Compiler-dependent options
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# MSVC options
	target_compile_options(terminal PRIVATE /W4 /utf-8)

    # Use MSVC's Unicode character set
    target_compile_definitions(terminal PRIVATE UNICODE _UNICODE)

	# Windows - GLFW lib file
	target_link_libraries(terminal ${PROJECT_SOURCE_DIR}/3rdparty/GLFW/glfw3.lib)

	# This prevents LNK4098 in Debug mode, but causes LNK2001 in Release mode.
	# This is because the GLFW pre-built binaries were built in Release mode.
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_options(terminal PRIVATE /NODEFAULTLIB:msvcrt.lib)
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# GCC options
	target_compile_options(terminal PRIVATE -Wall -Wextra)
endif()

if (NOT WIN32)
	# Unix - GLFW should be installed via your package manager, use PackageConfig to find and configure it.
	find_package(PkgConfig REQUIRED)
	pkg_search_module(GLFW REQUIRED glfw3)
	include_directories(${GLFW_INCLUDE_DIRS})

	# POSIX threads
	set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
	set(THREADS_PREFER_PTHREAD_FLAG TRUE)
	find_package(Threads REQUIRED)

	# Link libraries (GLFW, POSIX threads, libdl (dynamic library management), BlueZ)
	target_link_libraries(terminal ${GLFW_STATIC_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} -lbluetooth)
endif()
