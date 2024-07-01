-- Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

-- This script parses the xrepo lock file and transforms it into a JSON file that can be used by Python.

import("core.base.json")
import("core.base.semver")

all_packages = {}

function record_packages(packages, os_key)
    for package, info in pairs(packages) do
        local name = package:match("^([%w_%-]+)")
        if all_packages[name] == nil then
            local version = info["version"]
            local parsed_version = semver.is_valid(version) and semver.new(version):shortstr() or version

            local online = info["repo"]["url"]:match("^https://")

            local repo_root = online
                and path.join(val("globaldir"), "repositories", "xmake-repo")
                or path.join(val("projectdir"), "xmake")

            all_packages[name] = {
                version = parsed_version,
                filepath = path.join(repo_root, "packages", name:sub(1, 1), name, "xmake.lua"),
                os = {}
            }
        end

        table.insert(all_packages[name]["os"], os_key)
    end
end

function main(...)
    package_info = io.load(path.join(os.projectdir(), "xmake-requires.lock"))
    for platform, packages in pairs(package_info) do
        if platform ~= "__meta__" then
            local os = platform:match("(.+)|")
            local os_key = ""
            if os == "linux" then
                os_key = "L"
            elseif os == "macosx" then
                os_key = "M"
            elseif os == "windows" then
                os_key = "W"
            end

            record_packages(packages, os_key)
        end
    end

    json.savefile(path.join(os.projectdir(), "build", "packages.json"), all_packages)
end
