# WhaleConnect

![Banner](img/banner.png)

[![DOI](https://joss.theoj.org/papers/10.21105/joss.06964/status.svg)](https://doi.org/10.21105/joss.06964)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.13328366.svg)](https://doi.org/10.5281/zenodo.13328366)

**WhaleConnect** is an application for Internet and Bluetooth communication for Windows, Linux, and macOS.

![Screenshot](img/screenshot.png)

## Documentation Links

- [Building from Source](building.md)
- [Dependencies](dependencies.md)
- [Testing](testing.md)
- [Usage](https://www.aidansun.com/articles/whaleconnect-usage/)

## Design

WhaleConnect is designed to address limitations of existing network communication tools, which have low scalability over the number of connections and limited interoperability with other technologies. The user interface is supported by a custom I/O backend that uses asynchronous APIs native to the application's target operating systems:

- **Windows:** Input/output completion ports (IOCP)
- **macOS:** kqueue and IOBluetooth
- **Linux:** io_uring

This allows for efficient communication over parallel connections to other computers and devices. WhaleConnect also supports various common communication protocols on top of this framework, enabling interoperability with a wide range of network-enabled systems and platforms, and security is offered through Transport Layer Security (TLS) protocol support.

WhaleConnect aims to be a general-purpose communication tool that can be used by researchers, developers, and hobbyists in a wide variety of industries, such as the Internet of Things (IoT).

## Features

- Parallel connections
  - Create and manage multiple connections at the same time
  - Uses operating system features for efficiency and scalability
- Low resource usage: see system requirements
- Wide protocol support
  - TCP and UDP over IPv4 and IPv6
  - L2CAP and RFCOMM over Bluetooth
  - TLS 1.2 and 1.3 for TCP clients
- Clients
  - Create outgoing connections to other devices
  - DNS lookup for IP connections
  - Service Discovery Protocol (SDP) inquiries for Bluetooth connections
  - Create Bluetooth connections to devices paired to the computer
- Servers
  - Accept incoming connections from other devices
  - Options to view data sent from all clients and individual clients
  - Selections for clients to send data to or have connections closed
- Data view options
  - Timestamps
  - UTF-8 encoded hexadecimal
  - Logs of sent data
- Multiline textbox to send data
  - Select line ending: CR, LF, or both

## Minimum Hardware Requirements

- **Architecture:** amd64 (Windows, Linux), arm64 (macOS)
- **Free memory:** 256 MB
- **Free disk space:** 32 MB
- **Display resolution:** 1280x720

Additional features require:

- **Network adapter** for Internet-based communication
- **Bluetooth adapter** for Bluetooth-based communication

### About CPUs

- 32-bit processors are not supported.
- Only Apple Silicon (e.g., M1) CPUs are supported with macOS.
- You may get WhaleConnect running on an unsupported CPU by building from source, though we provide no guarantees of compatibility and may not be able to provide full support for any issues.

## Software Requirements

- **Operating system:** Windows 10 or later, macOS Sonoma 14.0 or later, Linux kernel 5.19 or later
- **OpenGL:** 3.0 or higher
- **Windows only:** Microsoft Visual C++ 2015-2022 Redistributable
- **Recommended:** Updated drivers for your graphics card, network adapter, and Bluetooth adapter

### About Operating Systems

- The Linux kernel 5.19 is required because of `io_uring_prep_cancel_fd` and `IORING_ASYNC_CANCEL_ALL` usage. The distribution does not matter.
- WhaleConnect has not been tested on older versions of Windows and macOS than those above. While they may work, compatibility is not guaranteed.

## Installation

To install WhaleConnect, follow these steps:

1. Go to the [releases page](https://github.com/WhaleConnect/whaleconnect/releases) for this repository.
2. Under the latest version (displayed first on the page), download the ZIP file for your operating system (the files are displayed under "Assets").
3. Extract the downloaded ZIP file.
4. Run WhaleConnect as shown below.

- **On Windows:** Run `bin/WhaleConnect.exe`.
- **On Linux:** Run `bin/WhaleConnect`.
- **On macOS:** You should have a `.dmg` file after extracting the ZIP. Copy `WhaleConnect.app` out of the DMG, then run `WhaleConnect.app`.

All executables are unsigned, so you may see warnings from your operating system if you run them. On macOS, if you see a message that says "WhaleConnect is damaged and can't be opened", run the following command, then reopen the application:

```shell
xattr -c path/to/WhaleConnect.app
```

> [!NOTE]
> You do not need to download any other software or libraries as all dependencies are bundled into the package that you download.

## License

WhaleConnect and its build scripts are licensed under the [GPL v3+ license](../COPYING).

## Citation

If you use WhaleConnect, please cite our paper:

```text
@article{WhaleConnect2024,
  author = {Sun, Aidan},
  doi = {10.21105/joss.06964},
  journal = {Journal of Open Source Software},
  month = aug,
  number = {100},
  pages = {6964},
  title = {{WhaleConnect: A General-Purpose, Cross-Platform Network Communication Application}},
  url = {https://joss.theoj.org/papers/10.21105/joss.06964},
  volume = {9},
  year = {2024}
}
```

## About the Name

With their complex songs and vocalizations, whales are among the best animal communicators. They can also be found in every ocean.

This project was previously called "Network Socket Terminal" until the 1.0.0 release.
