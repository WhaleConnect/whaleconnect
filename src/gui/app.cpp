// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app.hpp"

#include <array>
#include <cmath>
#include <string>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>

#include "notifications.hpp"

#include "utils/handleptr.hpp"
#include "utils/settings.hpp"

static SDL_Window* window;      // The main application window
static SDL_GLContext glContext; // The OpenGL context

static float getScale() {
    return 1;
}

static void scaleToDPI() {
    ImGuiIO& io = ImGui::GetIO();
    float dpiScale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window));

    ImFontConfig config;
    float scale = getScale();

    config.SizePixels = Settings::fontSize * scale;
    float fontSize = std::floor(Settings::fontSize * dpiScale);

    if (io.Fonts->IsBuilt()) io.Fonts->Clear();

    static const HandlePtr<char, SDL_free> basePath{ SDL_GetBasePath() };
    static const std::string basePathStr{ basePath.get() };

    // Select glyphs for loading
    // Include all in Unicode plane 0 except for control characters (U+0000 - U+0019), surrogates (U+D800 - U+DFFF),
    // private use area (U+E000 - U+F8FF), and noncharacters (U+FFFE and U+FFFF).
    static const std::array<ImWchar, 5> ranges{ 0x0020, 0xD7FF, 0xF900, 0xFFFD, 0 };
    static const auto fontFile = basePathStr + "unifont.otf";

    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), fontSize, nullptr, ranges.data());

    // Load icons from Font Awesome
    static const std::array<ImWchar, 3> iconRanges{ 0xe000, 0xf8ff, 0 };
    static const auto faFontFile = basePathStr + "font-awesome.otf";

    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF(faFontFile.c_str(), fontSize, &config, iconRanges.data());

    io.FontGlobalScale = 1.0f / scale;
    io.Fonts->Build();

    ImGui::GetStyle().ScaleAllSizes(dpiScale);
}

// Sets Dear ImGui's configuration for use by the application.
static void configImGui() {
    using namespace Settings;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    // Disable imgui.ini
    // It can easily get plastered all over the filesystem and grow in size rapidly over time.
    io.IniFilename = nullptr;

    // Set styles
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = windowTransparency ? 0.92f : 1.0f;
    style.Colors[ImGuiCol_Tab].w = 0.0f;

    // Set corner rounding
    style.WindowRounding = roundedCorners ? 8.0f : 0.0f;

    // clang-format off
    style.ChildRounding = style.FrameRounding
                        = style.PopupRounding
                        = style.ScrollbarRounding
                        = style.GrabRounding
                        = style.TabRounding
                        = roundedCorners ? 4.0f : 0.0f;
    // clang-format on

    scaleToDPI();
}

bool App::init() {
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

bool App::newFrame() {
    // Poll for events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;
            case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                scaleToDPI();
                ImGui_ImplOpenGL3_CreateFontsTexture();
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::DrawNotifications();

#ifndef NDEBUG
    // The demo and metrics window are enabled in debug builds, provide a window to show them
    ImGui::Begin("Debug Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("This is a debug build for testing.");

    static bool showDemoWindow = false;
    static bool showMetricsWindow = false;
    static bool showStackToolWindow = false;
    ImGui::Checkbox("Show Demo Window", &showDemoWindow);
    ImGui::Checkbox("Show Metrics Window", &showMetricsWindow);
    ImGui::Checkbox("Show Stack Tool Window", &showStackToolWindow);

    if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
    if (showMetricsWindow) ImGui::ShowMetricsWindow(&showMetricsWindow);
    if (showStackToolWindow) ImGui::ShowStackToolWindow(&showStackToolWindow);

    // Buttons to add notifications with different timeouts and icons
    if (ImGui::Button("Test Notification (3s)"))
        ImGui::AddNotification("Test Notification (3s)", NotificationType::Info, 3);

    if (ImGui::Button("Test Notification (5s)"))
        ImGui::AddNotification("Test Notification (5s)", NotificationType::Success, 5);

    ImGui::End();
#endif

    return true;
}

void App::render() {
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

void App::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
