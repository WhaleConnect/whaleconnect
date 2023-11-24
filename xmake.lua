-- Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

add_repositories("xrepo-patches https://github.com/NSTerminal/xrepo-patches.git")

add_rules("mode.debug", "mode.release")

-- Avoid linking to system libraries - prevents dependency mismatches on different platforms and makes package self-contained
add_requireconfs("*|opengl", { system = false })

add_requires("imgui-with-sdl3 v20230807-docking", { configs = { sdl3_no_renderer = true, opengl3 = true, freetype = true } })
add_requires("libsdl3", { configs = { use_sdlmain = true } })
add_requires("catch2", "icu4c", "magic_enum", "nameof", "nlohmann_json", "opengl", "out_ptr")

if is_plat("linux") then
    add_requires("liburing", "bluez")
    add_requires("dbus", { configs = { system_bus_address = "unix:path=/run/dbus/system_bus_socket" } })
end

set_languages("c++20")
set_exceptions("cxx")
set_warnings("allextra")
set_defaultmode("debug")
set_license("GPL-3.0-or-later")

add_packages("icu4c", "magic_enum", "nameof", "out_ptr")

add_cxxflags("-Wno-missing-field-initializers",
             "-Wno-read-modules-implicitly",
             "-pthread", { tools = { "clang", "clangxx" } })
add_cxxflags("cl::/Zc:preprocessor", { force = true })

-- Use MSVC Unicode character set and prevent clashing macros
add_defines("UNICODE", "_UNICODE", "NOMINMAX")

-- UTF-8 charset
set_encodings("utf-8")

-- Platform detection macros ("or false" is needed because is_plat() can return nil)
add_defines("OS_WINDOWS=" .. tostring(is_plat("windows") or false),
            "OS_MACOS=" .. tostring(is_plat("macosx") or false),
            "OS_LINUX=" .. tostring(is_plat("linux") or false))

-- Include directories
add_includedirs("src")

-- Platform-specific links/compile options
local swiftBuildMode = is_mode("release") and "release" or "debug"
if is_plat("windows") then
    add_syslinks("Ws2_32", "Bthprops")
elseif is_plat("macosx") then
    local swiftBuildDir = format("$(scriptdir)/swift/.build/%s", swiftBuildMode)
    local swiftLibDir = "/Library/Developer/CommandLineTools/usr/lib/swift"

    add_includedirs(path.join(swiftBuildDir, "BluetoothMacOS.build"), swiftLibDir)
    add_linkdirs(swiftBuildDir, path.join(swiftLibDir, "macosx"))
    add_links("BluetoothMacOS")
elseif is_plat("linux") then
    add_packages("liburing", "dbus", "bluez")
end

target("swift")
    set_kind("phony")

    on_build("macosx", function(target)
        os.cd("$(scriptdir)/swift")
        os.exec("swift build -c %s", swiftBuildMode)
    end)

target("terminal-core")
    -- Project files
    set_kind("static")

    add_files("src/net/*.cpp", "src/net/*.mpp",
              "src/os/*.cpp", "src/os/*.mpp",
              "src/sockets/*.mpp", "src/sockets/delegates/*.mpp",
              "src/utils/*.cpp", "src/utils/*.mpp")

    -- Platform-specific files
    if is_plat("windows") then
        add_files("src/net/windows/*.cpp",
                  "src/os/windows/*.cpp", "src/os/windows/*.mpp",
                  "src/sockets/delegates/windows/*.cpp")
    elseif is_plat("macosx") then
        add_deps("swift")
        add_files("swift/bridge/cppbridge.cpp",
                  "src/net/macos/*.cpp",
                  "src/os/macos/*.cpp", "src/os/macos/*.mpp",
                  "src/sockets/delegates/macos/*.cpp")
    elseif is_plat("linux") then
        add_files("src/net/linux/*.cpp",
                  "src/os/linux/*.cpp", "src/os/linux/*.mpp",
                  "src/sockets/delegates/linux/*.cpp")
    end

target("terminal")
    add_packages("imgui-with-sdl3", "libsdl3", "opengl")
    add_deps("terminal-core")

    -- GUI code and main entry point
    add_files("src/components/*.cpp", "src/components/*.mpp",
              "src/gui/*.cpp", "src/gui/*.mpp",
              "src/main.cpp")

    -- Add platform rules
    if is_plat("windows") then
        add_rules("win.sdk.application")
        add_files("res/app.manifest")
    elseif is_plat("macosx") then
        add_rules("xcode.application")
        add_files("res/Info.plist")
    end

    on_load(function (target)
        -- Install font files and license next to executable
        -- Fonts are embedded in the Resources directory in the macOS bundle.
        local prefixdir = is_plat("macosx") and "" or "bin"
        target:add("installfiles", format("%s/(*.ttf)", target:targetdir()), { prefixdir = prefixdir })
        target:add("installfiles", "COPYING")
    end)

    -- Download font files next to executable on pre-build
    before_build(function (target)
        import("download.fonts").download_fonts(target:targetdir())
    end)

    after_install(function (target)
        import("download.licenses").download_licenses(target:installdir())
    end)

target("socket-tests")
    set_default(false)

    add_packages("catch2", "nlohmann_json")
    add_deps("terminal-core")
    add_files("tests/src/**.cpp", "tests/src/**.mpp")

     -- Path to settings file
    add_defines(format("SETTINGS_FILE=R\"(%s)\"", path.absolute("tests/settings/settings.json")))
