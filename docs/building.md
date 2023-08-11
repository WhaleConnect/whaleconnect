# Building Network Socket Terminal

To build NST from source code, you will need [xmake](https://xmake.io) and an up-to-date compiler (see below).

All commands are run in the repository root.

## Supported Compilers

This project uses C++20 features. Compilation is officially supported with:

- GCC 13+
- Clang 16+ (if using libc++, enable the experimental library)
- Visual Studio 2022 17.5+

All code is standards compliant. However, because the project uses recent C++ features, other compilers, and versions older than those listed above, are not supported.

## Configuring the Build

The arguments below are passed to `xmake f` to configure the build process. Configuration should be done before compiling.

**Note:** You should set all of your desired options in one `xmake f` command. An `xmake f` invocation overwrites previously set configuration options.

When configuring for the first time, xmake will ask you to install libraries needed by NST. Enter `y` to continue and install them.

If you are using the xmake VSCode extension, these arguments can be saved in the `xmake.additionalConfigArguments` setting.

### Build Mode

```text
-m debug   # For debug builds
-m release # For release builds
```

### Toolchain

Xmake defaults to using MSVC on Windows, GCC on Linux, and Xcode on macOS. If your platform/compiler combination is different, run one of the following:

**Note:** Xcode/Apple Clang are not supported. If you are on macOS, you should install GCC or Clang and run the respective command below.

```text
--toolchain=msvc  # Use MSVC
--toolchain=gcc   # Use GCC
--toolchain=clang # Use Clang
```

### Alternative Compiler Installation

If you have installed a different version of your selected compiler than the system default and would like to use it, the following arguments allow xmake to locate it:

```text
--cc=/path/to/c-compiler --cxx=/path/to/c++-compiler # For C/C++ - duplicate with --mm/--mxx for Objective-C/C++ (macOS only)
```

### Alternative Standard Library Installation (GCC/Clang)

If you need to use an alternative C++ standard library (e.g., you want to use a newer one bundled with an alternative compiler), set the C++ build and link flags accordingly:

```text
--cxxflags="-I/path/to/library/headers -nostdinc++"                          # Compile flags - duplicate with --mxxflags for Objective-C/C++ (macOS only)
--ldflags="-L/path/to/library/archives -Wl,-rpath,/path/to/library/archives" # Link flags
```

### Using libc++ (Clang)

Many parts of the C++20 standard library require a separate flag in libc++ to be enabled. Add the following flags to `cxxflags` (and `mxxflags` if needed) and `ldflags`:

```text
-stdlib=libc++ -fexperimental-library
```

## Compiling With xmake

```shell
xmake     # Build project
xmake run # Run app
```
