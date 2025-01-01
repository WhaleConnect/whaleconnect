-- Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

set_version("1.0.2", { build = "%Y%m%d%H%M" })
add_repositories("additional-deps ./xmake")
set_policy("package.requires_lock", true)
set_license("GPL-3.0-or-later")

add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

-- Avoid linking to system libraries - prevents dependency mismatches on different platforms and makes package self-contained
add_requireconfs("*|opengl", { system = false })
add_requireconfs("imgui.freetype", { system = false })

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

local imgui_version = "v1.91.6-docking"
local imgui_configs = { glfw = true, opengl3 = true, freetype = true }
add_requires(("imgui %s"):format(imgui_version), { configs = imgui_configs })
add_requires("catch2", "glfw", "imguitextselect", "opengl", "out_ptr", "utfcpp", "noto-sans-mono", "remix-icon")
add_requireconfs("imguitextselect.imgui", { override = true, version = imgui_version, configs = imgui_configs })

add_packages("botan", "out_ptr")

if is_plat("windows") then
    -- Use MSVC Unicode character set and prevent clashing macros
    add_defines("UNICODE", "_UNICODE", "NOMINMAX", "_CRT_SECURE_NO_WARNINGS")
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
add_cxxflags("-Wno-missing-field-initializers", { tools = { "gcc", "gxx", "clang", "clangxx" } })

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

target("swift")
    set_kind("phony")

    on_build("macosx", function(target)
        local buildMode = import("xmake.swift_settings").getSwiftBuildMode()
        os.cd("$(scriptdir)/swift")
        os.exec("swift build -c %s --build-path $(buildir)/swift --config-path $(buildir)/swift", buildMode)
    end)

rule("swift-deps")
    on_load("macosx", function(target)
        local buildDir, libDir, linkDir = import("xmake.swift_settings").getSwiftSettings()

        target:add("includedirs",
            libDir,
            path.join(buildDir, "GUIMacOS.build"),
            path.join(buildDir, "BluetoothMacOS.build"))
        target:add("linkdirs", buildDir, linkDir)
        target:add("links", "BluetoothMacOS", "GUIMacOS")
    end)

target("core")
    -- Project files
    set_kind("object")
    add_deps("swift")
    add_rules("swift-deps")

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
            "src/os/bluetooth.cpp",
            "src/sockets/delegates/macos/*.cpp"
        )
    elseif is_plat("linux") then
        add_files(
            "src/net/btutils.linux.cpp",
            "src/os/async.linux.cpp",
            "src/sockets/delegates/linux/*.cpp"
        )
    end

target("WhaleConnect")
    add_deps("core", "swift")
    add_rules("swift-deps")
    add_packages("glfw", "imgui", "imguitextselect", "noto-sans-mono", "opengl", "remix-icon", "utfcpp")

    -- GUI code and main entry point
    add_files("src/app/*.cpp", "src/components/*.cpp", "src/gui/*.cpp", "src/main.cpp")
    add_configfiles("src/app/config.hpp.in")
    set_configvar("RESDIR", string.format("%s/res", string.gsub(os.scriptdir(), "\\", "/")))
    add_includedirs("$(buildir)")

    -- Add platform rules
    if is_plat("windows") then
        add_rules("win.sdk.application")
        add_configfiles("res/app.rc.in")
    elseif is_plat("macosx") then
        add_rules("xcode.application")
        add_configfiles("res/Info.plist.in")
        add_installfiles("res/icons/app.icns")
    elseif is_plat("linux") then
        add_rpathdirs("$ORIGIN/../lib", { installonly = true })
    end

    after_load(function (target)
        -- Install font files and license next to executable
        -- Fonts are embedded in the Resources directory in the macOS bundle.
        local prefixdir = is_plat("macosx") and "" or "bin"
        target:add("installfiles", os.getenv("NOTO_SANS_MONO_PATH"), { prefixdir = prefixdir })
        target:add("installfiles", os.getenv("REMIX_ICON_PATH"), { prefixdir = prefixdir })
        target:add("installfiles", "COPYING")
    end)

    on_config(function (target)
        if is_plat("windows") then
            target:add("files", "$(buildir)/app.rc")
        elseif is_plat("macosx") then
            target:add("files", "$(buildir)/Info.plist")
        end
    end)

    -- Download font files next to executable on pre-build
    before_build(function (target)
        import("xmake.download").downloadFonts(target:targetdir())
    end)

    after_install(function (target)
        import("xmake.download").downloadLicenses(target:installdir())
    end)

target("socket-tests")
    set_default(false)

    add_packages("catch2")
    add_deps("core")
    add_rules("swift-deps")
    add_files("tests/src/**.cpp")

     -- Path to settings file
    add_defines(format("SETTINGS_FILE=R\"(%s)\"", path.absolute("tests/settings/settings.ini")))

target("benchmark-server")
    set_default(false)

    add_deps("core")
    add_rules("swift-deps")
    add_files("tests/benchmarks/server.cpp")
