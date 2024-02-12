# Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from enum import Enum
import glob
import os
from pathlib import Path
import time
import sys

INDENT_SIZE = 4
SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))

# All possible blocks
Blocks = Enum(
    "Blocks", ["Global", "Define", "Include", "Constants", "Functions", "Namespace"]
)


# Returns the spacing for the current indentation level.
def calc_spacing(indentation: int):
    return " " * max(indentation * INDENT_SIZE, 0)


# Returns the correct number of brackets to reach a lower target indentation level.
def calc_dedent(indentation: int, target_indent: int, ns_list: list[str]):
    out = ""
    for i in range(indentation, target_indent, -1):
        ns_list.pop()
        out += f"{calc_spacing(i - 1)}}}\n"

    return out


# Returns a using-directive for a line.
def calc_using_statement(stripped: str):
    parts = stripped.split(" ", 1)
    return f"{parts[0]} = {parts[1]}" if len(parts) == 2 else f"::{parts[0]}"


# Returns the syntax for a namespace directive and if a new scope should be opened.
def handle_namespace(spacing: str, stripped: str, indentation: int, ns_list: list[str]):
    parts = stripped.split(" ")
    prefix = ""
    if parts[0] == "ns:inline":
        prefix = "inline "
    elif indentation == 0:
        prefix = "export "

    namespace_name = parts[1]
    if len(parts) == 2:
        ns_list.append(namespace_name)
        return True, f"{spacing}{prefix}namespace {namespace_name} {{\n"
    else:
        return False, f"{spacing}{prefix}namespace {namespace_name} = {parts[2]};\n"


# Returns the syntax for a line in a block.
def handle_statement(
    spacing: str,
    stripped: str,
    block: Blocks,
    ns_list: list[str],
):
    out = ""
    match block:
        case Blocks.Global:
            statement = calc_using_statement(stripped)
            out += f"{spacing}export using {statement};\n"
        case Blocks.Define:
            out += f"#define {stripped}\n"
        case Blocks.Include:
            out += f"#include <{stripped}>\n"
        case Blocks.Constants:
            parts = stripped.split(":")
            type = parts[1] if len(parts) == 2 else "constexpr auto"
            name = parts[0]
            tmp_name = f"tmp_{name}"
            out += f"{type} {tmp_name} = {name};\n#undef {name}\nexport {type} {name} = {tmp_name};\n"
        case Blocks.Functions:
            parts = stripped.split("->", 1)
            out += f"export {parts[0].strip()} {{ {parts[1].strip()}; }}\n"
        case Blocks.Namespace:
            full_namespace = "::".join(ns_list)
            out += f"{spacing}using {full_namespace}::{stripped};\n"

    return out


# Returns the generated C++ file for a list of data lines.
def generate(name: str, lines: list[str]):
    out = "module;\n"
    block = Blocks.Global
    indentation = 0
    can_indent = False

    ns_list = []

    for line_num, line in enumerate(lines):
        line_without_comments = line.split("//")[0]
        noindent = line_without_comments.lstrip()
        stripped = line_without_comments.strip()

        if not stripped:
            continue

        new_indent = (len(line_without_comments) - len(noindent)) // INDENT_SIZE

        excess_indents = new_indent - indentation > 1
        forbidden_indent = new_indent > indentation and not can_indent

        # Validate indentation
        if excess_indents or forbidden_indent:
            raise Exception(f"Extra indentation on line {line_num + 1}")

        # Check if exiting a block
        if new_indent < indentation:
            if block == Blocks.Include:
                out += f"export module external.{name};\n"
            elif block == Blocks.Namespace:
                out += calc_dedent(indentation, new_indent, ns_list)
            if new_indent == 0:
                block = Blocks.Global

        indentation = new_indent
        can_indent = True
        spacing = calc_spacing(indentation)

        if line.startswith("defines"):
            block = Blocks.Define
        elif line.startswith("includes"):
            block = Blocks.Include
        elif line.startswith("constants"):
            block = Blocks.Constants
        elif line.startswith("functions"):
            block = Blocks.Functions
        elif noindent.startswith("ns ") or noindent.startswith("ns:inline "):
            block = Blocks.Namespace
            can_indent, new_out = handle_namespace(
                spacing, stripped, indentation, ns_list
            )
            out += new_out
        else:
            can_indent = False
            out += handle_statement(spacing, stripped, block, ns_list)

    if indentation > 0 and block == Blocks.Namespace:
        out += calc_dedent(indentation, 0, ns_list)

    return out


# Returns the platform to use in directory names.
def get_platform():
    match sys.platform:
        case "win32":
            return "windows"
        case "darwin":
            return "macos"
        case "linux":
            return "linux"
        case _:
            raise Exception("Unsupported platform")


if len(sys.argv) != 2:
    raise Exception("Output directory must be specified")

print("Generating modules...")

out_dir = sys.argv[1]
Path(out_dir).mkdir(parents=True, exist_ok=True)

# Get last generation time from cache file
file = os.path.join(out_dir, "lastbuild.txt")
last_build_time = -1
if os.path.exists(file):
    with open(file) as f:
        last_build_time = float(f.read())

common_files = glob.glob(f"{SCRIPT_PATH}/common/*.txt")
platform_files = glob.glob(f"{SCRIPT_PATH}/{get_platform()}/*.txt")
for i in common_files + platform_files:
    out = ""
    name = Path(i).stem

    # Compare modification time to last generation time
    modified = os.path.getmtime(i)
    if modified > last_build_time:
        print(">", name)
        with open(i) as f:
            out = generate(name, f.readlines())

        with open(os.path.join(out_dir, name + ".mpp"), "w") as f:
            f.write(out)

# Save last generation time
with open(file, "w") as f:
    f.write(str(time.time()))

print("Generation complete.")
