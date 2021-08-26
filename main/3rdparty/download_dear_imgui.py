#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download the latest revision of Dear ImGui's docking
# branch (https://github.com/ocornut/imgui/tree/docking) from GitHub.

import os
from download_utils import *

# Main source files
main_files = ["imgui.cpp", "imgui.h", "imgui_demo.cpp", "imgui_draw.cpp",
              "imgui_internal.h", "imgui_tables.cpp", "imgui_widgets.cpp",
              "imstb_rectpack.h", "imstb_textedit.h", "imstb_truetype.h"]

# Backend files (both .cpp/.h variants, handled later)
backend_files = ["imgui_impl_glfw", "imgui_impl_opengl3"]

# Contents of imconfig.h
imconfig_contents = b"""#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#ifdef NDEBUG
#define IMGUI_DISABLE_DEMO_WINDOWS
#define IMGUI_DISABLE_METRICS_WINDOW
#endif
"""


def main():
    """The script's entry point.
    """

    # Dear ImGui GitHub repo URL
    set_repo("ocornut/imgui", "docking")

    # Download all main files
    print("Downloading main files...")
    for file in main_files:
        download_file(file)

    # Download backend files (.cpp/.h variants)
    print("Downloading backend files...")
    for file in backend_files:
        download_file(f"backends/{file}.cpp")
        download_file(f"backends/{file}.h")

    # Download Dear ImGui built-in OpenGL loader (NEW: 8/19/2021)
    download_file("backends/imgui_impl_opengl3_loader.h")

    # Generate imconfig.h if it doesn't exist
    imconfig_path = calc_out_path("imconfig.h")
    if not os.path.exists(imconfig_path):
        print("Generating imconfig.h...")
        with open(imconfig_path, "wb") as f:
            # This Python script has LF (\n) line endings,
            # the target imconfig.h should have CRLF (\r\n) endings.
            f.write(imconfig_contents.replace(b"\n", b"\r\n"))

    # Download license
    print("Downloading license...")
    download_file("LICENSE.txt")


if __name__ == "__main__":
    main()
