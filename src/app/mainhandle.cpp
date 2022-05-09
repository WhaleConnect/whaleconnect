// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono> // std::chrono

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>

#include <GLFW/glfw3.h>

#include <fmt/os.h>
#include <fmt/chrono.h>

#include "mainhandle.hpp"
#include "settings.hpp"
#include "sys/filesystem.hpp"
#include "util/imguiext.hpp"

// The GUI window of the app
static GLFWwindow* window = nullptr;

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
    static const ImWchar ranges[] = { 0x0020, 0xD7FF, 0xF900, 0xFFFD, 0 };
    static auto fontFile = System::getProgramDir() / "unifont.otf";
    io.Fonts->AddFontFromFileTTF(fontFile.string().c_str(), fontSize, nullptr, ranges);
}

bool MainHandler::initApp() {
    // Set an error callback for GLFW so we know when something goes wrong
    glfwSetErrorCallback([](int error, const char* description) {
        // Error file for logging GLFW errors
        auto file = fmt::output_file("err.txt", fmt::file::CREATE | fmt::file::APPEND | fmt::file::WRONLY);

        // Add the error to the file with timestamp, name, and description
        file.print("[{:%F %T}] [GLFW] Error {}: {}\n", std::chrono::system_clock::now(), error, description);
    });

    // Init GLFW
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window
    window = glfwCreateWindow(1280, 720, "Network Socket Terminal", nullptr, nullptr);
    if (!window) return false;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context and backends/renderers
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    configImGui();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Initialization was successful
    return true;
}

bool MainHandler::isActive() {
    return !glfwWindowShouldClose(window);
}

void MainHandler::handleNewFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // FPS counter
    if (Settings::showFPScounter)
        ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopRight, "%d FPS", static_cast<int>(ImGui::GetIO().Framerate));

#ifndef NDEBUG
    // The demo and metrics window are enabled in debug builds (see imconfig.h), provide a window to show them
    static bool showDebugTools = true;

    if (showDebugTools) {
        ImGui::Begin("Debug Tools", &showDebugTools, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("This is a debug build for testing.");
        ImGui::Text("Closing this window hides it until the next launch!");

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
    }
#endif
}

void MainHandler::renderWindow() {
    // Render the main application window
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Render multi-viewport platform windows
    GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backupCurrentContext);

    glfwSwapBuffers(window);
}

void MainHandler::cleanupApp() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
