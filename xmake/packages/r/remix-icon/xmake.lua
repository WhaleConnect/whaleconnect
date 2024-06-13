package("remix-icon")
    set_kind("library", { headeronly = true })
    set_homepage("https://github.com/Remix-Design/RemixIcon")
    set_description("Remix Icon font")
    set_license("Apache-2.0")

    add_urls("https://github.com/Remix-Design/RemixIcon/archive/refs/tags/v$(version).tar.gz")
    add_versions("4.3.0", "5bdfaf4863ca75fca22cb728a13741116ce7d2fc0b5b9a6d7261eb2453bc137d")

    on_install(function (package)
        os.cp("fonts/remixicon.ttf", package:installdir())
        package:addenv("REMIX_ICON_PATH", path.join(package:installdir(), "remixicon.ttf"))
    end)

    on_test(function (package)
        assert(os.isfile(package:getenv("REMIX_ICON_PATH")[1]))
    end)
