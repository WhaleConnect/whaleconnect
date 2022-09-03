// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <system_error>
#include <variant>

#include "app/mainhandle.hpp"
#include "core/newconnbt.hpp"
#include "core/newconnip.hpp"
#include "gui/windowlist.hpp"
#include "net/btutils.hpp"
#include "util/imguiext.hpp"

#ifdef _WIN32
// Use WinMain() as an entry point on MSVC
#define MAIN_FUNC CALLBACK WinMain
#define MAIN_ARGS _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int
#else
// Use the standard main() as an entry point on other compilers
#define MAIN_FUNC main
#define MAIN_ARGS
#endif

int MAIN_FUNC(MAIN_ARGS) {
    if (!MainHandler::initApp()) return EXIT_FAILURE; // Create a main application window

    // Initialize APIs for sockets and Bluetooth
    std::variant<std::monostate, System::SystemError, std::system_error> initResult;
    try {
        BTUtils::init();
    } catch (const System::SystemError& error) {
        initResult = error;
    } catch (const std::system_error& error) {
        initResult = error;
    }

    // Main loop
    while (MainHandler::isActive()) {
        MainHandler::handleNewFrame();

        // Error message for failed initialization
        if (std::holds_alternative<System::SystemError>(initResult))
            ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopLeft,
                           "Initialization failed - %s", std::get<System::SystemError>(initResult).formatted().c_str());
        else if (std::holds_alternative<std::system_error>(initResult))
            ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner::TopLeft, "Could not initialize thread pool.");

        static WindowList connections; // List of open windows
        static WindowList sdpWindows; // List of windows for creating Bluetooth connections

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

        MainHandler::renderWindow();
    }

    BTUtils::cleanup();
    MainHandler::cleanupApp();
    return EXIT_SUCCESS;
}
