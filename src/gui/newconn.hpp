// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string_view>

#include "notifications.hpp"

#include "net/device.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "windows/connwindow.hpp"
#include "windows/windowlist.hpp"

// Adds a ConnWindow to a window list and handles errors during socket creation.
template <auto Tag>
void addConnWindow(WindowList& list, const Device& device, std::string_view extraInfo) try {
    auto socket = std::make_unique<ClientSocket<Tag>>(device);
    bool isNew = list.add<ConnWindow>(std::move(socket), socket->getServer(), extraInfo);

    // If the connection exists, show a message
    if (!isNew) ImGui::AddNotification("This connection is already open.", NotificationType::Warning);
} catch (const System::SystemError& e) {
    using namespace std::literals;
    ImGui::AddNotification("Socket creation error "s + e.what(), NotificationType::Error);
}
