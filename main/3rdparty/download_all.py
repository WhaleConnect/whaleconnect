#!/usr/bin/env python
# SPDX-License-Identifier: MIT
#
# This script will download all of NST's third-party dependencies at
# once.

import download_dear_imgui
import download_fmt
import download_unifont

# Run each script's entry point
download_dear_imgui.main()
download_fmt.main()
download_unifont.main()
