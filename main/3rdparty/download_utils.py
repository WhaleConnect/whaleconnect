#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script contains the definitions needed to download a file from
# GitHub.
# This is not a runnable script; it does nothing by itself.

import os
import urllib.request

out_dir, repo_url = "", ""


def calc_out_path(name):
    """Get the name of a file appended to the output directory.

    Parameters
    ----------
    name : str
        The name of the file

    Returns
    -------
    str
        The given name joined to the end of the output directory path
    """

    return os.path.join(out_dir, name)


def make_dir(name):
    """Create a directory if it doesn't already exist.

    Parameters
    ----------
    name : str
        The name of the directory to create relative to the output
        directory (nested directories allowed)
    """

    full_path = calc_out_path(name)
    if not os.path.exists(full_path):
        os.makedirs(full_path)

def set_out_dir(name):
    """Set the directory in which downloaded files will be placed.

    Parameters
    ----------
    name : str
        The path of the output directory, relative to the path of
        this script
    """

    global out_dir

    # Where this file is
    file_dir = os.path.dirname(os.path.realpath(__file__))

    out_dir = os.path.join(file_dir, name)

    # If the output directory doesn't exist, create it
    make_dir(out_dir)


def set_repo(name, branch, out_name=""):
    """Configure the script to download files from a GitHub repository.

    Parameters
    ----------
    name : str
        The repository's identifier, in the format [owner]/[repo]
    branch : str
        The branch to download from: "master", "main", etc.
    out_name : str, optional
        The name of the output directory to places the files in, by
        default the repository name
    """

    global repo_url

    # Repository URL
    repo_url = f"http://raw.githubusercontent.com/{name}/{branch}/"

    if out_name == "":
        # Unspecified output name, use the repo name
        # This is the part after the /:
        out_name = name.split("/")[1]

    set_out_dir(out_name)

    # Print an information message
    print(f"Downloading branch '{branch}' of '{name}' to '{out_name}'")

def write_file(name, contents, normalize_crlf=True):
    """Write a string to a file in the output directory.

    Parameters
    ----------
    name : str
        The name of the file relative to the output directory
    contents : bytes
        The string to write to the file
    normalize_crlf : bool, optional
        If newline endings (LF) in "contents" are replaced with CRLF, by
        default True
    """

    with open(calc_out_path(name), "wb") as f:
        if normalize_crlf:
            f.write(contents.replace(b"\n", b"\r\n"))
        else:
            f.write(contents)

def download_file(name, flatten=True, normalize_crlf=True):
    """Download a file from the specified GitHub repository.

    Parameters
    ----------
    name : str
        The file to download, relative to the repository's base path
    flatten : bool, optional
        If the download is directly placed into the output directory
        (not maintaining the repo's original file structure),
        by default True
    normalize_crlf : bool, optional
        If newline endings (LF) in the file are replaced with CRLF, by
        default True

    Raises
    ------
    ValueError
        If the repository has not been specified
    """

    if repo_url == "" or out_dir == "":
        raise ValueError("Repository is not defined, use set_repo().")

    # Get the file name, it's always after the last forward slash
    # URLs always use forward slashes, never backslashes.
    file_name = name.split("/")[-1]
    # Open the full url, with the repo and the file
    with urllib.request.urlopen(repo_url + name) as u:
        # Get the file size with the Content-Length HTTP leader
        file_size = int(u.getheader("Content-Length"))

        # If the file is > 1024 bytes, set the unit to kb
        file_unit = "bytes"
        if (file_size > 1024):
            file_size /= 1024
            file_unit = "kb"

        # Print info
        print(f"> {file_name} [{file_size:.2f} {file_unit}]")

        # Get the path where the file will be saved
        out_name = ""

        if flatten:
            # The flatten option loses the original directory structure
            # and places everything directly inside the output dir.
            out_name = file_name
        else:
            # Setting flatten to false preserves the original directory
            # structure
            # Join each path part to the output
            out_name = os.path.join(*[i for i in name.split("/")])

            # Make the directory:
            make_dir(os.path.dirname(out_name))

        # Open the output file for writing
        write_file(out_name, u.read(), normalize_crlf)


def create_file(path, contents=b"", normalize_crlf=True):
    """Create a file in the output directory and write to it.

    Parameters
    ----------
    path : str
        The path of the file, relative to the path of the path of
        the output directory
    contents : bytes, optional
        The text to write to the file, as a bytes string, by default
        empty
    normalize_crlf : bool, optional
        If newline endings (LF) in "contents" are replaced with CRLF, by
        default True
    """

    # Get the full path using `calc_out_path()`
    full_path = calc_out_path(path)

    # If the file doesn't exist, create it
    if not os.path.exists(full_path):
        write_file(full_path, contents, normalize_crlf)
