#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download the latest revision of {fmt}'s master
# branch (https://github.com/fmtlib/fmt) from GitHub.

from download_utils import *

def main():
    """The script's entry point.
    """

    # {fmt} GitHub repo URL
    set_repo("Neargye/magic_enum", "master")

    # Download header
    print("Downloading header file...")
    download_file("include/magic_enum.hpp")

    # Download license
    print("Downloading license...")
    download_file("LICENSE")

if __name__ == "__main__":
    main()
