-- Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

-- This script generates the list of dependencies (docs/dependencies.md).
-- The package data file must be generated before running this script:
--   xmake l parse_lock_file.lua

local header = [[# WhaleConnect Dependencies

This information was generated with data from xmake-repo.

OS keys: L = Linux, M = macOS, W = Windows

| Name | Version | OS | License | Description |
| --- | --- | --- | --- | --- |
]]

function main(...)
    local packages = io.load(path.join(os.projectdir(), "build", "referenced.txt"))
    local lockInfo = io.load(path.join(os.projectdir(), "build", "packages.txt"))

    local outLines = {}
    for _, package in ipairs(packages) do
        local outLine = {}

        local packageInfo = lockInfo[package]
        if packageInfo == nil then
            print("Could not find information in lock file for:", package)
        else
            local packageFile = io.readfile(packageInfo["filepath"])

            local homepageStr = packageFile:match("set_homepage%(\"([^\n]+)\"%)")
            outLine[1] = homepageStr and ("[%s](%s)"):format(package, homepageStr) or package

            outLine[2] = packageInfo["version"]
            outLine[3] = table.concat(packageInfo["os"], " ")

            local licenseStr = packageFile:match("set_license%(\"([^\n]+)\"%)")
            outLine[4] = licenseStr or "Unknown"

            local descriptionStr = packageFile:match("set_description%(\"([^\n]+)\"%)")
            outLine[5] = descriptionStr or "None"
        end

        outLines[#outLines + 1] = ("| %s |"):format(table.concat(outLine, " | "))
    end

    local outText = header .. table.concat(outLines, "\n") .. "\n"
    io.writefile(path.join(os.projectdir(), "docs", "dependencies.md"), outText)
end
