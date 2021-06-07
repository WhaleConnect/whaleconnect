# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This script will download the latest revision of Dear ImGui's docking branch
# (https://github.com/ocornut/imgui/tree/docking) from GitHub and place it in
# the imgui/ directory next to this script.

import os
import urllib.request

# Working directory of the script
working_dir = os.path.dirname(os.path.realpath(__file__))

# Dear ImGui GitHub repo URL
main_url = "https://raw.githubusercontent.com/ocornut/imgui/docking/"

# Main source files
main_files = ["imgui.cpp", "imgui.h", "imgui_demo.cpp", "imgui_draw.cpp",
              "imgui_internal.h", "imgui_tables.cpp", "imgui_widgets.cpp",
              "imstb_rectpack.h", "imstb_textedit.h", "imstb_truetype.h"]

# Backend files (both .cpp/.h variants, handled later)
backend_files = ["imgui_impl_glfw", "imgui_impl_opengl3"]

# Output directory
out_dir = os.path.join(working_dir, "imgui")


def download_file(name):
    """Download a file from the GitHub repo."""
    # Get the file name, it's always after the last forward slash
    # URLs always use forward slashes, never backslashes.
    file_name = name.split("/")[-1]
    # Open the full url, with the repo and the file
    with urllib.request.urlopen(main_url + name) as u:
        # Get the file size with the Content-Length HTTP leader
        file_size = int(u.getheader("Content-Length"))
        # If the file is > 1024 bytes, set the unit to kb
        file_unit = "bytes"
        if (file_size > 1024):
            file_size /= 1024
            file_unit = "kb"

        # Print info
        print(f"> {file_name} [{file_size:.2f} {file_unit}]")

        # Get the full path where the file will be saved
        out_name = os.path.join(out_dir, file_name)

        # Open the output file for writing
        with open(out_name, "wb") as f:
            # Write the URL file to the local file
            f.writelines(u.readlines())


# If the output directory doesn't exist, create it
if not os.path.exists(out_dir):
    os.makedirs(out_dir)

# Download all main files
print("Downloading main files...")
for file in main_files:
    download_file(file)

# Download backend files (.cpp/.h variants)
print("Downloading backend files...")
for file in backend_files:
    download_file("backends/" + file + ".cpp")
    download_file("backends/" + file + ".h")

# Generate imconfig.h if it doesn't exist
imconfig_path = os.path.join(out_dir, "imconfig.h")
if not os.path.exists(imconfig_path):
    print("Generating imconfig.h...")
    with open(imconfig_path, "w") as f:
        f.write("""#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_DISABLE_DEMO_WINDOWS
#define IMGUI_DISABLE_METRICS_WINDOW
""")

# Download license
print("Downloading license...")
download_file("LICENSE.txt")
