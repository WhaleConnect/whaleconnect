# Testing Network Socket Terminal

NST's tests are located in the [tests directory](/tests). Certain tests are designed to test socket I/O functionality, so a server and/or remote device is required to run them.

## Test Server

A Python server script is located in /tests/scripts. It should be invoked with `-t [type]`, where `[type]` is the type of the server: `TCP`, `UDP`, `RFCOMM`, or `L2CAP`.

Multiple instances of the script may be run simultaneously so e.g., both a TCP server and UDP server can be available during testing.

The server may also be called with the following switches (mutually exclusive):

- `-i` for interactive mode (accept data from the standard input to send to clients)
- `-e` for echo mode (send back all data that it receives from the client)

When using the server with the unit tests, use the `-e` switch.

## Server Device

Some tests only need one device involved - for example, Internet Protocol tests can use the loopback address (`127.0.0.1` or `::1`) so they can be run on the same device as the server script. However, Bluetooth does not have such capabilities, so all Bluetooth tests must have a server running on a separate device. Also, Bluetooth sockets in Python may be limited on some platforms, so a Linux system is recommended for running Bluetooth servers.

## Settings

Both the C++ tests and the Python server load configuration information from the same file. See [this document](/tests/settings/readme.md) for more information.

> [!IMPORTANT]
> If you have separate devices that are running the tests and server script, the same settings file must be available on both.
