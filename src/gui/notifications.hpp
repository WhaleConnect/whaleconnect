// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

// Icons to display in notifications.
enum class NotificationType { Error, Warning, Info, Success };

namespace ImGui {
    // Adds a notification with text, icon, and an optional automatic close timeout.
    void AddNotification(std::string_view s, NotificationType type, float timeout = 10);

    // Draws the notifications in the bottom-right corner of the window.
    void DrawNotifications();

    // Draws a window containing all notifications that have not been explicitly closed.
    void DrawNotificationWindow(bool* open);
}
