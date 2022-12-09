// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include <imgui.h>

namespace ImGui {
    // Adds a notification with text and an optional automatic close timeout.
    void AddNotification(std::string_view s, float timeout = 5);

    // Draws the notification area.
    void DrawNotificationArea(ImGuiID dockSpaceID);
}
