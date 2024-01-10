// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <format>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>

#include <imgui.h>
#include <nlohmann/json.hpp> // IWYU pragma: keep (fixes errors on MSVC)
#include <SDL3/SDL_main.h>

import components.windowlist;
import gui.appcore;
import gui.imguiext;
import gui.newconnbt;
import gui.newconnip;
import gui.newserver;
import gui.notifications;
import gui.settings;
import net.btutils;
import net.device;
import net.enums;
import os.async;
import os.error;

// Draws the new connection window.
void drawNewConnectionWindow(bool& open, WindowList& connections, WindowList& sdpWindows) {
    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(40_fh * 11_fh, ImGuiCond_Appearing);

    if (ImGui::Begin("New Connection", &open) && ImGui::BeginTabBar("ConnectionTypes")) {
        drawIPConnectionTab(connections);
        drawBTConnectionTab(connections, sdpWindows);
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void windowMenu(WindowList& list, std::string_view desc) {
    if (!ImGui::BeginMenu(desc.data())) return;

    if (list.empty()) ImGui::TextDisabled("%s", std::format("No {}", desc).c_str());
    else
        for (const auto& i : list) ImGuiExt::windowMenuItem(i->getTitle());

    ImGui::EndMenu();
}

// Draws the main menu bar.
void drawMenuBar(bool& quit, bool& settingsOpen, bool& newConnOpen, bool& notificationsOpen, bool& newServerOpen,
    WindowList& connections, WindowList& servers) {
    if (!ImGui::BeginMainMenuBar()) return;

    ImGuiExt::drawNotificationsMenu(notificationsOpen);

    if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Settings", nullptr, &settingsOpen);
        ImGui::MenuItem("Quit", nullptr, &quit);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("New Connection", nullptr, &newConnOpen);
        ImGui::MenuItem("New Server", nullptr, &newServerOpen);
        ImGui::MenuItem("Notifications", nullptr, &notificationsOpen);
        ImGui::EndMenu();
    }

    // List all open connections and servers
    windowMenu(connections, "Connections");
    windowMenu(servers, "Servers");

    ImGui::EndMainMenuBar();
}

// Contains the app's core logic and functions.
void mainLoop() {
    // These variables must be in a separate scope from the resource instances, so these can be destructed before
    // cleanup
    WindowList connections; // List of open windows
    WindowList sdpWindows; // List of windows for creating Bluetooth connections
    WindowList servers; // List of servers

    bool quit = false;
    while (!quit && AppCore::newFrame()) {
        Async::handleEvents();

        static bool settingsOpen = false;
        static bool newConnOpen = true;
        static bool newServerOpen = false;
        static bool notificationsOpen = false;

        // Main menu bar
        drawMenuBar(quit, settingsOpen, newConnOpen, notificationsOpen, newServerOpen, connections, servers);

        // Application windows
        Settings::drawSettingsWindow(settingsOpen);
        drawNewConnectionWindow(newConnOpen, connections, sdpWindows);
        drawNewServerWindow(servers, newServerOpen);
        ImGuiExt::drawNotificationsWindow(notificationsOpen);

        connections.update();
        servers.update();
        sdpWindows.update();
        AppCore::render();
    }
}

int main(int, char**) {
    // Create a main application window
    if (!AppCore::init()) return EXIT_FAILURE;

    // OS API resource instances
    std::optional<Async::Instance> asyncInstance;
    std::optional<BTUtils::Instance> btutilsInstance;

    using namespace std::literals;

    // Initialize APIs for sockets and Bluetooth
    try {
        asyncInstance.emplace(Settings::getSetting<uint8_t>("os.numThreads"),
            Settings::getSetting<uint8_t>("os.queueEntries"));
        btutilsInstance.emplace();
    } catch (const System::SystemError& error) {
        ImGuiExt::addNotification("Initialization error "s + error.what(), NotificationType::Error, 0);
    } catch (const std::system_error&) {
        ImGuiExt::addNotification("Initialization error: Could not initialize thread pool", NotificationType::Error, 0);
    }

    // Run app
    mainLoop();

    AppCore::cleanup();
    return EXIT_SUCCESS;
}
