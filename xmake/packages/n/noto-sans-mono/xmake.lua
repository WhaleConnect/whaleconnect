package("noto-sans-mono")
    set_kind("library", { headeronly = true })
    set_homepage("https://github.com/notofonts/notofonts.github.io")
    set_description("Noto Sans Mono font")

    add_urls("https://github.com/notofonts/latin-greek-cyrillic/releases/download/NotoSansMono-v$(version)/NotoSansMono-v$(version).zip")
    add_versions("2.014", "090cf6c5e03f337a755630ca888b1fef463e64ae7b33ee134e9309c05f978732")

    on_install(function (package)
        os.cp("NotoSansMono/unhinted/ttf/NotoSansMono-Regular.ttf", package:installdir())
        os.cp("OFL.txt", package:installdir())
        package:addenv("NOTO_SANS_MONO_PATH", path.join(package:installdir(), "NotoSansMono-Regular.ttf"))
    end)

    on_test(function (package)
        assert(os.isfile(package:getenv("NOTO_SANS_MONO_PATH")[1]))
    end)
