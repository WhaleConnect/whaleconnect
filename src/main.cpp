// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <optional>
#include <system_error>

#include <SDL_main.h> // For definition of main function

#include "gui/app.hpp"
#include "gui/imguiext.hpp"
#include "gui/newconnbt.hpp"
#include "gui/newconnip.hpp"
#include "gui/windowlist.hpp"
#include "os/async.hpp"
#include "os/btutils.hpp"
#include "os/net.hpp"

// Contains the app's core logic and functions.
void mainLoop() {
    // Initialize APIs for sockets and Bluetooth
    std::optional<std::string> failureMessage;
    try {
        Net::init();
        Async::init();
        BTUtils::init();
    } catch (const System::SystemError& error) {
        failureMessage = "Initialization failed - " + error.formatted();
    } catch (const std::system_error&) {
        failureMessage = "Initialization failed - Could not initialize thread pool";
    }

    // These variables must be in a separate scope from the cleanup function so they can be destructed before cleanup
    WindowList connections; // List of open windows
    WindowList sdpWindows;  // List of windows for creating Bluetooth connections

    while (App::newFrame()) {
        // Error message for failed initialization
        if (failureMessage) ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopLeft, failureMessage->c_str());

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

    // Run app
    mainLoop();

    BTUtils::cleanup();
    Async::cleanup();
    Net::cleanup();
    App::cleanup();
    return EXIT_SUCCESS;
}
