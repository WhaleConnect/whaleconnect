# Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# This script generates the list of dependencies (docs/dependencies.md).
# The package data file must be generated before running this script:
#   xmake l parse_lock_file.lua

import xmake_utils

packages = xmake_utils.get_referenced_packages()
lock_info = xmake_utils.read_lock_file()

out_text = """# WhaleConnect Dependencies

This information was generated with data from xmake-repo.

OS keys: L = Linux, M = macOS, W = Windows

| Name | Version | OS | License | Description |
| --- | --- | --- | --- | --- |
"""

for package in sorted(packages):
    out_line = []

    package_info = lock_info.get(package)
    if package_info is None:
        print("Could not find information in lock file for:", package)
        continue

    with open(package_info["filepath"], "b+r") as f:
        # Explicitly decode as UTF-8 since package files may contain Unicode
        package_file = f.read().decode("utf-8")

    # Name
    if homepage_str := xmake_utils.find_homepage(package_file):
        out_line.append(f"[{package}]({homepage_str})")
    else:
        out_line.append(package)

    # Version
    out_line.append(package_info["version"])

    # OS
    out_line.append(" ".join(sorted(package_info["os"])))

    # License
    if license_str := xmake_utils.find_license(package_file):
        out_line.append(license_str)
    else:
        out_line.append("Unknown")

    # Description
    if description_str := xmake_utils.find_description(package_file):
        out_line.append(description_str)
    else:
        out_line.append("None")

    out_text += "| " + " | ".join(out_line) + " |\n"

with open(xmake_utils.PROJECT_DIR / "docs" / "dependencies.md", "w") as f:
    f.write(out_text)
