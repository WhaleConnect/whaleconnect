// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <optional>
#include <system_error>

#if OS_WINDOWS
#undef SDL_MAIN_HANDLED
#include <SDL_main.h> // For definition of main function
#endif

#include "gui/app.hpp"
#include "gui/newconnbt.hpp"
#include "gui/newconnip.hpp"
#include "gui/notifications.hpp"
#include "gui/windowlist.hpp"
#include "os/async.hpp"
#include "os/btutils.hpp"
#include "utils/settings.hpp"

// Contains the app's core logic and functions.
void mainLoop() {
    // These variables must be in a separate scope from the resource instances, so these can be destructed before cleanup
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

        BTUtils::processAsyncEvents();
    }
}

int main(int, char**) {
    // Create a main application window
    if (!App::init()) return EXIT_FAILURE;

    // OS API resource instances
    std::optional<Async::Instance> asyncInstance;
    std::optional<BTUtils::Instance> btutilsInstance;

    // Initialize APIs for sockets and Bluetooth
    try {
        asyncInstance.emplace(Settings::numThreads);
        btutilsInstance.emplace();
    } catch (const System::SystemError& error) {
        ImGui::AddNotification("Initialization error " + error.formatted(), 0);
    } catch (const std::system_error&) {
        ImGui::AddNotification("Initialization error: Could not initialize thread pool", 0);
    }

    // Run app
    mainLoop();

    App::cleanup();
    return EXIT_SUCCESS;
}
