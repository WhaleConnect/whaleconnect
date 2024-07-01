# Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# This script is used in the CI to publish releases

from pathlib import Path
import re
import sys

changelog_path = project_dir = Path(__file__).parent.parent / "docs" / "changelog.md"

with open(changelog_path, "r") as f:
    changelog = f.read()

base_text = """These notes are also available on the [changelog](https://github.com/WhaleConnect/whaleconnect/blob/main/docs/changelog.md).

All executables are unsigned, so you may see warnings from your operating system if you run them.

"""

if latest_changes := re.search(r"\n## .+?\n\n(.+?)\n\n## ", changelog, re.S):
    text = base_text + latest_changes.group(1)
    with open("changelog-latest.txt", "w") as f:
        f.write(text)
else:
    sys.exit(1)
