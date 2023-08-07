# Building Network Socket Terminal

To build NST from source code, you will need [xmake](https://xmake.io) and an up-to-date compiler.

All commands are run in the repository root.

## Configuration

The following commands will configure the build process. They do not have to be rerun on every build.

### Setting Debug Mode (Default)

```shell
xmake f -m debug
```

### Setting Release Mode

```shell
xmake f -m release
```

### Setting LDFLAGS (With Homebrew LLVM)

The following LDFLAGS will make brew-installed Clang use its own libc++:

```shell
xmake f --ldflags="-L$(brew --prefix)/opt/llvm/lib/c++ -Wl,-rpath,$(brew --prefix)/opt/llvm/lib/c++"
```

## Compiling

This project uses C++20 features. Compilation is officially supported with:

- GCC 13+
- Clang 16+ (with libc++ experimental library)
- Visual Studio 2022 17.5+

All code is standards compliant. However, because it uses recent C++ features, some compilers may not currently be supported.

```shell
xmake # Build project
xmake run terminal # Run app
```
