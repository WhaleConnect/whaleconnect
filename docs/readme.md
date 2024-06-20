# Network Socket Terminal

**Network Socket Terminal** (NST) is an application for Internet and Bluetooth communication for Windows, Linux, and macOS.

This software is currently in alpha. Feedback and feature requests are welcome.

![Screenshot](img/screenshot.png)

## Documentation Links

- [Building from Source](building.md)
- [Dependencies](dependencies.md)
- [Testing](testing.md)

## Minimum Hardware Requirements

- **Architecture:** amd64, arm64
- **Free memory:** 256 MB
- **Free disk space:** 32 MB
- **Display resolution:** 1280x720

32-bit processors are not supported.

Additional features require:

- **Network adapter** for IP-based communication
- **Bluetooth adapter** for Bluetooth-based communication

## Software Requirements

- **Operating system:** Windows 10, macOS Big Sur (11), Linux kernel 5.19 or higher
- **OpenGL:** 3.0 or higher
- **Windows only:** Microsoft Visual C++ 2015-2022 Redistributable
- **Recommended:** Updated drivers for your graphics card; network and Bluetooth adapter

### About Operating Systems

- The Linux kernel 5.19 is required because of `io_uring_prep_cancel_fd` and `IORING_ASYNC_CANCEL_ALL` usage. The distribution does not matter.
- NST has not been tested on older versions of Windows and macOS than those above. While they may work, compatibility is not guaranteed.

## License

NST and its build scripts are licensed under the [GPL v3+ license](../COPYING).
