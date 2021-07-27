#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download the latest revision of Dear ImGui's docking
# branch (https://github.com/ocornut/imgui/tree/docking) from GitHub.

import os
from download_utils import *

# Dear ImGui GitHub repo URL
set_repo("ocornut/imgui", "docking")

# Main source files
main_files = ["imgui.cpp", "imgui.h", "imgui_demo.cpp", "imgui_draw.cpp",
              "imgui_internal.h", "imgui_tables.cpp", "imgui_widgets.cpp",
              "imstb_rectpack.h", "imstb_textedit.h", "imstb_truetype.h"]

# Backend files (both .cpp/.h variants, handled later)
backend_files = ["imgui_impl_glfw", "imgui_impl_opengl3"]

# Download all main files
print("Downloading main files...")
for file in main_files:
    download_file(file)

# Download backend files (.cpp/.h variants)
print("Downloading backend files...")
for file in backend_files:
    download_file(f"backends/{file}.cpp")
    download_file(f"backends/{file}.h")

# Generate imconfig.h if it doesn't exist
imconfig_path = calc_out_path("imconfig.h")
if not os.path.exists(imconfig_path):
    print("Generating imconfig.h...")
    with open(imconfig_path, "w") as f:
        f.write("""#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#if !defined(DEBUG) && !defined(_DEBUG) || defined(NDEBUG)
#define IMGUI_DISABLE_DEMO_WINDOWS
#define IMGUI_DISABLE_METRICS_WINDOW
#endif
""")

# Download license
print("Downloading license...")
download_file("LICENSE.txt")
