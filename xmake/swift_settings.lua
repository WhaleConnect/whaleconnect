-- Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

import("core.project.config")

function getSwiftBuildMode()
    return config.mode()
end

function getSwiftSettings()
    local buildDir = format("$(buildir)/swift/%s", getSwiftBuildMode())
    local libDir = path.join(config.get("xcode"), "Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift")
    local linkDir = path.join(libDir, "macosx")

    return buildDir, libDir, linkDir
end
