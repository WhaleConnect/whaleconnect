-- Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

-- This script is used in the CI to publish releases

local baseText = [[These notes are also available on the [changelog](https://github.com/WhaleConnect/whaleconnect/blob/main/docs/changelog.md).

All executables are unsigned, so you may see warnings from your operating system if you run them.

]]

function main(...)
    local changelogPath = path.join(os.projectdir(), "docs", "changelog.md")
    local changelog = io.readfile(changelogPath)

    local latestChanges = changelog:match("\n## .-\n\n(.-)\n\n## ")
    if latestChanges then
        io.writefile("changelog-latest.txt", baseText .. latestChanges)
    else
        os.exit(1)
    end
end
