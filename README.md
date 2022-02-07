# Network Socket Terminal

**Network Socket Terminal** (NST) is an application for Internet and Bluetooth communication on Windows and Linux computers.

*Note: This software is currently in alpha status. All feedback/feature requests welcome.*

![Screenshot](/docs/img/shot1.png)

## Documentation Links

- [Screenshots](/docs/SCREENSHOTS.md)
- [Building from Source](/docs/BUILDING.md)
- [Software Used](/docs/SOFTWARE.md)
- [Todo List](/docs/TODO.txt)

## Development Branch

The [`dev` branch](https://github.com/NSTerminal/terminal/tree/dev) is used for in-progress features. Updates can vary in maturity; **the branch is not guaranteed to compile or run properly.**

When a feature is merged into `main` it is ready for release.

## Licenses

This repository contains two licenses: the third version of the GNU General Public License (GPLv3) and the MIT license.

The [GPLv3 license](/COPYING) applies to the NST source code and CMake build scripts.\
The [MIT license](/docs/MIT_LICENSE.txt) applies to all utility scripts that are unrelated to the GPL-ed code and releases (e.g. Python downloaders).

## Third-Party Dependencies

NST uses the following third-party libraries and dependencies:

- {fmt} (MIT License) [Website](https://fmt.dev/) | [GitHub](https://www.github.com/fmtlib/fmt)
- GLFW (Zlib License) [Website](https://www.glfw.org/) | [GitHub](https://www.github.com/glfw/glfw)
- Dear ImGui (MIT License) [GitHub](https://www.github.com/ocornut/imgui)
- GNU Unifont (Dual-Licensed) [Website](https://www.unifoundry.com/unifont/index.html) | [Sources](https://www.unifoundry.com/unifont/unifont-utilities.html) | [Mirror on GitHub](https://www.github.com/NSTerminal/unifont)
- BlueZ (GNU GPL v2+) [Website](http://www.bluez.org/) | [GitHub](https://www.github.com/bluez/bluez)
- libdbus (GNU GPL v2+) [Website](https://www.freedesktop.org/wiki/Software/dbus)

*GNU Unifont is dual-licensed under the GNU GPLv2+ with font embedding exception, and the SIL OFL v1.1.*

NST also used the following dependencies in the past:

- glad (Public Domain) [Webservice for v2](https://gen.glad.sh/) | [GitHub](https://www.github.com/Dav1dde/glad) **Dropped 8/19/2021**
- GDBus (GNU GPL v2+) [GitHub](https://github.com/bluez/bluez/tree/master/gdbus) **Dropped 11/12/2021**
