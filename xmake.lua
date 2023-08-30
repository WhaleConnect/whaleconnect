-- Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

add_repositories("xrepo-patches https://github.com/NSTerminal/xrepo-patches.git")

add_rules("mode.debug", "mode.release")

-- Avoid linking to system libraries - prevents dependency mismatches on different platforms and makes package self-contained
add_requireconfs("*|opengl", { system = false })

add_requires("imgui-with-sdl3 v20230807-docking", { configs = { sdl3_no_renderer = true, opengl3 = true, freetype = true } })
add_requires("libsdl3", { configs = { use_sdlmain = true } })
add_requires("catch2", "icu4c", "magic_enum", "nlohmann_json", "opengl", "out_ptr")

if is_plat("linux") then
    add_requires("liburing", "bluez")
    add_requires("dbus", { configs = { system_bus_address = "unix:path=/run/dbus/system_bus_socket" } })
end

set_languages("c++20")
set_exceptions("cxx")
set_warnings("allextra")
set_defaultmode("debug")

add_packages("icu4c", "magic_enum", "out_ptr")

-- Ignored warnings
add_cxxflags("-Wno-missing-field-initializers", { tools = { "clang", "clangxx", "gcc", "gxx" } })

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

    add_files("src/*/*.cpp|app/*|gui/*|windows/*|**_windows.*|**_macos.*|**_linux.*")

    -- Platform-specific files
    if is_plat("windows") then
        add_files("src/**_windows.cpp")
    elseif is_plat("macosx") then
        add_files("src/**_macos.cpp", "src/**.mm")
    elseif is_plat("linux") then
        add_files("src/**_linux.cpp")
    end

target("terminal")
    add_packages("imgui-with-sdl3", "libsdl3", "opengl")
    add_deps("terminal-core")

    -- GUI code and main entry point
    add_files("src/app/*.cpp", "src/gui/*.cpp", "src/windows/*.cpp", "src/main.cpp")

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
    add_files("tests/src/**.cpp")

     -- Path to settings file
    add_defines(format("SETTINGS_FILE=R\"(%s)\"", path.absolute("tests/settings/settings.json")))
