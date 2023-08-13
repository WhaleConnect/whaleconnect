function download_fonts(targetdir)
    local font_path = path.join(targetdir, "NotoSansMono-Regular.ttf")
    local icon_font_path = path.join(targetdir, "RemixIcon.ttf")
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
end
