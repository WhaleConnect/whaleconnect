-- Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

set_version("0.4.0", { build = "%Y%m%d%H%M" })
add_repositories("additional-deps ./xmake")
set_license("GPL-3.0-or-later")

add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

-- Avoid linking to system libraries - prevents dependency mismatches on different platforms and makes package self-contained
add_requireconfs("*|opengl", { system = false })

add_requires("botan", { configs = { modules = (function()
    local certstoreSystem = "certstor_flatfile"
    if is_plat("windows") then
        certstoreSystem = "certstor_system_windows"
    elseif is_plat("macosx") then
        certstoreSystem = "certstor_system_macos"
    end

    return {
        "certstor_system", -- System certificate store
        "chacha20poly1305", -- Useful cipher suite for TLS
        "http_util", -- Online OCSP checking
        "system_rng", -- Random number generator
        "tls13", -- TLS 1.3
        certstoreSystem
    }
end)() } })
add_requires("imgui docking", { configs = { glfw = true, opengl3 = true, freetype = true } })
add_requires("catch2", "glfw", "imguitextselect", "opengl", "out_ptr", "utfcpp")

add_packages("botan", "out_ptr")

if is_plat("windows") then
    -- Use MSVC Unicode character set and prevent clashing macros
    add_defines("UNICODE", "_UNICODE", "NOMINMAX")
elseif is_plat("linux") then
    add_requires("liburing", "bluez")
    add_requires("dbus", { configs = { system_bus_address = "unix:path=/run/dbus/system_bus_socket" } })
    add_packages("liburing", "dbus", "bluez")
end

set_languages("c++23")
set_exceptions("cxx")
set_encodings("utf-8")

-- Warnings
set_warnings("allextra")
add_cxxflags("-Wno-missing-field-initializers", { tools = { "gcc", "clang" } })

add_cxxflags("clang::-fexperimental-library")
add_ldflags("clangxx::-fexperimental-library")

add_includedirs("src")

add_defines(
    -- Platform detection macros ("or false" is needed because is_plat can return nil)
    "OS_WINDOWS=" .. tostring(is_plat("windows") or false),
    "OS_MACOS=" .. tostring(is_plat("macosx") or false),
    "OS_LINUX=" .. tostring(is_plat("linux") or false),

    -- Windows currently has its own no_unique_address attribute
    "NO_UNIQUE_ADDRESS=" .. (is_plat("windows") and "[[msvc::no_unique_address]]" or "[[no_unique_address]]")
)

local swiftBuildMode = is_mode("release") and "release" or "debug"
local swiftBuildDir = format("$(scriptdir)/swift/.build/%s", swiftBuildMode)
if is_plat("macosx") then
    local swiftLibDir = "/Library/Developer/CommandLineTools/usr/lib/swift"
    add_includedirs(swiftLibDir)
    add_linkdirs(swiftBuildDir, path.join(swiftLibDir, "macosx"))
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

    if is_plat("macosx") then
        add_deps("swift")
        add_includedirs(path.join(swiftBuildDir, "BluetoothMacOS.build"), { public = true })
        add_links("BluetoothMacOS")
    end

    add_files(
        "src/net/netutils.cpp",
        "src/os/async.cpp", "src/os/error.cpp",
        "src/sockets/delegates/secure/*.cpp",
        "src/utils/*.cpp"
    )

    -- Platform-specific files
    if is_plat("windows") then
        add_syslinks("Bthprops", "crypt32", "user32", "Ws2_32")
        add_files(
            "src/net/btutils.windows.cpp",
            "src/os/async.windows.cpp",
            "src/sockets/delegates/windows/*.cpp"
        )
    elseif is_plat("macosx") then
        add_files(
            "src/net/btutils.macos.cpp",
            "src/os/async.macos.cpp",
            "src/sockets/delegates/macos/*.cpp"
        )
    elseif is_plat("linux") then
        add_files(
            "src/net/btutils.linux.cpp",
            "src/os/async.linux.cpp",
            "src/sockets/delegates/linux/*.cpp"
        )
    end

target("terminal")
    add_packages("glfw")
    add_deps("terminal-core")
    add_packages("glfw", "imgui", "imguitextselect", "opengl", "utfcpp")

    if is_plat("macosx") then
        add_deps("swift")
        add_includedirs(path.join(swiftBuildDir, "GUIMacOS.build"))
        add_links("GUIMacOS")
    end

    -- GUI code and main entry point
    add_files("src/app/*.cpp", "src/components/*.cpp", "src/gui/*.cpp", "src/main.cpp")
    add_configfiles("src/app/config.hpp.in")
    add_includedirs("$(buildir)")

    -- Add platform rules
    if is_plat("windows") then
        add_rules("win.sdk.application")
        add_files("res/app.rc")
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
        import("xmake.download").download_fonts(target:targetdir())
    end)

    after_install(function (target)
        import("xmake.download").download_licenses(target:installdir())
    end)

target("socket-tests")
    set_default(false)

    add_packages("catch2")
    add_deps("terminal-core")
    add_files("tests/src/**.cpp")

     -- Path to settings file
    add_defines(format("SETTINGS_FILE=R\"(%s)\"", path.absolute("tests/settings/settings.ini")))

target("benchmark-server")
    set_default(false)

    add_deps("terminal-core")
    add_files("tests/benchmarks/server.cpp")
