// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <format> // std::format() [C++20]
#include <fstream> // std::ofstream
#include <chrono> // std::chrono

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "mainhandle.hpp"
#include "imguiext.hpp"
#include "util.hpp"

// The GUI window of the app
static GLFWwindow* window = nullptr;

/// <summary>
/// Set the configuration for Dear ImGui.
/// </summary>
static void configImGui() {
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

	// Set styles
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0;
	style.TabRounding = 0;
	style.ScrollbarRounding = 0;
	style.Colors[ImGuiCol_WindowBg].w = 1;

	// Load font file
	ImFontAtlas* fonts = io.Fonts;
	fonts->AddFontFromFileTTF("3rdparty/unifont/font/precompiled/unifont-13.0.06.ttf", Settings::fontSize);
}

bool initApp(int width, int height, const char* title) {
	glfwSetErrorCallback([](int error, const char* description) {
		// Error file for logging GLFW errors
		std::ofstream file("err.txt", std::ios::app);

		// Add the error to the file with timestamp, name, and description
		file << std::format("[{:%F %T}] [GLFW] Error {}: {}\n", std::chrono::system_clock::now(), error, description);
	});

	// Init GLFW
	if (!glfwInit()) return false;

	// Decide GL versions
#ifdef __APPLE__
	// GL 3.2
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
	// GL 3.0
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

	// Create window
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (window == nullptr) return false;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Set glad loader
	gladSetGLOnDemandLoader(glfwGetProcAddress);

	// Setup Dear ImGui context and backends/renderers
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	configImGui();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	// Initialization was successful
	return true;
}

bool isActive() {
	return !glfwWindowShouldClose(window);
}

void handleNewFrame() {
	glfwPollEvents();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Dockspace
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void renderWindow() {
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

void cleanupApp() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
}
