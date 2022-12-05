// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <system_error>

#if OS_WINDOWS
#include <SDL_main.h> // For definition of main function
#endif

#include "gui/app.hpp"
#include "gui/newconnbt.hpp"
#include "gui/newconnip.hpp"
#include "gui/notifications.hpp"
#include "gui/windowlist.hpp"
#include "os/async.hpp"
#include "os/btutils.hpp"

// Contains the app's core logic and functions.
void mainLoop() {
    // These variables must be in a separate scope from the cleanup function, so they can be destructed before cleanup
    WindowList connections; // List of open windows
    WindowList sdpWindows;  // List of windows for creating Bluetooth connections

    while (App::newFrame()) {
        // New connection window
        ImGui::SetNextWindowSize({ 600, 170 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("New Connection") && ImGui::BeginTabBar("ConnectionTypes")) {
            drawIPConnectionTab(connections);
            drawBTConnectionTab(connections, sdpWindows);
            ImGui::EndTabBar();
        }
        ImGui::End();

        connections.update();
        sdpWindows.update();
        App::render();
    }
}

int main(int, char**) {
    // Create a main application window
    if (!App::init()) return EXIT_FAILURE;

    // Initialize APIs for sockets and Bluetooth
    try {
        Async::init();
        BTUtils::init();
    } catch (const System::SystemError& error) {
        ImGui::AddNotification("Initialization error " + error.formatted(), 0);
    } catch (const std::system_error&) {
        ImGui::AddNotification("Initialization error: Could not initialize thread pool", 0);
    }

    // Run app
    mainLoop();

    BTUtils::cleanup();
    Async::cleanup();
    App::cleanup();
    return EXIT_SUCCESS;
}
