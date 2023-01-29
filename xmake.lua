-- Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

add_rules("mode.debug", "mode.release")

set_languages("c++20")
set_policy("check.auto_ignore_flags", false)
set_defaultmode("debug")

add_requires("imgui v1.89-docking", { configs = { sdl2_opengl3 = true } })
add_requires("libsdl", { configs = { use_sdlmain = true } })
add_requires("magic_enum", "out_ptr")

if is_plat("linux") then
    add_requires("liburing")
    add_requires("dbus")
end

target("terminal")
    add_packages("imgui", "libsdl", "magic_enum", "out_ptr")

    set_kind("binary")
    set_exceptions("cxx")
    set_warnings("allextra")
    add_cxxflags("clang::-Wno-missing-field-initializers")

    on_config(function (target)
        if target:has_tool("cxx", "cl") then
            -- Use MSVC Unicode character set and prevent clashing macros
            target:add("defines", "UNICODE", "_UNICODE", "NOMINMAX")
        else
            -- Use Clang libc++ and enable experimental library
            target:add("cxxflags", "-stdlib=libc++")
            target:add("ldflags", "-stdlib=libc++")
            target:add("cxxflags", "-fexperimental-library")
            target:add("ldflags", "-fexperimental-library")
        end
    end)

    -- Download GNU Unifont file next to executable on post-build
    after_build(function (target)
        local unifont_path = path.join(target:targetdir(), "unifont.otf")

        if not os.isfile(unifont_path) then
            print("Downloading GNU Unifont...")
            import("net.http").download("https://github.com/NSTerminal/unifont/raw/main/font/precompiled/unifont-15.0.01.otf", unifont_path)
        end
    end)

    -- Enable MSVC conformant preprocessor
    add_cxxflags("cl::/Zc:preprocessor")

    -- UTF-8 charset
    add_cxxflags("cl::/utf-8", "clang::-finput-charset=UTF-8", "clang::-fexec-charset=UTF-8")

    -- Platform detection macros ("or false" is needed because is_plat() can return nil)
    add_defines("OS_WINDOWS=" .. tostring(is_plat("windows") or false),
                "OS_APPLE=" .. tostring(is_plat("macosx") or false),
                "OS_LINUX=" .. tostring(is_plat("linux") or false))

    -- Project files
    add_includedirs("src")
    add_files("src/gui/*.cpp", "src/os/*.cpp", "src/sockets/*.cpp", "src/utils/*.cpp", "src/main.cpp")

    if is_plat("windows") then
        add_files("src/plat_windows/*.cpp")

        add_syslinks("Ws2_32", "Bthprops", "opengl32")
        add_ldflags("/SUBSYSTEM:WINDOWS")
    elseif is_plat("macosx") then
        add_files("src/plat_macos/*.cpp", "src/plat_macos/*.mm")

        add_frameworks("Foundation", "IOBluetooth", "OpenGL")
        set_values("objc++.build.arc", true)
    elseif is_plat("linux") then
        add_files("src/plat_linux/*.cpp")

        add_packages("liburing", "dbus")
    end
