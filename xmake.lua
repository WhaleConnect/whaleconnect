-- Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

set_version("0.4.0", { build = "%Y%m%d%H%M" })
add_repositories("xrepo-patches https://github.com/NSTerminal/xrepo-patches.git")

add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

-- Avoid linking to system libraries - prevents dependency mismatches on different platforms and makes package self-contained
add_requireconfs("*|opengl", { system = false })

add_requires("imgui-with-sdl3 v20240204-docking", { configs = { sdl3_no_renderer = true, opengl3 = true, freetype = true } })
add_requires("libsdl3", { configs = { use_sdlmain = true, wayland = false } })
add_requires("botan", "catch2", "imguitextselect", "opengl", "out_ptr", "utfcpp")

if is_plat("linux") then
    add_requires("liburing", "bluez")
    add_requires("dbus", { configs = { system_bus_address = "unix:path=/run/dbus/system_bus_socket" } })
end

if not is_plat("windows") then
    add_cxxflags("-pthread", "-fexperimental-library")
    add_ldflags("-fexperimental-library")
end

set_languages("c++23")
set_exceptions("cxx")
set_warnings("allextra")
set_defaultmode("debug")
set_license("GPL-3.0-or-later")

add_cxxflags(
    -- Warnings
    "-Wno-missing-field-initializers",

    -- UTF-8 charset
    "-finput-charset=UTF-8", "-fexec-charset=UTF-8"
)

add_defines(
    -- Platform detection macros ("or false" is needed because is_plat() can return nil)
    "OS_WINDOWS=" .. tostring(is_plat("windows") or false),
    "OS_MACOS=" .. tostring(is_plat("macosx") or false),
    "OS_LINUX=" .. tostring(is_plat("linux") or false),

    -- Windows currently has its own no_unique_address attribute
    "NO_UNIQUE_ADDRESS=" .. (is_plat("windows") and "[[msvc::no_unique_address]]" or "[[no_unique_address]]")
)

includes("external")

target("terminal-core")
    -- Project files
    set_kind("static")
    add_deps("external-std", "external-core")

    add_files(
        "src/net/*.mpp",
        "src/os/*.mpp|async.internal.mpp",
        "src/sockets/*.mpp",
        "src/sockets/delegates/*.mpp",
        "src/sockets/delegates/secure/*.mpp",
        "src/utils/*.mpp",
        { public = true }
    )
    add_files("src/os/async.internal.mpp")

    add_files(
        "src/net/*.cpp",
        "src/os/*.cpp",
        "src/sockets/delegates/secure/*.cpp",
        "src/utils/*.cpp"
    )

    -- Platform-specific files
    if is_plat("windows") then
        add_syslinks("Bthprops", "crypt32", "user32", "Ws2_32")
        add_files(
            "src/net/windows/*.cpp",
            "src/os/windows/*.cpp",
            "src/sockets/delegates/windows/*.cpp"
        )
        add_files("src/os/windows/*.mpp", { public = true })
    elseif is_plat("macosx") then
        add_files(
            "swift/bridge/cppbridge.cpp",
            "src/net/macos/*.cpp",
            "src/os/macos/*.cpp",
            "src/sockets/delegates/macos/*.cpp"
        )
        add_files("src/os/macos/*.mpp", { public = true })
    elseif is_plat("linux") then
        add_files(
            "src/net/linux/*.cpp",
            "src/os/linux/*.cpp",
            "src/sockets/delegates/linux/*.cpp"
        )
        add_files("src/os/linux/*.mpp", { public = true })
    end

target("terminal")
    add_packages("libsdl3")
    add_deps("external-std", "external-core", "external-app", "terminal-core")

    -- GUI code and main entry point
    add_files(
        "src/app/*.cpp", "src/app/*.mpp",
        "src/components/*.cpp", "src/components/*.mpp",
        "src/gui/*.cpp", "src/gui/*.mpp",
        "src/main.cpp"
    )
    add_configfiles("src/app/config.mpp.in")

    on_config(function (target)
        target:add("files", "$(buildir)/config.mpp")
    end)

    -- Add platform rules
    if is_plat("windows") then
        add_ldflags("-Wl,/SUBSYSTEM:WINDOWS")
        add_files("res/app.rc")
    elseif is_plat("macosx") then
        add_rules("xcode.application")
        add_files("res/Info.plist", "swift/bridge/guibridge.cpp")
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

    add_packages("catch2")
    add_deps("external-std", "external-core", "terminal-core")
    add_files("tests/src/**.cpp", "tests/src/**.mpp")

     -- Path to settings file
    add_defines(format("SETTINGS_FILE=R\"(%s)\"", path.absolute("tests/settings/settings.ini")))
