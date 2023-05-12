// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "connwindow.hpp"
#include "notifications.hpp"
#include "windowlist.hpp"

#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "sockets/device.hpp"

// Adds a ConnWindow to a window list and handles errors during socket creation.
template <auto Tag>
void addConnWindow(WindowList& list, const Device& device, std::string_view extraInfo) try {
    auto socket = createClientSocket<Tag>(device);
    bool isNew = list.add<ConnWindow>(std::move(socket), socket->getDevice(), extraInfo);

    // If the connection exists, show a message
    if (!isNew) ImGui::AddNotification("This connection is already open.", NotificationType::Warning);
} catch (const System::SystemError& e) {
    ImGui::AddNotification("Socket creation error " + e.formatted(), NotificationType::Error);
}
