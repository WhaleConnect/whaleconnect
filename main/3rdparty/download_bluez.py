#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download the latest revision of the GDBus and BlueZ
# APIs (https://github.com/bluez/bluez) from GitHub.

from download_utils import *

# BlueZ files (only pick out what we need)
bluez_files = ["bluetooth.c", "bluetooth.h",
               "hci.c", "hci.h", "hci_lib.h", "l2cap.h",
               "rfcomm.h", "sdp.c", "sdp.h", "sdp_lib.h"]

# GDBus files
gdbus_files = ["client.c", "gdbus.h", "mainloop.c", "object.c", "polkit.c",
               "watch.c"]


def main():
    """The script's entry point.
    """

    # BlueZ GitHub repo URL
    set_repo("bluez/bluez", "master", "bluetooth")

    # Download BlueZ
    print("Downloading BlueZ...")
    for file in bluez_files:
        download_file(f"lib/{file}")

    download_file("COPYING")

    # Download GDBus
    set_out_dir("gdbus")
    print("Downloading GDBus...")
    for file in gdbus_files:
        download_file(f"gdbus/{file}")

    download_file("COPYING")


if __name__ == "__main__":
    main()
