# Testing Network Socket Terminal

NST's tests are located in the [tests directory](/tests). Certain tests are designed to test socket I/O functionality, so a server and/or remote device is required to run them.

## Test Server

A Python server script is located in /tests/scripts. It should be invoked with either `-t TCP` or `-t UDP` on the command line, which tell the script to run a TCP server or UDP server, respectively.

Multiple instances of the script may be run simultaneously so e.g., both a TCP server and UDP server can be available during testing.

The server may also be called with the following switches (mutually exclusive):

- `-i` for interactive mode (accept data from the standard input to send to clients)
- `-e` for echo mode (send back all data that it receives from the client)

## Server Device

Some tests only need one device involved - for example, Internet Protocol tests can use the loopback address (`127.0.0.1` or `::1`) so they can be run on the same device as the server script. However, Bluetooth does not have such capabilities, so all Bluetooth tests must have a server running on a separate device.

TODO: Add Bluetooth tests

## Settings

Both the C++ tests and the Python server load configuration information from the same file. See [this document](/tests/settings/readme.md) for more information.

**Note:** If you have separate devices that are running the tests and server script, the same settings file must be available on both.
