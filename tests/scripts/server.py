# Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# This script is intended to support testing of WhaleConnect.

import argparse
import configparser
import pathlib
import socket
import sys

# Command-line arguments to set server configuration
parser = argparse.ArgumentParser()
parser.add_argument("-t", "--transport", type=str, required=True)

mode = parser.add_mutually_exclusive_group(required=True)
mode.add_argument("-i", "--interactive", action="store_true")
mode.add_argument("-e", "--echo", action="store_true")

args = parser.parse_args()
is_interactive = args.interactive
is_echo = args.echo

# Load settings from INI
config = configparser.ConfigParser()
config.read(pathlib.Path(__file__).parent.parent / "settings" / "settings.ini")

match args.transport:
    case "TCP":
        SOCKET_TYPE = socket.SOCK_STREAM
        SOCKET_FAMILY = socket.AF_INET6
        SOCKET_PROTO = -1
        PORT = int(config["ip"]["tcpPort"])
        HOST = "::"
    case "UDP":
        SOCKET_TYPE = socket.SOCK_DGRAM
        SOCKET_FAMILY = socket.AF_INET6
        SOCKET_PROTO = -1
        PORT = int(config["ip"]["udpPort"])
        HOST = "::"
    case "RFCOMM":
        if sys.platform == "linux":
            SOCKET_TYPE = socket.SOCK_STREAM
            SOCKET_FAMILY = socket.AF_BLUETOOTH
            SOCKET_PROTO = socket.BTPROTO_RFCOMM
            PORT = int(config["bluetooth"]["rfcommPort"])
            HOST = socket.BDADDR_ANY
        else:
            raise Exception("RFCOMM is only available on Linux")
    case "L2CAP":
        if sys.platform == "linux":
            SOCKET_TYPE = socket.SOCK_SEQPACKET
            SOCKET_FAMILY = socket.AF_BLUETOOTH
            SOCKET_PROTO = socket.BTPROTO_L2CAP
            PORT = int(config["bluetooth"]["l2capPSM"])
            HOST = socket.BDADDR_ANY
        else:
            raise Exception("L2CAP is only available on Linux")
    case _:
        raise Exception("Unsupported transport")


# Handles I/O for a TCP server.
def server_loop_tcp(s: socket.socket):
    # Wait for new client connections
    conn, addr = s.accept()
    peer_addr = addr[0]

    print(f"New connection from {peer_addr}")

    with conn:
        while True:
            # Wait for new data to be received
            data = conn.recv(1024)
            if data:
                print(f"Received: {data.decode()}")

                if is_interactive:
                    input_str = input("> ")
                    conn.sendall(input_str.encode())
                elif is_echo:
                    # Send back data
                    conn.sendall(data)
            else:
                # Connection closed by client
                print(f"Disconnected from {peer_addr}")
                conn.close()
                break


# Handles I/O for a UDP server.
def server_loop_udp(s: socket.socket):
    data, addr = s.recvfrom(1024)
    if data:
        print(f"Received: {data} (from {addr[0]})")

        if is_interactive:
            input_str = input("> ")
            s.sendto(input_str.encode(), addr)
        elif is_echo:
            s.sendto(data, addr)


if is_interactive:
    print("Running in interactive mode.")
elif is_echo:
    print("Running in echo mode.")

with socket.socket(SOCKET_FAMILY, SOCKET_TYPE, SOCKET_PROTO) as s:
    # Enable dual-stack server so IPv4 and IPv6 clients can connect
    if SOCKET_FAMILY == socket.AF_INET6:
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)

    s.bind((HOST, PORT))

    # Listen on connection-based sockets
    is_dgram = SOCKET_TYPE == socket.SOCK_DGRAM
    if not is_dgram:
        s.listen()

    print(f"Server is active on port {PORT}")

    # Handle clients until termination
    while True:
        try:
            if is_dgram:
                server_loop_udp(s)
            else:
                server_loop_tcp(s)
        except IOError as e:
            print("I/O Error:", e)
            break
        except KeyboardInterrupt as e:
            print("Interrupted")
            break
