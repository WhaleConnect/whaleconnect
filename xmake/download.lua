-- Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

function downloadLicenses(installdir)
    print("Downloading licenses...")

    -- Licenses are installed to Resources in macOS bundle
    local targetDir = is_plat("macosx") and path.join(installdir, "Contents", "Resources") or installdir

    local licenseFiles = {
        -- Libraries
        ["bluez"] = { "https://raw.githubusercontent.com/bluez/bluez/master/LICENSES/preferred/GPL-2.0" },
        ["botan"] = { "https://raw.githubusercontent.com/randombit/botan/master/license.txt" },
        ["catch2"] = { "https://raw.githubusercontent.com/catchorg/Catch2/devel/LICENSE.txt" },
        ["dear_imgui"] = { "https://raw.githubusercontent.com/ocornut/imgui/master/LICENSE.txt" },
        ["glfw"] = { "https://raw.githubusercontent.com/glfw/glfw/master/LICENSE.md" },
        ["imguitextselect"] = { "https://raw.githubusercontent.com/AidanSun05/ImGuiTextSelect/main/LICENSE.txt" },
        ["libdbus"] = { "https://gitlab.freedesktop.org/dbus/dbus/-/raw/master/LICENSES/GPL-2.0-or-later.txt" },
        ["liburing"] = {
            "https://raw.githubusercontent.com/axboe/liburing/master/LICENSE",
            "https://raw.githubusercontent.com/axboe/liburing/master/COPYING",
            "https://raw.githubusercontent.com/axboe/liburing/master/COPYING.GPL"
        },
        ["utfcpp"] = { "https://raw.githubusercontent.com/nemtrif/utfcpp/master/LICENSE" },
        ["ztd.out_ptr"] = { "https://raw.githubusercontent.com/soasis/out_ptr/main/LICENSE" },

        -- Fonts
        ["noto_sans_mono"] = { "https://raw.githubusercontent.com/notofonts/noto-fonts/main/LICENSE" },
        ["remix_icon"] = { "https://raw.githubusercontent.com/Remix-Design/RemixIcon/master/License" }
    }

    local http = import("net.http")
    for name, urls in pairs(licenseFiles) do
        for _, url in ipairs(urls) do
            local filename = path.filename(url)
            local targetPath = path.join(targetDir, "3rdparty", name, filename)

            if not os.isfile(targetPath) then
                print("- %s -> %s", url, targetPath)
                http.download(url, targetPath)
            end
        end
    end
end

function downloadFonts(targetdir)
    local fontPath = path.join(targetdir, "NotoSansMono-Regular.ttf")
    local iconFontPath = path.join(targetdir, "remixicon.ttf")

    if not os.isfile(fontPath) then
        os.ln(os.getenv("NOTO_SANS_MONO_PATH"), fontPath)
    end

    if not os.isfile(iconFontPath) then
        os.ln(os.getenv("REMIX_ICON_PATH"), iconFontPath)
    end
end
