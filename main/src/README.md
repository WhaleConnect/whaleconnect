# Network Socket Terminal Source

## The Source Files

This directory contains the C++ source code for Network Socket Terminal. A description of each file is given below.\
Each entry below has both a \*.cpp file and an \*.hpp file unless either extension is explicitly given.

Name | Short for | Contains
--- | --- | ---
`error` | - | Facilities to convert error codes to descriptions, used with `errorlist.cpp`
`errorlist.cpp`\* | - | A lookup table which maps numeric error codes to their readable descriptions
`formatcompat.hpp` | Format Compatibility | Polyfill for `std::format` with {fmt} for non-C++20 complete compilers
`imguiext` | Dear ImGui Extensions | Functions to extend and add on to Dear ImGui
`main.cpp` | - | Implementation of the entry point and core application logic
`mainhandle` | Main Handler | Main window handling functions (e.g. create/destroy, render)
`searchbt` | Search Bluetooth | Bluetooth searching function to discover nearby devices
`sockets` | - | High-level, cross-platform socket functions
`uicomp` | User Interface Components | Reusable classes to represent frequently-used GUI elements
`util.hpp` | Utilities | Definitions for anything used by multiple files
`winutf8` | Windows UTF-8 | Helpers to convert strings between UTF-8/UTF-16 on Windows

\*: This file is *automatically generated*, do not edit it. Instead, edit the files in the `error_gen` directory. See its README for details.
