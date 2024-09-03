# Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Functions and definitions to parse and interface with xmake and xmake-repo files.
# This is not a runnable script; it does nothing by itself.

import json
from pathlib import Path
import re

PROJECT_DIR = Path(__file__).parent.parent

LICENSE_SEARCH = re.compile(r"set_license\(\"(.+)\"\)")
HOMEPAGE_SEARCH = re.compile(r"set_homepage\(\"(.+)\"\)")
DESCRIPTION_SEARCH = re.compile(r"set_description\(\"(.+)\"\)")


# Parses the project xmake.lua to get packages which are explicitly required.
def get_referenced_packages() -> set[str]:
    xmake_file_path = PROJECT_DIR / "xmake.lua"

    packages = set([])
    with open(xmake_file_path, "r") as f:
        package_search = re.compile(r"add_packages\((.+)\)")
        for line in f.readlines():
            if packages_str := package_search.search(line):
                args = packages_str.group(1).split(",")
                for arg in args:
                    packages.add(arg.split('"')[1])

    return packages


# Reads the lock file JSON.
def read_lock_file() -> dict:
    lock_json = PROJECT_DIR / "build" / "packages.json"

    with open(lock_json, "r") as f:
        lock_info = json.load(f)

    return lock_info


# Base matcher function for package metadata.
def match_or_none(s: str, pattern: re.Pattern) -> str | None:
    if matched := pattern.search(s):
        return matched.group(1)
    return None


def find_license(s: str) -> str | None:
    return match_or_none(s, LICENSE_SEARCH)


def find_homepage(s: str) -> str | None:
    return match_or_none(s, HOMEPAGE_SEARCH)


def find_description(s: str) -> str | None:
    return match_or_none(s, DESCRIPTION_SEARCH)
