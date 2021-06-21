#!/usr/bin/env python
# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This script will download the latest revision of {fmt}'s master
# branch (https://github.com/fmtlib/fmt) from GitHub.

from download_utils import *

# Dear ImGui GitHub repo URL
set_repo("fmtlib/fmt", "master")

# Header files (.h extension)
header_files = ["chrono", "core", "format-inl", "format", "os"]

# Source files (.cc extension)
source_files = ["format", "os"]

# Download all header files
print("Downloading header files...")
for file in header_files:
    download_file(f"include/fmt/{file}.h")

# Download all source files
print("Downloading source files...")
for file in source_files:
    download_file(f"src/{file}.cc")

# Download license
print("Downloading license...")
download_file("LICENSE.rst")
