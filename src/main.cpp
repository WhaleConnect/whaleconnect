// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <optional>
#include <system_error>

#include <imgui.h>
#include <SDL3/SDL_main.h>

#include "app/app.hpp"
#include "app/settings.hpp"
#include "gui/imguiext.hpp"
#include "gui/newconnbt.hpp"
#include "gui/newconnip.hpp"
#include "gui/notifications.hpp"
#include "net/btutils.hpp"
#include "os/async.hpp"
#include "windows/windowlist.hpp"

// Draws the new connection window.
void drawNewConnectionWindow(bool& open, WindowList& connections, WindowList& sdpWindows) {
    if (!open) return;

    using namespace ImGui::Literals;
    ImGui::SetNextWindowSize(40_fh * 11_fh, ImGuiCond_Appearing);

    if (ImGui::Begin("New Connection", &open) && ImGui::BeginTabBar("ConnectionTypes")) {
        drawIPConnectionTab(connections);
        drawBTConnectionTab(connections, sdpWindows);
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// Draws the main menu bar.
void drawMenuBar(bool& newConnOpen, bool& notificationsOpen, WindowList& connections) {
    if (!ImGui::BeginMainMenuBar()) return;

    ImGui::DrawNotificationsMenu(notificationsOpen);

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("New Connection", nullptr, &newConnOpen);
        ImGui::MenuItem("Notifications", nullptr, &notificationsOpen);
        ImGui::EndMenu();
    }

    // List all open connection windows
    if (ImGui::BeginMenu("Connections")) {
        if (connections.empty()) ImGui::TextDisabled("No Open Connections");
        else
            for (const auto& i : connections) ImGui::WindowMenuItem(i->getTitle());

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

// Contains the app's core logic and functions.
void mainLoop() {
    // These variables must be in a separate scope from the resource instances, so these can be destructed before
    // cleanup
    WindowList connections; // List of open windows
    WindowList sdpWindows;  // List of windows for creating Bluetooth connections

    while (App::newFrame()) {
        static bool newConnOpen = true;
        static bool notificationsOpen = false;

        // Main menu bar
        drawMenuBar(newConnOpen, notificationsOpen, connections);

        // Application windows
        drawNewConnectionWindow(newConnOpen, connections, sdpWindows);
        ImGui::DrawNotificationsWindow(notificationsOpen);

        connections.update();
        sdpWindows.update();
        App::render();
    }
}

int main(int, char**) {
    // Create a main application window
    if (!App::init()) return EXIT_FAILURE;

    // OS API resource instances
    std::optional<Async::Instance> asyncInstance;
    std::optional<BTUtils::Instance> btutilsInstance;

    using namespace std::literals;

    // Initialize APIs for sockets and Bluetooth
    try {
        asyncInstance.emplace(Settings::numThreads, Settings::queueEntries);
        btutilsInstance.emplace();
    } catch (const System::SystemError& error) {
        ImGui::AddNotification("Initialization error "s + error.what(), NotificationType::Error, 0);
    } catch (const std::system_error&) {
        ImGui::AddNotification("Initialization error: Could not initialize thread pool", NotificationType::Error, 0);
    }

    // Run app
    mainLoop();

    App::cleanup();
    return EXIT_SUCCESS;
}
