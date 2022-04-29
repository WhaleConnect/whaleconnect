# Building Network Socket Terminal

To build NST from source code, you will need CMake and an up-to-date compiler. MSVC is recommended on Windows, and GCC is recommended on Linux.

## Required Linux Packages

You will need to install the following packages (if you're using a Debian-based system):

```shell
sudo apt install libbluetooth-dev # BlueZ API
sudo apt install libdbus-1-dev    # D-Bus API
sudo apt install libgl-dev        # OpenGL API
sudo apt install libglfw3-dev     # GLFW 3 API
```

TODO: Add install commands for pacman and dnf

## Compiling

This project uses C++23 features, so be sure to update your compiler(s) if needed. Compilation has been tested successfully with the following compiler versions:

- GCC 12.1+
- Visual Studio 2022 17.1+

All code is standards-compliant. However, because it uses a recent C++ revision, some compilers may not be supported at the moment.

### Build Commands with CMake

```shell
cd core
mkdir build
cd build
cmake ..
cmake --build .
```
