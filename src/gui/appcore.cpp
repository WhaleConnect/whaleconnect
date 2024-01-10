// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <nlohmann/json.hpp> // IWYU pragma: keep (fixes errors on MSVC)
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>

module gui.appcore;
import gui.settings;
import gui.notifications;
import utils.handleptr;

SDL_Window* window; // The main application window
SDL_GLContext glContext; // The OpenGL context

const std::string settingsFilePath = [] {
    const HandlePtr<char, SDL_free> prefPath{ SDL_GetPrefPath("NSTerminal", "terminal") };
    const std::string prefPathStr{ prefPath.get() };
    return prefPathStr + "settings.json";
}();

// Scales the app and fonts to the screen's DPI.
void scaleToDPI() {
    // https://github.com/ocornut/imgui/issues/5301
    // https://github.com/ocornut/imgui/issues/6485
    // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-how-should-i-handle-dpi-in-my-application
    // https://github.com/libsdl-org/SDL/blob/main/docs/README-highdpi.md

    ImGuiIO& io = ImGui::GetIO();
    float dpiScale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window));

    float scale = SDL_GetWindowPixelDensity(window);

    // Font size
    auto configuredFontSize = Settings::getSetting<uint8_t>("font.size");
    ImFontConfig config;
    config.SizePixels = configuredFontSize * scale;

    // The icons are slightly larger than the main font so they are scaled down from the font size
    float fontSize = std::floor(configuredFontSize * dpiScale * scale);
    float iconFontSize = std::floor(fontSize * 0.9f);

    ImFontAtlas& fonts = *io.Fonts;

    // Clear built fonts to save memory
    if (fonts.IsBuilt()) fonts.Clear();

    // Path to font files
    static const HandlePtr<char, SDL_free> basePath{ SDL_GetBasePath() };
    static const std::string basePathStr{ basePath.get() };

    // Select glyphs for loading
    static const ImVector<ImWchar> ranges = [&] {
        ImVector<ImWchar> rangesOut;
        ImFontGlyphRangesBuilder builder;

        auto fontRanges = Settings::getSetting<std::vector<ImWchar>>("font.ranges");
        fontRanges.push_back(0);
        builder.AddRanges(fontRanges.data());
        builder.AddChar(0xFFFD); // Substitution character

        builder.BuildRanges(&rangesOut);
        return rangesOut;
    }();

    static const auto configuredFontFile = Settings::getSetting<std::string>("font.file");
    static const auto fontFile = configuredFontFile.empty() ? basePathStr + "NotoSansMono-Regular.ttf" : configuredFontFile;
    fonts.AddFontFromFileTTF(fontFile.c_str(), fontSize, nullptr, ranges.Data);

    // Load icons
    static const std::array<ImWchar, 3> iconRanges{ 0xe000, 0xf8ff, 0 };
    static const auto iconFontFile = basePathStr + "RemixIcon.ttf";

    // Merge icons into main font
    config.MergeMode = true;
    fonts.AddFontFromFileTTF(iconFontFile.c_str(), iconFontSize, &config, iconRanges.data());

    // Scale fonts and rebuild
    io.FontGlobalScale = 1.0f / scale;
    fonts.Build();

    // Scale sizes to DPI scale
    ImGui::GetStyle().ScaleAllSizes(dpiScale);
}

// Sets Dear ImGui's configuration for use by the application.
void configImGui() {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard // Enable Keyboard Controls
        | ImGuiConfigFlags_DockingEnable // Enable Docking
        | ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // Disable imgui.ini
    // It can easily get plastered all over the filesystem and grow in size rapidly over time.
    io.IniFilename = nullptr;

    // Set styles
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = Settings::getSetting<bool>("gui.windowTransparency") ? 0.92f : 1.0f;
    style.Colors[ImGuiCol_Tab].w = 0.0f;

    // Set corner rounding
    auto roundedCorners = Settings::getSetting<bool>("gui.roundedCorners");
    style.WindowRounding = roundedCorners ? 8.0f : 0.0f;

    style.ChildRounding = style.FrameRounding = style.PopupRounding = style.ScrollbarRounding = style.GrabRounding
        = style.TabRounding = roundedCorners ? 4.0f : 0.0f;

    scaleToDPI();
}

bool AppCore::init() {
    Settings::load(settingsFilePath);

    // Set up SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Initialization Error", SDL_GetError(), nullptr);
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    window = SDL_CreateWindow("Network Socket Terminal", 1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    // Create context
    glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Set up Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    configImGui();
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init();

    return true;
}

bool AppCore::newFrame() {
    // Poll for events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;
            case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
                // Rescale app on DPI change
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                scaleToDPI();
                ImGui_ImplOpenGL3_CreateFontsTexture();
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGuiExt::drawNotifications();

#ifndef NDEBUG
    // The demo and metrics window are enabled in debug builds, provide a window to show them
    ImGui::Begin("Debug Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("This is a debug build for testing.");

    static bool showDemoWindow = false;
    static bool showMetricsWindow = false;
    static bool showIDStackToolWindow = false;
    ImGui::Checkbox("Show Demo Window", &showDemoWindow);
    ImGui::Checkbox("Show Metrics Window", &showMetricsWindow);
    ImGui::Checkbox("Show Stack Tool Window", &showIDStackToolWindow);

    if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
    if (showMetricsWindow) ImGui::ShowMetricsWindow(&showMetricsWindow);
    if (showIDStackToolWindow) ImGui::ShowIDStackToolWindow(&showIDStackToolWindow);

    // Buttons to add notifications with different timeouts and icons
    if (ImGui::Button("Test Notification (3s)"))
        ImGuiExt::addNotification("Test Notification (3s)", NotificationType::Info, 3);

    if (ImGui::Button("Test Notification (5s)"))
        ImGuiExt::addNotification("Test Notification (5s)", NotificationType::Success, 5);

    ImGui::End();
#endif

    return true;
}

void AppCore::render() {
    // Render the main application window
    const ImVec2& displaySize = ImGui::GetIO().DisplaySize;

    ImGui::Render();
    glViewport(0, 0, static_cast<int>(displaySize.x), static_cast<int>(displaySize.y));
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Render multi-viewport platform windows
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SwapWindow(window);
}

void AppCore::cleanup() {
    Settings::save(settingsFilePath);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
