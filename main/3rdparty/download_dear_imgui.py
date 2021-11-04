#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download the latest revision of Dear ImGui's docking
# branch (https://github.com/ocornut/imgui/tree/docking) from GitHub.

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
    create_file("imconfig.h", imconfig_contents)

    # Download license
    print("Downloading license...")
    download_file("LICENSE.txt")


if __name__ == "__main__":
    main()
