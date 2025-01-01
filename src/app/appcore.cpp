// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#undef GLFW_INCLUDE_NONE

#include "appcore.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <numeric>
#include <string>

#include <config.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "fs.hpp"
#include "settings.hpp"
#include "gui/notifications.hpp"

bool doConfig = true;
GLFWwindow* window; // The main application window

// Scales the app and fonts to the screen's DPI.
void loadFont(GLFWwindow*, float scaleX, float scaleY) {
    // https://github.com/ocornut/imgui/issues/5301
    // https://github.com/ocornut/imgui/issues/6485
    // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-how-should-i-handle-dpi-in-my-application

    int fbX, fbY;
    glfwGetFramebufferSize(window, &fbX, &fbY);

    int windowX, windowY;
    glfwGetWindowSize(window, &windowX, &windowY);

    auto pixelRatioX = static_cast<float>(fbX) / windowX;
    auto pixelRatioY = static_cast<float>(fbY) / windowY;

    float contentScale = std::midpoint(scaleX, scaleY);
    float pixelRatio = std::midpoint(pixelRatioX, pixelRatioY);
    float zoomFactor = std::midpoint(scaleX / pixelRatioX, scaleY / pixelRatioY);

    // The icons are slightly larger than the main font so they are scaled down from the font size
    float fontSize = std::floor(Settings::Font::size * contentScale);
    float iconFontSize = std::floor(fontSize * 0.9f);

    ImGuiIO& io = ImGui::GetIO();
    ImFontAtlas& fonts = *io.Fonts;

    // Clear built fonts to save memory
    bool fontsBuilt = false;
    if (fonts.IsBuilt()) {
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        fontsBuilt = true;
        fonts.Clear();
    }

    // Select glyphs for loading
    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    auto fontRanges = Settings::Font::ranges;
    fontRanges.push_back(0); // Add null terminator to configured ranges
    builder.AddRanges(fontRanges.data());
    builder.AddChar(0xFFFD); // Substitution character
    builder.BuildRanges(&ranges);

    const auto basePath = AppFS::getBasePath();
    const std::filesystem::path configuredFontFile = Settings::Font::file;
    const auto fontFile = configuredFontFile.empty() ? basePath / "NotoSansMono-Regular.ttf" : configuredFontFile;

    if (std::filesystem::is_regular_file(fontFile)) {
        fonts.AddFontFromFileTTF(fontFile.string().c_str(), fontSize, nullptr, ranges.Data);
    } else {
        ImFontConfig config;
        config.SizePixels = fontSize;
        fonts.AddFontDefault(&config);
        ImGuiExt::addNotification("Font file not found: " + fontFile.string(), NotificationType::Error, 0);
    }

    // Load icons
    static const std::array<ImWchar, 3> iconRanges{ 0xE000, 0xF8FF, 0 };
    static const auto iconFontFile = basePath / "remixicon.ttf";

    ImFontConfig config;
    config.SizePixels = Settings::Font::size * pixelRatio;
    config.MergeMode = true;
    fonts.AddFontFromFileTTF(iconFontFile.string().c_str(), iconFontSize, &config, iconRanges.data());

    // Scale fonts and rebuild
    io.FontGlobalScale = 1.0f / pixelRatio;
    fonts.Build();

    // Scale sizes to zoom factor
    ImGui::GetStyle().ScaleAllSizes(zoomFactor);

    if (fontsBuilt) ImGui_ImplOpenGL3_CreateFontsTexture();
}

// Sets Dear ImGui's configuration for use by the application.
void configImGui() {
    // Set styles
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = Settings::GUI::windowTransparency ? 0.92f : 1.0f;
    style.Colors[ImGuiCol_Tab].w = 0.0f;

    // Set corner rounding
    bool roundedCorners = Settings::GUI::roundedCorners;
    style.WindowRounding = roundedCorners ? 8.0f : 0.0f;

    style.ChildRounding = style.FrameRounding = style.PopupRounding = style.ScrollbarRounding = style.GrabRounding
        = style.TabRounding = roundedCorners ? 4.0f : 0.0f;
}

void drawDebugTools() {
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
}

bool AppCore::init() {
    Settings::load();

    // Set up GLFW
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create window
    window = glfwCreateWindow(1280, 720, "WhaleConnect", nullptr, nullptr);
    if (!window) return false;

    glfwSetWindowContentScaleCallback(window, loadFont);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Set up Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard // Enable Keyboard Controls
        | ImGuiConfigFlags_DockingEnable // Enable Docking
        | ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // Disable imgui.ini
    // It can easily get plastered all over the filesystem and grow in size rapidly over time.
    io.IniFilename = nullptr;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    return true;
}

void AppCore::configOnNextFrame() {
    doConfig = true;
}

bool AppCore::newFrame() {
    if (glfwWindowShouldClose(window)) return false;

    // Poll for events
    glfwPollEvents();

    // Edit configuration before the new frame
    if (doConfig) {
        float scaleX, scaleY;
        glfwGetWindowContentScale(window, &scaleX, &scaleY);

        loadFont(nullptr, scaleX, scaleY);
        configImGui();
        doConfig = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGuiExt::drawNotifications();

    if constexpr (Config::debug == 1) drawDebugTools();

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
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}

void AppCore::cleanup() {
    Settings::save();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
