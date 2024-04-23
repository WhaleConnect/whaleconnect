-- Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
-- SPDX-License-Identifier: GPL-3.0-or-later

function download_licenses(installdir)
    print("Downloading licenses...")

    -- Licenses are installed to Resources in macOS bundle
    local target_dir = is_plat("macosx") and path.join(installdir, "Contents", "Resources") or installdir

    local license_files = {
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
    for name, urls in pairs(license_files) do
        for _, url in ipairs(urls) do
            local filename = path.filename(url)
            local target_path = path.join(target_dir, "3rdparty", name, filename)

            if not os.isfile(target_path) then
                print("- %s -> %s", url, target_path)
                http.download(url, target_path)
            end
        end
    end
end
