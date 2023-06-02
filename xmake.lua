-- Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

add_rules("mode.debug", "mode.release")

add_requires("imgui v1.89.5-docking", { configs = { sdl2_no_renderer = true, opengl3 = true } })
add_requires("libsdl", { configs = { use_sdlmain = true } })
add_requires("catch2", "icu4c", "magic_enum", "nlohmann_json", "out_ptr", "opengl")

if is_plat("linux") then
    add_requires("liburing", "dbus", "bluez")
end

set_languages("c++20")
set_exceptions("cxx")
set_warnings("allextra")
set_defaultmode("debug")

add_packages("icu4c", "imgui", "libsdl", "magic_enum", "out_ptr", "opengl")

add_cxxflags("clang::-Wno-missing-field-initializers")

-- Use MSVC Unicode character set and prevent clashing macros
add_defines("UNICODE", "_UNICODE", "NOMINMAX")

-- Use Clang libc++ and enable experimental library
add_cxxflags("clang::-stdlib=libc++", "clang::-fexperimental-library")
add_ldflags("clangxx::-stdlib=libc++", "clangxx::-fexperimental-library")

-- Enable MSVC conformant preprocessor
add_cxxflags("cl::/Zc:preprocessor")

-- UTF-8 charset
add_cxxflags("cl::/utf-8", "clang::-finput-charset=UTF-8", "clang::-fexec-charset=UTF-8")

-- Platform detection macros ("or false" is needed because is_plat() can return nil)
add_defines("OS_WINDOWS=" .. tostring(is_plat("windows") or false),
            "OS_MACOS=" .. tostring(is_plat("macosx") or false),
            "OS_LINUX=" .. tostring(is_plat("linux") or false))

-- Include directories
add_includedirs("src")

-- Platform-specific links/compile options
if is_plat("windows") then
    add_syslinks("Ws2_32", "Bthprops")
elseif is_plat("macosx") then
    add_frameworks("Foundation", "IOBluetooth")
    set_values("objc++.build.arc", true)
elseif is_plat("linux") then
    add_packages("liburing", "dbus", "bluez")
end

target("terminal-core")
    -- Project files
    set_kind("object")

    add_files("src/gui/*.cpp", "src/os/*.cpp", "src/sockets/*.cpp", "src/utils/*.cpp")

    -- Platform-specific files
    if is_plat("windows") then
        add_files("src/plat_windows/*.cpp")
    elseif is_plat("macosx") then
        add_files("src/plat_macos/*.cpp", "src/plat_macos/*.mm")
    elseif is_plat("linux") then
        add_files("src/plat_linux/*.cpp")
    end

target("terminal")
    add_deps("terminal-core")

    -- Main entry point
    add_files("src/main.cpp")

    -- DPI awareness manifest (for Windows)
    if is_plat("windows") then
        add_files("res/dpi-aware.manifest")
    end

    add_ldflags("cl::/SUBSYSTEM:WINDOWS")

    -- Download font files next to executable on post-build
    after_build(function (target)
        local unifont_path = path.join(target:targetdir(), "unifont.otf")
        local font_awesome_path = path.join(target:targetdir(), "font-awesome.otf")

        local http = import("net.http")

        if not os.isfile(unifont_path) then
            print("Downloading GNU Unifont...")
            http.download("https://github.com/NSTerminal/unifont/raw/main/font/precompiled/unifont-15.0.01.otf", unifont_path)
        end

        if not os.isfile(font_awesome_path) then
            print("Downloading Font Awesome...")
            http.download("https://github.com/FortAwesome/Font-Awesome/raw/6.x/otfs/Font%20Awesome%206%20Free-Solid-900.otf", font_awesome_path)
        end
    end)

target("socket-tests")
    set_default(false)

    add_packages("catch2", "nlohmann_json")
    add_deps("terminal-core")
    add_files("tests/src/*.cpp", "tests/src/helpers/*.cpp")

    -- Path to settings file
    add_defines(format("SETTINGS_FILE=\"%s\"", path.absolute("tests/settings/settings.json")))
