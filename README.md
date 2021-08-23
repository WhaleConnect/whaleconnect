# Network Socket Terminal

**Network Socket Terminal** (NST) is an application for Internet and Bluetooth communication on Windows and Linux computers.

*Note: This software is currently in alpha status. All feedback/feature requests welcome.*

![Screenshot](/docs/img/shot1.png)

## Documentation Links

- [Screenshots](/docs/SCREENSHOTS.md)
- [Building from Source](/docs/BUILDING.md)
- [Software Used](/docs/SOFTWARE.md)
- [Todo List](/docs/TODO.txt)

## Licenses

This repository contains two licenses: the third version of the GNU General Public License (GPLv3) and the MIT license.

The [GPLv3 license](/COPYING) applies to the NST source code and CMake build scripts.\
The [MIT license](/docs/MIT_LICENSE.txt) applies to all utility scripts that are unrelated to the GPL-ed code and releases (e.g. Python downloaders).

## Third-Party Dependencies

NST uses the following third-party libraries and dependencies:

- {fmt} (MIT License) [Website](https://fmt.dev/) | [GitHub](https://github.com/fmtlib/fmt)
- GLFW (Zlib License) [Website](https://glfw.org/) | [GitHub](https://github.com/glfw/glfw)
- Dear ImGui (MIT License) [GitHub](https://github.com/ocornut/imgui)
- GNU Unifont (Dual-Licensed) [Website](https://unifoundry.com/unifont/index.html) | [Sources](https://unifoundry.com/unifont/unifont-utilities.html)
- BlueZ and GDBus (GPL v2+) [BlueZ Website](https://bluez.org/) | [GitHub](https://github.com/bluez/bluez)

*GNU Unifont is dual-licensed under the GNU GPLv2+ with font embedding exception, and the SIL OFL v1.1.*

NST also used the following dependencies in the past:

- glad (Public Domain) [Webservice for v2](https://gen.glad.sh/) | [GitHub](https://github.com/Dav1dde/glad) **Dropped 8/19/2021**
