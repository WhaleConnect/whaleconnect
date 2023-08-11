-- Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

add_repositories("xrepo-patches https://github.com/NSTerminal/xrepo-patches.git")

add_rules("mode.debug", "mode.release")

add_requires("imgui-with-sdl3 v20230807-docking", { configs = { sdl3_no_renderer = true, opengl3 = true, freetype = true } })
add_requires("libsdl3", { configs = { use_sdlmain = true } })
add_requires("catch2", "icu4c", "magic_enum", "nlohmann_json", "out_ptr", "opengl")

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
add_cxxflags("-Wno-subobject-linkage", { tools = { "gcc", "gxx" } })

-- Use MSVC Unicode character set and prevent clashing macros
add_defines("UNICODE", "_UNICODE", "NOMINMAX")

-- UTF-8 charset
add_cxxflags("cl::/utf-8")
add_cxxflags("-finput-charset=UTF-8", "-fexec-charset=UTF-8", { tools = { "clang", "clangxx", "gcc", "gxx" } })

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

    add_files("src/os/*.cpp", "src/sockets/*.cpp", "src/utils/*.cpp")

    -- Platform-specific files
    if is_plat("windows") then
        add_files("src/plat_windows/**.cpp")
    elseif is_plat("macosx") then
        add_files("src/plat_macos/**.cpp", "src/plat_macos/**.mm")
    elseif is_plat("linux") then
        add_files("src/plat_linux/**.cpp")
    end

target("terminal")
    add_packages("imgui-with-sdl3", "libsdl3", "opengl")
    add_deps("terminal-core")

    -- GUI code and main entry point
    add_files("src/gui/*.cpp", "src/main.cpp")

    -- Add application manifests
    if is_plat("windows") then
        add_files("res/app.manifest")
    elseif is_plat("macosx") then
        -- Generate application bundle
        add_rules("xcode.application")

        add_files("res/Info.plist")
    end

    add_ldflags("cl::/SUBSYSTEM:WINDOWS")

    -- Download font files next to executable on post-build
    before_build(function (target)
        local download_path = target:targetdir(), "fonts"
        local font_path = path.join(download_path, "NotoSansMono-Regular.ttf")
        local icon_font_path = path.join(download_path, "RemixIcon.ttf")
        local http = import("net.http")

        local font_url = "https://cdn.jsdelivr.net/gh/notofonts/notofonts.github.io/fonts/NotoSansMono/unhinted/ttf/NotoSansMono-Regular.ttf"
        local icon_font_url = "https://github.com/Remix-Design/RemixIcon/raw/f88a51b6402562c6c2465f61a3e845115992e4c6/fonts/remixicon.ttf"

        if not os.isfile(font_path) then
            print("Downloading Noto Sans Mono...")
            http.download(font_url, font_path)
        end

        if not os.isfile(icon_font_path) then
            print("Downloading Remix Icon...")
            http.download(icon_font_url, icon_font_path)
        end

        target:add("installfiles", format("%s/*.ttf", download_path))
    end)

target("socket-tests")
    set_default(false)

    add_packages("catch2", "nlohmann_json")
    add_deps("terminal-core")
    add_files("tests/src/*.cpp", "tests/src/helpers/*.cpp")

     -- Path to settings file
    add_defines(format("SETTINGS_FILE=R\"(%s)\"", path.absolute("tests/settings/settings.json")))
