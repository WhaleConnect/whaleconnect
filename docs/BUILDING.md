# Building Network Socket Terminal

To build NST from source code, you will need [XMake](https://xmake.io) and an up-to-date compiler.

All commands will need to be run in the repository's root directory.

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

This project uses C++23 features, so be sure to update your compiler(s) if needed. Compilation is officially supported on the following compilers:

- Clang 15+ (with experimental library)
- Visual Studio 2022 17.1+

All code is standards-compliant. However, because it uses a recent C++ revision, some compilers may not currently be supported.

### Building from the Command Line

```shell
xmake
```

### Running the Executable

```shell
xmake run terminal
```
