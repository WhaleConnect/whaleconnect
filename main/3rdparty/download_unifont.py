#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download GNU Unifont
# (https://github.com/NSTerminal/unifont) from GitHub.

from download_utils import *

readme_text = b"""The Unifont distribution bundled with this repository\
 contains the full Unifont TTF file and its license documents.

For its source code and other relevant files, go to Network Socket\
 Terminal's GNU Unifont mirror at: https://github.com/NSTerminal/unifont
"""


def main():
    """The script's entry point.
    """

    # GNU Unifont GitHub repo URL (mirror)
    set_repo("NSTerminal/unifont", "main")

    # Download the TTF
    print("Downloading TTF...")
    download_file("font/precompiled/unifont-14.0.01.ttf")

    # Download license files
    print("Downloading licenses...")
    download_file("COPYING")
    download_file("OFL-1.1.txt")

    # Create readme
    print("Generating readme...")
    create_file("README.txt", readme_text, True)


if __name__ == "__main__":
    main()
