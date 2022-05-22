# Building Network Socket Terminal

To build NST from source code, you will need CMake, vcpkg, Ninja (if building in VS or VSCode), and an up-to-date compiler. MSVC is recommended on Windows, and GCC is recommended on Linux.

## Integration with vcpkg

NST uses [vcpkg](https://github.com/microsoft/vcpkg) to manage dependencies. Follow its installation guide if it's not present on your system.

Then, create an enviroment variable called `VCPKG_ROOT` and point it to your vcpkg install path (for example, `C:\dev\vcpkg`). This allows the CMake script to know where vcpkg is installed.

When CMake configures the project, vcpkg should automatically download the required packages.

## Required Linux Packages

You will need to install the following packages (if you're using a Debian-based system):

- `libbluetooth-dev` (BlueZ API)
- `libdbus-1-dev` (D-Bus API)
- `libgl-dev` (OpenGL API)

TODO: Add package names for pacman and dnf

## Compiling

This project uses C++23 features, so be sure to update your compiler(s) if needed. Compilation has been tested successfully with the following compilers:

- GCC 12.1+
- Visual Studio 2022 17.1+

All code is standards-compliant. However, because it uses a recent C++ revision, some compilers may not currently be supported.

### Build Commands with CMake

To build NST from the command line, start in the root of the repository and execute the commands below.

If you are building on Windows and are not using the default Visual Studio generator, run these commands in the Visual Studio command prompt.

**Note**: Specify additional configuration options as needed in the `cmake ..` step, such as the build type (`-DCMAKE_BUILD_TYPE=(Debug|Release)`) and generator (`-G (generator name)`).

```shell
mkdir build
cd build
cmake ..
cmake --build .
```

## Building Source Documentation

To build the documentation for the source code, you will need [Doxygen](https://www.doxygen.nl).

If it is installed, specify `-DBUILD_DOCS=ON` when configuring CMake. The documentation exports in HTML format in `build/<config>/src-docs`.
