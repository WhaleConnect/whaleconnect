-- Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

-- This script parses the xrepo lock file and transforms it into a JSON file.

import("core.base.json")
import("core.base.semver")

local allPackages = {}

function recordPackages(packages, osKey)
    for package, info in pairs(packages) do
        local name = package:match("^([%w_%-]+)")
        if allPackages[name] == nil then
            local version = info["version"]
            local parsedVersion = semver.is_valid(version) and semver.new(version):shortstr() or version

            local online = info["repo"]["url"]:match("^https://")

            local repoRoot = online
                and path.join(val("globaldir"), "repositories", "xmake-repo")
                or path.join(val("projectdir"), "xmake")

            allPackages[name] = {
                filepath = path.join(repoRoot, "packages", name:sub(1, 1), name, "xmake.lua"),
                os = {}
            }

            local prevVersion = allPackages[name]["version"]
            if prevVersion == nil or (semver.is_valid(prevVersion) and semver.compare(parsedVersion, prevVersion) == 1) then
                allPackages[name]["version"] = parsedVersion
            end
        end

        table.insert(allPackages[name]["os"], osKey)
    end
end

function recordReferencedPackages()
    local xmakeFilePath = path.join(os.projectdir(), "xmake.lua")
    local packages = {}

    local lines = io.lines(xmakeFilePath)
    for line in lines do
        local package = line:match("add_packages%((.+)%)")
        if package then
            local args = package:split(",")
            for _, arg in ipairs(args) do
                local name = arg:split("\"", { plain = true, strict = true })[2]
                packages[#packages + 1] = name
            end
        end
    end

    packages = table.unique(packages)
    table.sort(packages)
    return packages
end

function main(...)
    local pkgInfo = io.load(path.join(os.projectdir(), "xmake-requires.lock"))
    for platform, packages in pairs(pkgInfo) do
        if platform ~= "__meta__" then
            local os = platform:match("(.+)|")
            local osKey = ""
            if os == "linux" then
                osKey = "L"
            elseif os == "macosx" then
                osKey = "M"
            elseif os == "windows" then
                osKey = "W"
            end

            recordPackages(packages, osKey)
        end
    end

    for _, packageInfo in pairs(allPackages) do
        table.sort(packageInfo["os"])
    end

    io.save(path.join(os.projectdir(), "build", "packages.txt"), allPackages)
    io.save(path.join(os.projectdir(), "build", "referenced.txt"), recordReferencedPackages())
end
