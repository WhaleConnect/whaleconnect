#!/usr/bin/env python
# Copyright 2021 the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This script generates `errorlist.cpp` from the JSON data in
# `errorlist.jsonc`.

import json
import re
import os


def write_errs(f, err_list):
    """Write a set of errors to a file.

    Parameters
    ----------
    f : file object
        The file to write to
    err_list : dict
        The error list to write (key: error name, value: message string)
    """
    for err in err_list:
        err_name = err

        # If the delimiter is present in the string, it has an RN.
        # Split the string, then get its first part to access the
        # defined name.
        if delim in err:
            err_name = err.split(delim)[0]
        f.write(f"\n    E({err_name}, \"{err_list[err]}\"),")


# What separates an error and its remapped name in the JSON file
delim = ":"

# Where this script is in the filesystem
current_dir = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(current_dir)

# Open the JSON file and get its contents
json_str = ""
with open(os.path.join(current_dir, "errorlist.jsonc"), "r") as f:
    json_str = "".join(f.readlines())

# Preprocess the JSON by removing all comments, then parse it
json_str = re.sub(r"//.*", "", json_str)
data = json.loads(json_str)

# Open the output file for writing
with open(os.path.join(parent_dir, "errorlist.cpp"), "w") as f:
    cross_platform_errs = data["crossplatform"]

    # The start of the source file
    f.write(f"""// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file was generated with {os.path.basename(__file__)}.
// DO NOT MODIFY THIS FILE, edit the generator script/JSON data instead.

#include "sockets.hpp"

#ifdef _WIN32
#include <WinSock2.h>""")

    # On Windows, Unix error identifiers (e.g. EMSGSIZE) are already
    # defined. This loop will go through each, undefine them, then
    # redefine them to their Winsock equivalents.
    #
    # Generally in C++ (especially modern C++) use of macros is
    # discouraged, but here, each error name must be known at
    # preprocessing time so a smaller output file can be generated.
    # The preprocessor then takes the file and expands it for
    # compilation.
    for err in cross_platform_errs:
        # Undefine, then redefine the error
        if delim in err:
            parts = err.split(delim)
            f.write(f"\n#undef {parts[0]}\n#define {parts[0]} ")

            # If the error has an RN, write it in.
            f.write(parts[1])
        else:
            f.write(f"\n#undef {err}\n#define {err} WSA{err}")

    # The rest of the file
    f.write("""
#else
#include <cerrno>
#include <netdb.h>
#endif

#define E(code, desc) { code, { #code, desc } }

const std::unordered_map<long, Sockets::NamedError> Sockets::errors = {""")

    # Write the errors into the unordered map
    write_errs(f, cross_platform_errs)
    f.write("\n#ifdef _WIN32")
    write_errs(f, data["windows"])
    f.write("\n#else")
    write_errs(f, data["unix"])
    f.write("\n#endif\n};\n")
