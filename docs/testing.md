# Testing Network Socket Terminal

NST's tests are located in the [tests directory](/tests). Certain tests are designed to test socket I/O functionality, so a server and/or remote device is required to run them.

Due to the nature of the tests (external script is required; actual hardware may be required), these tests are not run in GitHub Actions.

To build the unit tests, execute `xmake build socket-tests` in the root of the repository. The unit tests use the [Catch2](https://github.com/catchorg/Catch2) testing framework (see its [command-line usage](https://github.com/catchorg/Catch2/blob/devel/docs/command-line.md)).

## Test Server

A Python server script is located in `/tests/scripts`. It should be invoked with `-t [type]`, where `[type]` is the type of the server: `TCP`, `UDP`, `RFCOMM`, or `L2CAP`.

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

## Benchmarking

A small HTTP server is located in `/tests/benchmarks`. It responds to clients with the following response:

```text
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 4
Content-Type: text/html

test
```

This server can be used to assess the performance of NST's core system code through its throughput measurement. It can be built with `xmake build benchmark-server`.

This server accepts an optional command-line argument: the size of the thread pool. If unspecified, it uses the maximum number of supported threads on the CPU. When started, the server prints the TCP port it is listening on.
