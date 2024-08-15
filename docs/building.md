# Building WhaleConnect

To build WhaleConnect from source code, you will need [xmake](https://xmake.io) and an up-to-date compiler (see below).

All commands are run in the repository root.

## Tools

This project uses C++23 features and Swift-to-C++ interop on macOS. Recommended tools and versions are listed below for each operating system:

- **Windows:** MSVC 19.39 (Visual Studio 2022 17.9)
- **macOS:** Apple Clang 15, Swift 5.9 (Xcode 15.3)
- **Linux:** GCC 14

All code is standards compliant. However, because the project uses recent C++ features, other compilers, and versions older than those listed above, may not be supported.

## Configuring the Build

The arguments below are passed to `xmake f`/`xmake config` to configure the build process. Configuration should be done before compiling.

> [!NOTE]
> You should set all of your desired options in one config command. An invocation overwrites previously set configuration options.

When configuring for the first time, xmake will ask you to install libraries needed by WhaleConnect. Enter `y` to continue and install them.

If you are using the xmake VSCode extension, these arguments can be saved in the `xmake.additionalConfigArguments` setting.

### Build Mode

Use one of the following to set the build mode:

```text
-m debug   # For debug builds (default)
-m release # For release builds
```

## Compiling With xmake

```shell
xmake build swift # Build Swift code (macOS only)
xmake             # Build project
xmake run         # Run app
```

## Packaging

Creating a distributable package for WhaleConnect is done with `xmake i`/`xmake install`:

```shell
xmake i -o /path/to/package WhaleConnect
```

This command must be run after a successful compilation. xmake will copy files such as the built executable and all required shared libraries and download third-party licenses into the specified output directory.
