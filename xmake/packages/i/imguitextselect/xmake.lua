package("imguitextselect")
    set_homepage("https://github.com/AidanSun05/ImGuiTextSelect")
    set_description("Text selection implementation for Dear ImGui")
    set_license("MIT")

    add_urls("https://github.com/AidanSun05/ImGuiTextSelect/archive/v$(version).tar.gz")
    add_versions("1.1.1", "ed36cb7bdbe248a5e0d9f73977b24eeb14d051dd385b273d25e4617aac303c29")
    add_versions("1.1.0", "9464f5cdd118a77ecd64c21cad713ed4a729bae742750feb980d7c36e787d317")
    add_versions("1.0.0", "198184dc562a868e748606e1b88c708491f04762413ddcb2d2a251a1cba38a43")

    -- Config to match WhaleConnect
    add_deps("imgui docking", { configs = { glfw = true, opengl3 = true, freetype = true } })
    add_deps("utfcpp")

    on_install(function (package)
        local imgui = package:dep("imgui")
        local configs = imgui:requireinfo().configs
        if configs then
            configs = string.serialize(configs, { strip = true, indent = false })
        end

        io.writefile("xmake.lua", ([[
            add_rules("mode.debug", "mode.release")
            add_requires("imgui %s", { configs = %s })
            add_requires("utfcpp")
            set_languages("c++20")
            target("imguitextselect")
                add_packages("imgui", "utfcpp")
                set_kind("$(kind)")
                add_files("textselect.cpp")
                add_headerfiles("textselect.hpp")
        ]]):format(imgui:version_str(), configs))
        import("package.tools.xmake").install(package)
    end)

    on_test(function (package)
        assert(package:has_cxxtypes("TextSelect", { includes = "textselect.hpp", configs = { languages = "c++20" } }))
    end)
