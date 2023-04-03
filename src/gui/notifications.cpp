// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications.hpp"

#include <algorithm> // std::max()
#include <format>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

// Structure to contain information about a notification.
struct Notification {
    std::string text; // Text shown in the entry
    double timeAdded; // When the notification was added
    bool active;      // If the notification should still be shown
    float timeout;    // Number of seconds before automatically closing the notification
};

static std::vector<Notification> notifications; // Currently active notifications
static bool areNotificationsVisible = false;    // If the notifications window is being rendered

// Draws a single notification in the notification area.
static void drawNotification(Notification& n, float areaWidth) {
    //   <------------------------>   [Area width]
    //        <------------------->   [Entry width]
    //          <--------------->     [Wrap width]
    // |      +-------------------+ |
    // | [X]  | Notification text | |
    // |      +-------------------+ |
    //  ^   ^~ ^                 ^ ^
    //  1   2  1                 1 1  [1: Window padding, 2: Item spacing]

    // Window padding
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    ImVec2 paddingDouble = { padding.x * 2, padding.y * 2 };

    // Close button information
    const char* buttonLabel = "\u2715";
    ImVec2 buttonSize = ImGui::CalcTextSize(buttonLabel);
    float buttonSpace = buttonSize.x + ImGui::GetStyle().ItemSpacing.x;

    // Entry dimensions
    // All text is wrapped in each entry, so the height must be calculated from the text and wrap width.
    float entryWidth = areaWidth - buttonSpace;
    float wrapWidth = entryWidth - paddingDouble.x;
    float textHeight = ImGui::CalcTextSize(n.text.data(), nullptr, false, wrapWidth).y;
    float entryHeight = textHeight + paddingDouble.y;

    // Save the current cursor position, then move the cursor to align the close button vertically
    ImVec2 currentPos = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(currentPos.y + (entryHeight / 2) - (ImGui::GetFontSize() / 2));

    // Remove padding, then set button colors
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.82f, 0, 0, 1 });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.64f, 0, 0, 1 });

    // When the button is clicked, deactivate the notification
    // When the button is hovered, deactivate the timeout
    if (ImGui::Button(buttonLabel, buttonSize)) n.active = false;
    if (ImGui::IsItemHovered()) n.timeout = 0;

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Draw the entry next to the close button
    ImGui::SetCursorPos({ currentPos.x + buttonSpace, currentPos.y });
    ImGui::BeginChild("##entry", { entryWidth, entryHeight }, true);
    ImGui::TextWrapped("%s", n.text.c_str());
    ImGui::EndChild();

    // If the timeout is 0, skip the countdown
    if (!(n.timeout > 0.0f)) return;

    // Calculate the percent of time still remaining before the notification is closed automatically (0 to 1)
    // If this percent reaches 0, deactivate the notification
    auto timePercent = static_cast<float>(std::max(1 - ((ImGui::GetTime() - n.timeAdded) / n.timeout), 0.0));
    n.active = timePercent > 0.0f;

    // Get left and bottom coordinates of the entry rect to draw the countdown line
    float entryLeft = ImGui::GetItemRectMin().x;
    float entryBottom = ImGui::GetItemRectMax().y;

    // Draw the countdown line
    // The length is proportional to the entry width and the amount of time remaining before automatic closure
    ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);
    ImVec2 lineStart{ entryLeft, entryBottom };
    ImVec2 lineEnd{ entryLeft + entryWidth * timePercent, entryBottom };
    ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, borderColor, 4);
}

void ImGui::AddNotification(std::string_view s, float timeout) {
    // Add a notification struct to the vector
    // If the notifications are not visible (such as the notifications area being collapsed), the timeout is set to 0
    // (disabled) to prevent the user from missing a potentially important notification.
    notifications.emplace_back(std::string{ s }, GetTime(), true, areNotificationsVisible ? timeout : 0);
}

void ImGui::DrawNotificationArea(ImGuiID dockSpaceID) {
    constexpr const char* windowIDBase = "###Notifications";
    static auto makeWindowID
        = [windowIDBase](std::string_view s = "") { return std::format("Notifications{}{}", s, windowIDBase); };

    static std::string windowID = makeWindowID();

    // Set up docking configuration on the first frame of the application
    static bool firstFrame = true;
    if (firstFrame) {
        // Remove previous docking configurations
        DockBuilderRemoveNodeChildNodes(dockSpaceID);

        // Dock the notifications area to the right node of the dock space
        ImGuiID nodeID;
        DockBuilderSplitNode(dockSpaceID, ImGuiDir_Right, 0.25f, &nodeID, nullptr);
        DockBuilderDockWindow(windowIDBase, nodeID);
        DockBuilderFinish(dockSpaceID);
        firstFrame = false;
    }

    // The last number of notifications the user saw
    static size_t lastNumberSeen = 0;

    areNotificationsVisible = Begin(windowID.c_str());
    if (areNotificationsVisible) {
        // If there are new notifications, scroll to the top to display them
        if (lastNumberSeen < notifications.size()) ImGui::SetScrollHereY(0);

        // Update the number of seen notifications and the window ID
        lastNumberSeen = notifications.size();
        windowID = makeWindowID();

        // Control buttons
        if (!notifications.empty()) {
            if (ImGui::Button("Clear All")) notifications.clear();

            ImGui::SameLine();
            if (ImGui::Button("Preserve All"))
                for (auto& i : notifications) i.timeout = 0;
        }

        // Available width for entries
        float width = GetContentRegionAvail().x;

        // Draw each notification
        // TODO: Use views::enumerate and views::reverse in C++23
        for (auto i = static_cast<int>(notifications.size()) - 1; i >= 0; i--) {
            ImGui::PushID(i);
            drawNotification(notifications[i], width);
            ImGui::PopID();
        }
    } else {
        // The notifications are not visible
        // If there are any new ones, the number of them is appended into the title.
        size_t newNotifications = notifications.size() - lastNumberSeen;
        if (newNotifications > 0) windowID = makeWindowID(std::format(" ({})", newNotifications));
    }
    End();

    // Erase inactive notifications
    std::erase_if(notifications, [](const Notification& n) { return !n.active; });
}
