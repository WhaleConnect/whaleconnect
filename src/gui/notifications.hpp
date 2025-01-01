// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

// Icons to display in notifications.
enum class NotificationType { Error, Warning, Info, Success };

namespace ImGuiExt {
    // Adds a notification with text, icon, and an optional automatic close timeout.
    void addNotification(std::string_view s, NotificationType type, float timeout = 10);

    // Draws the notifications in the bottom-right corner of the window.
    void drawNotifications();

    // Draws a window containing the notifications.
    void drawNotificationsWindow(bool& open);

    // Draws a menu containing the notifications.
    void drawNotificationsMenu(bool& notificationsOpen);
}
