# Building Network Socket Terminal

To build NST from source code, you will need CMake, Conan, Ninja, and an up-to-date compiler. MSVC is recommended on Windows, and GCC is recommended on Linux.

## Integration with Conan

NST uses [Conan](https://conan.io/) to manage dependencies. Follow its installation guide if it's not present on your system.

To install the required dependencies, run the following commands in the repository's root directory:

```shell
conan install . --build=missing -s build_type=Debug   # For a Debug build
conan install . --build=missing -s build_type=Release # For a Release build
```

If needed, specify a build profile in this step (such as `-pr:b=default`).

Conan will let you know if you need additional packages installed with your system's package manager.

## Required Linux Packages

You will need to install the following packages (if you're using a Debian-based system):

- `libbluetooth-dev` (BlueZ API)
- `libdbus-1-dev` (D-Bus API)

TODO: Add package names for pacman and dnf

## Compiling

This project uses C++23 features, so be sure to update your compiler(s) if needed. Compilation is officially supported on the following compilers:

- GCC 12.1+
- Visual Studio 2022 17.1+

All code is standards-compliant. However, because it uses a recent C++ revision, some compilers may not currently be supported.

### Build Commands with CMake

To build NST from the command line, start in the root of the repository and execute the commands below.

If you are building on Windows, run these commands in the Visual Studio command prompt.

```shell
cmake -S . --preset (debug|release)
cd build
cmake --build .
```

Alternatively, to build in one step with Conan:

```shell
conan build .
```

## Building Source Documentation

To build the documentation for the source code, you will need [Doxygen](https://www.doxygen.nl).

If it is installed, specify `-DBUILD_DOCS=ON` when configuring CMake. The documentation exports in HTML format in `build/src-docs`.
