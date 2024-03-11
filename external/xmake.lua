-- Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

local swiftBuildMode = is_mode("release") and "release" or "debug"
local swiftBuildDir = format("$(projectdir)/swift/.build/%s", swiftBuildMode)
local swiftLibDir = "/Library/Developer/CommandLineTools/usr/lib/swift"

rule("external")
    on_load(function(target)
        target:set("kind", "moduleonly")
        if target:is_plat("macosx") then
            target:add("includedirs", swiftLibDir, { public = true })
        end
    end)

target("swift")
    set_kind("phony")

    on_build("macosx", function(target)
        os.cd("$(projectdir)/swift")
        os.exec("swift build -c %s", swiftBuildMode)
    end)

target("external-generator")
    set_kind("phony")
    on_build(function (target)
        os.exec("python $(scriptdir)/generate.py $(buildir)/generated-modules")
    end)

target("external-std")
    set_kind("moduleonly")
    set_warnings("none")
    add_deps("external-generator")
    add_files("$(buildir)/generated-modules/std.mpp")

target("external-core")
    add_rules("external")
    add_packages("botan", "out_ptr", { public = true })
    add_files(
        "$(buildir)/generated-modules/botan.mpp",
        "$(buildir)/generated-modules/out_ptr.mpp",
        "$(buildir)/generated-modules/platform.mpp"
    )

    if is_plat("macosx") then
        add_includedirs(path.join(swiftBuildDir, "BluetoothMacOS.build"), { public = true })
        add_linkdirs(swiftBuildDir, path.join(swiftLibDir, "macosx"), { public = true })
        add_links("BluetoothMacOS", { public = true })
    end

target("external-app")
    add_rules("external")
    add_packages("imgui-with-sdl3", "imguitextselect", "libsdl3", "opengl", "out_ptr", "utfcpp", { public = true })
    if is_plat("linux") then
        add_packages("liburing", "dbus", "bluez", { public = true })
    end

    add_files(
        "$(buildir)/generated-modules/imgui.mpp",
        "$(buildir)/generated-modules/imguitextselect.mpp",
        "$(buildir)/generated-modules/libsdl3.mpp",
        "$(buildir)/generated-modules/menu.mpp",
        "$(buildir)/generated-modules/out_ptr.mpp",
        "$(buildir)/generated-modules/utfcpp.mpp"
    )

    if is_plat("macosx") then
        add_deps("swift")
        add_includedirs(path.join(swiftBuildDir, "GUIMacOS.build"), { public = true })
        add_linkdirs(swiftBuildDir, path.join(swiftLibDir, "macosx"), { public = true })
        add_links("GUIMacOS", { public = true })
    end
