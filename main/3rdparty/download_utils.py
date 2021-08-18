#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script contains the definitions needed to download a file from
# GitHub.

import os
import urllib.request

out_dir = ""
repo_url = ""


def make_dir(name):
    """Create a directory if it doesn't already exist.

    Parameters
    ----------
    name : str
        The name of the directory to create (nested paths allowed)
    """
    if not os.path.exists(name):
        os.makedirs(name)


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
    global out_dir

    # Repository URL
    repo_url = f"https://raw.githubusercontent.com/{name}/{branch}/"

    # The working directory of this file
    file_dir = os.path.dirname(os.path.realpath(__file__))

    if out_name == "":
        # Unspecified output name, use the repo name
        # This is the part after the /:
        out_name = name.split("/")[1]

    # Join the current directory and the repository name together to
    # get the directory where the files will be placed
    out_dir = os.path.join(file_dir, out_name)

    # If the output directory doesn't exist, create it
    make_dir(out_dir)


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


def download_file(name, flatten=True):
    """Download a file from the specified GitHub repository.

    Parameters
    ----------
    name : str
        The file to download, relative to the repository's base path
    flatten : bool, optional
        If the download is directly placed into the output directory
        (not maintaining the repo's original file structure),
        by default True

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

        # Get the full path where the file will be saved
        out_name = ""

        if flatten:
            # The flatten option loses the original directory structure
            # and places everything directly inside the output dir.
            out_name = calc_out_path(file_name)
        else:
            # Setting flatten to false preserves the original directory
            # structure
            out_name = out_dir

            # Join each path part to the output
            for i in name.split("/"):
                out_name = os.path.join(out_name, i)

            # Make the directory:
            make_dir(os.path.dirname(out_name))

        # Open the output file for writing
        with open(out_name, "wb") as f:
            # Write the URL file to the local file
            f.writelines(u.readlines())
