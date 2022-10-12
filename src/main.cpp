// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <system_error>
#include <variant>

#include <SDL_main.h> // For definition of main function

#include "app/app.hpp"
#include "core/newconnbt.hpp"
#include "core/newconnip.hpp"
#include "gui/windowlist.hpp"
#include "net/btutils.hpp"
#include "util/imguiext.hpp"

// Contains the app's core logic and functions.
void mainLoop(const App::SDLData &sdlData) {
    // Initialize APIs for sockets and Bluetooth
    std::variant<std::monostate, System::SystemError, std::system_error> initResult;
    try {
        BTUtils::init();
    } catch (const System::SystemError& error) {
        initResult = error;
    } catch (const std::system_error& error) {
        initResult = error;
    }

    // These variables must be in a separate scope from the cleanup function so they can be destructed before cleanup
    WindowList connections; // List of open windows
    WindowList sdpWindows; // List of windows for creating Bluetooth connections

    while (App::newFrame()) {
        // Error message for failed initialization
        if (std::holds_alternative<System::SystemError>(initResult))
            ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopLeft, "Initialization failed - %s",
                        std::get<System::SystemError>(initResult).formatted().c_str());
        else if (std::holds_alternative<std::system_error>(initResult))
            ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopLeft, "Could not initialize thread pool.");

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
        App::render(sdlData);
    }
}

int main(int, char**) {
    // Create a main application window
    auto sdlData = App::init();
    if (!sdlData.window || !sdlData.glContext) return EXIT_FAILURE;

    // Run app
    mainLoop(sdlData);

    BTUtils::cleanup();
    App::cleanup(sdlData);
    return EXIT_SUCCESS;
}
