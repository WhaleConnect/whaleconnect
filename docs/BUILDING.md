# Building Network Socket Terminal

## Supported Tools

NST can be built with:

- Standalone Visual Studio or MSBuild (Windows)
- MSVC with CMake (Windows)
- GCC or Clang with CMake (Linux)

## Software

To build NST from the sources in this repository, you will need the following software:

### Windows

- [Visual Studio 2019](https://visualstudio.microsoft.com/) (Requires the C++ Build Tools - the IDE is optional)
- [CMake](https://cmake.org/) (Optional)

### Linux

- [GCC](https://gcc.gnu.org/) or [Clang](https://clang.llvm.org/)
- [CMake](https://cmake.org/)

### VSCode extensions

- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

### Required Linux Packages

You will need to install the following packages (if you're using a Debian-based system):

```shell
sudo apt install libbluetooth-dev # BlueZ API
sudo apt install libdbus1-dev     # D-Bus API
sudo apt install libgl-dev        # OpenGL API
sudo apt install libglfw3-dev     # GLFW 3 API
```

TODO: Add install commands for Fedora and similar distros

## Compiling

This project uses C++20 features, so be sure to update your compiler(s) if needed. Compilation has been tested successfully with the following compiler versions:

- GCC 11.2.0
- MSVC 19.29

*Clang is currently not supported because it does not yet implement P0960R3 (Parenthesized initialization of aggregates) used in `std::vector::emplace_back()`.*

### Command Line

#### With MSBuild (Windows)

```shell
cd core
msbuild
```

#### With CMake (All Platforms)

```shell
cd core
mkdir build
cd build
cmake ..
cmake --build .
```

### IDE/Editor

#### With Visual Studio (Windows)

Open `terminal.sln`.

#### With VSCode+CMake (All Platforms)

1. Open the base path of the repository in VSCode.
2. The CMake Tools extension should automatically start the configure process. If not, invoke it from the Command Palette (Ctrl+Shift+P).

Then, select "Start Debugging" or "Start without Debugging" (F5 or Ctrl+F5 respectively). In VS, these are under the "Debug" menu; in VSCode, these are under the "Run" menu.
