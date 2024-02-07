# Module Files

This directory contains data files to generate module files from external library headers. Subdirectories are as follows:

- `common`: Libraries used on all platforms
- `linux`, `macos`, `windows`: Libraries used on Linux, macOS, and Windows, respectively

## Data Format

Modules are expressed in a minimal data format that is smaller and easier to maintain than raw `.mpp` files. These files are contained within the directories listed above, have the `.txt` extension, and are converted to C++ modules with a Python generator script.

This data format uses significant whitespace and must be indented with four spaces. Indentation is used to contain data within blocks. Each block is listed below:

### Defines

Contains macros that are defined before including any headers for configuration. The value of a macro may optionally be specified after its name, separated with a space.


```text
defines
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
```

### Includes

Contains the headers to be included. Each header is included with angle brackets (`<filename>`) in the resultant module file.

```text
includes
    imgui.h
    imgui_internal.h
    imgui_impl_opengl3.h
    imgui_impl_sdl3.h
```

### Constants

Contains a list of macros that are defined by the included headers and should be exported by the module. The type and storage specifiers of a constant may be optionally specified after its name, separated with a colon (by default `constexpr auto`).

```text
constants
    INFINITE
    INVALID_HANDLE_VALUE:const HANDLE
```

Macros are exported as variables with the following pattern:

```cpp
constexpr auto tmp_MACRO_NAME = MACRO_NAME;
#undef MACRO_NAME
export constexpr auto MACRO_NAME = tmp_MACRO_NAME;
```

### Functions

Contains a list of functions that can be used to expose function-like macros or values that cannot be exported through `constants`. Each line in this block consists of a function's signature and body, separated by `->`. The last semicolon in the body can be excluded as it is inserted automatically by the generator script.

```text
functions
    int& getErrno() -> return errno
```

### Namespaces and Identifiers

A namespace is specified with `ns` followed by its name. An inline namespace can be specified with `ns:inline`. Namespaces can contain identifiers and other namespaces. Nesting of namespaces is done through indentation and will be reflected in the generated module file.

Identifiers are any line that is not a block name and will be exported as using-declarations in their containing namespaces. Non-static enums, types, functions, and variables can be exposed this way.

If at the root of the file (not contained within a namespace), they will result in using-declarations in the global scope. For types, global identifiers can optionally be followed by another type to which they are aliased to.

```text
i8 std::int8_t

ns std
    string
    string_view

    ns chrono
        system_clock
        operator%
```

This produces:

```cpp
using i8 = std::uint8_t;

export namespace std {
    using std::string;
    using std::string_view;

    namespace chrono {
        using std::chrono::system_clock;
        using std::chrono::operator%;
    }
}
```

### Comments

Comments start with `//`. Within a line, everything past this will be ignored by the generator script.

## Generating Modules

The `generate.py` Python script in this directory creates the common modules and appropriate platform-specific modules in the build directory. This script should not be invoked manually; instead, it will be invoked by xmake when building the rest of the project.

You can also run `xmake build external` in the project root to only generate and build these module files.
