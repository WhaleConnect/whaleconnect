# Network Socket Terminal

**Network Socket Terminal** (NST) is an application for Internet and Bluetooth communication for Windows, Linux, and macOS.

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

- **Network adapter** for Internet-based communication
- **Bluetooth adapter** for Bluetooth-based communication

## Software Requirements

- **Operating system:** Windows 10, macOS Big Sur (11), Linux kernel 5.19 or higher
- **OpenGL:** 3.0 or higher
- **Windows only:** Microsoft Visual C++ 2015-2022 Redistributable
- **Recommended:** Updated drivers for your graphics card; network and Bluetooth adapter

### About Operating Systems

- The Linux kernel 5.19 is required because of `io_uring_prep_cancel_fd` and `IORING_ASYNC_CANCEL_ALL` usage. The distribution does not matter.
- NST has not been tested on older versions of Windows and macOS than those above. While they may work, compatibility is not guaranteed.

## Installation

To install NST, follow these steps:

1. Go to the [releases page](https://github.com/NSTerminal/terminal/releases) for this repository.
2. Under the latest version (displayed first on the page), download the ZIP file for your operating system (the files are displayed under "Assets").
3. Extract the downloaded ZIP file.
4. Run NST as shown below.

- **On Windows:** Run `bin/terminal.exe`.
- **On Linux:** Run `bin/terminal`.
- **On macOS:** You should have a `.dmg` file after extracting the ZIP. Copy `terminal.app` out of the DMG, then run `terminal.app`.

All executables are unsigned, so you may see warnings from your operating system if you run them. On macOS, if you see a message that says "terminal is damaged and can't be opened", run the following command, then reopen the application:

```shell
xattr -c path/to/terminal.app
```

> [!NOTE]
> You do not need to download any other software or libraries as all dependencies are bundled into the package that you download.

## License

NST and its build scripts are licensed under the [GPL v3+ license](../COPYING).
