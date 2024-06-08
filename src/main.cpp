// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <optional>
#include <system_error>

#include "app/appcore.hpp"
#include "app/settings.hpp"
#include "components/windowlist.hpp"
#include "gui/about.hpp"
#include "gui/menu.hpp"
#include "gui/menu.state.hpp"
#include "gui/newconn.hpp"
#include "gui/newserver.hpp"
#include "gui/notifications.hpp"
#include "net/btutils.hpp"
#include "os/async.hpp"
#include "os/error.hpp"

// Contains the app's core logic and functions.
void mainLoop() {
    // These variables must be in a separate scope from the resource instances, so these can be destructed before
    // cleanup
    WindowList connections; // List of open windows
    WindowList sdpWindows; // List of windows for creating Bluetooth connections
    WindowList servers; // List of servers

    bool quit = false;
    while (!quit && AppCore::newFrame()) {
        // Handle event loop without waiting, waiting is done by vsync
        Async::handleEvents(false);
        Menu::drawMenuBar(quit, connections, servers);

        // Application windows
        Settings::drawSettingsWindow(Menu::settingsOpen);
        drawNewConnectionWindow(Menu::newConnectionOpen, connections, sdpWindows);
        drawNewServerWindow(servers, Menu::newServerOpen);
        ImGuiExt::drawNotificationsWindow(Menu::notificationsOpen);
        drawAboutWindow(Menu::aboutOpen);
        drawLinksWindow(Menu::linksOpen);

        connections.update();
        servers.update();
        sdpWindows.update();
        AppCore::render();
    }
}

#if OS_WINDOWS
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(int, char**)
#endif
{
    // Create a main application window
    if (!AppCore::init()) return 1;
    if (Settings::GUI::systemMenu) Menu::setupMenuBar();

    // OS API resource instances
    std::optional<BTUtils::Instance> btutilsInstance;

    using namespace std::literals;

    // Initialize APIs for sockets and Bluetooth
    try {
        Async::init(Settings::OS::numThreads, Settings::OS::queueEntries);
        btutilsInstance.emplace();
    } catch (const System::SystemError& error) {
        ImGuiExt::addNotification("Initialization error "s + error.what(), NotificationType::Error, 0);
    } catch (const std::system_error&) {
        ImGuiExt::addNotification("Initialization error: Could not initialize thread pool", NotificationType::Error, 0);
    }

    // Run app
    mainLoop();

    AppCore::cleanup();
    return 0;
}
