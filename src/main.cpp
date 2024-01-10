// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <optional>
#include <string_view>
#include <system_error>

#include <imgui.h>
#include <nlohmann/json.hpp> // IWYU pragma: keep (fixes errors on MSVC)
#include <SDL3/SDL_main.h>

import app.appcore;
import app.settings;
import components.windowlist;
import gui.menu;
import gui.newconn;
import gui.newserver;
import gui.notifications;
import net.btutils;
import os.async;
import os.error;

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
