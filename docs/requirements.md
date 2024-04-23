# Network Socket Terminal System Requirements

This document lists the minimum system requirements for NST to run.

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
