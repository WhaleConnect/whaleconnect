# Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# This script generates the list of dependencies (docs/dependencies.md).
# The package data file must be generated before running this script:
#   xmake l parse_lock_file.lua

import json
from pathlib import Path
import re
import urllib.request

project_dir = Path(__file__).parent.parent
xmake_file_path = project_dir / "xmake.lua"
lock_json = project_dir / "build" / "packages.json"

# Parse the project xmake.lua to get packages which are explicitly required
packages = set([])
with open(xmake_file_path, "r") as f:
    package_search = re.compile(r"add_packages\((.+)\)")
    for line in f.readlines():
        if packages_str := package_search.search(line):
            args = packages_str.group(1).split(",")
            for arg in args:
                packages.add(arg.split("\"")[1])

# Read lock file JSON
with open(lock_json, "r") as f:
    lock_info = json.load(f)

license_search = re.compile(r"set_license\(\"(.+)\"\)")
homepage_search = re.compile(r"set_homepage\(\"(.+)\"\)")
description_search = re.compile(r"set_description\(\"(.+)\"\)")

out_text = """# Network Socket Terminal Dependencies

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

    if package_info["online"]:
        # Fetch xmake.lua from xmake-repo on GitHub
        url = f"https://raw.githubusercontent.com/xmake-io/xmake-repo/dev/packages/{package[0]}/{package}/xmake.lua"
        package_file = urllib.request.urlopen(url).read().decode("utf-8")
    else:
        # Local dependency, open xmake.lua from packages directory
        with open(project_dir / "xmake" / "packages" / package[0] / package / "xmake.lua", "r") as f:
            package_file = f.read()

    # Name
    if homepage_str := homepage_search.search(package_file):
        out_line.append(f"[{package}]({homepage_str.group(1)})")
    else:
        out_line.append(package)

    # Version
    out_line.append(package_info["version"])

    # OS
    out_line.append(" ".join(sorted(package_info["os"])))

    # License
    if license_str := license_search.search(package_file):
        out_line.append(license_str.group(1))
    else:
        out_line.append("Unknown")

    # Description
    if description_str := description_search.search(package_file):
        out_line.append(description_str.group(1))
    else:
        out_line.append("None")

    out_text += "| " + " | ".join(out_line) + " |\n"

with open(project_dir / "docs" / "dependencies.md", "w") as f:
    f.write(out_text)
