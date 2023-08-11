# Network Socket Terminal

**Network Socket Terminal** (NST) is an application for Internet and Bluetooth communication for Windows, Linux, and macOS.

This software is currently in alpha. Feedback and feature requests are welcome.

![Screenshot](img/screenshot.png)

## Documentation Links

- [Building from Source](building.md)
- [System Requirements](requirements.md)
- [Testing](testing.md)
- [Todo List](todo.txt)

## Development Branch

The [`dev` branch](https://github.com/NSTerminal/terminal/tree/dev) is used for in-progress features. Updates can vary in maturity; the branch is not guaranteed to be stable.

When a feature is merged into `main` it is ready for release.

## License

NST and its build scripts are licensed under the [GPL v3+ license](../COPYING).

## Dependencies

### Libraries

- [BlueZ](https://github.com/bluez/bluez/tree/master/lib) (GNU GPL v2+ License)
- [Catch2](https://github.com/catchorg/Catch2) (BSL 1.0 License)
- [Dear ImGui](https://www.github.com/ocornut/imgui) (MIT License)
- [ICU](https://unicode-org.github.io/icu/) (Unicode License)
- [JSON for Modern C++](https://github.com/nlohmann/json) (MIT License)
- [libdbus](https://www.freedesktop.org/wiki/Software/dbus) (GNU GPL v2+ License)
- [liburing](https://github.com/axboe/liburing) (GNU LGPL v2.1 and MIT Licenses)
- [Magic Enum](https://github.com/Neargye/magic_enum) (MIT License)
- [SDL](https://www.libsdl.org/) (Zlib License)
- [ztd.out_ptr](https://github.com/soasis/out_ptr) (Apache-2.0 License)

### Fonts

- [Noto Sans Mono](https://fonts.google.com/noto/specimen/Noto+Sans+Mono) (SIL OFL 1.1 License)
- [Remix Icon](https://remixicon.com/) (Apache-2.0 License)
