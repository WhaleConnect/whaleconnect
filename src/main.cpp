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

#if OS_WINDOWS
// Use WinMain() as an entry point on MSVC
#define MAIN_FUNC CALLBACK WinMain
#define MAIN_ARGS _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int
#else
// Use the standard main() as an entry point on other compilers
#define MAIN_FUNC main
#define MAIN_ARGS
#endif

using InitResult = std::variant<std::monostate, System::SystemError, std::system_error>;

void mainLoop(const InitResult& initResult) {
    WindowList connections; // List of open windows
    WindowList sdpWindows; // List of windows for creating Bluetooth connections

    while (MainHandler::isActive()) {
        MainHandler::handleNewFrame();

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

        MainHandler::renderWindow();
    }
}

int MAIN_FUNC(MAIN_ARGS) {
    if (!MainHandler::initApp()) return EXIT_FAILURE; // Create a main application window

    // Initialize APIs for sockets and Bluetooth
    InitResult initResult;
    try {
        BTUtils::init();
    } catch (const System::SystemError& error) {
        initResult = error;
    } catch (const std::system_error& error) {
        initResult = error;
    }

    mainLoop(initResult);

    BTUtils::cleanup();
    MainHandler::cleanupApp();
    return EXIT_SUCCESS;
}
