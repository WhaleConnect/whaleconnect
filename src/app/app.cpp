// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include <SDL.h>
#include <SDL_filesystem.h>
#include <SDL_opengl.h>

#include "app.hpp"
#include "settings.hpp"
#include "sys/handleptr.hpp"
#include "util/imguiext.hpp"

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

    style.ChildRounding
        = style.FrameRounding
        = style.PopupRounding
        = style.ScrollbarRounding
        = style.GrabRounding
        = style.TabRounding
        = roundedCorners ? 4.0f : 0.0f;

    // If the default font is used, the rest of this function can be skipped
    if (useDefaultFont) return;

    // Select glyphs for loading
    // Include all in Unicode plane 0 except for control characters (U+0000 - U+0019), surrogates (U+D800 - U+DFFF),
    // private use area (U+E000 - U+F8FF), and noncharacters (U+FFFE and U+FFFF).
    static ImWchar ranges[]{ 0x0020, 0xD7FF, 0xF900, 0xFFFD, 0 };
    static HandlePtr<char, SDL_free> basePath{ SDL_GetBasePath() };
    static auto fontFile = std::string{ basePath.get() } + "unifont.otf";

    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), fontSize, nullptr, ranges);
}

App::SDLData App::init() {
    // Set up SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Initialization Error", SDL_GetError(), nullptr);
        return { nullptr, nullptr };
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    auto window = SDL_CreateWindow("Network Socket Terminal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    // Create context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Set up Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    configImGui();
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init();

    return { window, glContext };
}

bool App::newFrame() {
    // Poll for events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) return false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // FPS counter
    if (Settings::showFPScounter)
        ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopRight, "%.0f FPS", ImGui::GetIO().Framerate);

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
    ImGui::End();
#endif

    return true;
}

void App::render(const SDLData& sdlData) {
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
    SDL_GL_MakeCurrent(sdlData.window, sdlData.glContext);
    SDL_GL_SwapWindow(sdlData.window);
}

void App::cleanup(const SDLData& sdlData) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(sdlData.glContext);
    SDL_DestroyWindow(sdlData.window);
    SDL_Quit();
}
