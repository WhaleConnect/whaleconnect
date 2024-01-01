// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#define IMGUI_DEFINE_MATH_OPERATORS

#include <format>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

module gui.notifications;
import gui.imguiext;

constexpr const char* notificationsWindowTitle = "Notifications";

// Contains information about a notification.
class Notification {
    // Visibility of the notification.
    enum Visibility {
        Visible = 1 << 0, // Displayed in corner and notifications list
        Hidden = 1 << 1, // Displayed in notifications list only
        Erased = 1 << 2, // Not displayed
        Fading = 1 << 3 // Fading out (or-ed with another value)
    };

    static inline int numNotifications = 0; // Total number of notifications

    std::string id = std::format("Notification {}", numNotifications); // String to identify the notification

    double timeAdded = ImGui::GetTime(); // When this notification was added

    std::string text; // Text shown
    NotificationType type; // Icon type
    int visibility = Visible; // How this notification is displayed
    float timeout; // Number of seconds before automatically closing this notification
    float opacity = 1.0f; // Opacity of the notification

public:
    // Sets the information to draw the notification.
    Notification(std::string_view text, NotificationType type, float timeout) :
        text(text), type(type), timeout(timeout) {
        numNotifications++;
    }

    // Draws this notification.
    float update(const ImVec2& pos, bool showInCorner);

    // Starts the fade out animation.
    void setFadeOut(bool eraseOnFinish) {
        visibility = Fading | (eraseOnFinish ? Erased : Hidden);
    }

    // Checks if this notification is hidden.
    bool hidden() const {
        return visibility == Hidden;
    }

    // Checks if this notification is erased.
    bool erased() const {
        return visibility == Erased;
    }
};

std::vector<Notification> notifications; // Currently active notifications

// Draws a single notification in the notification area.
float Notification::update(const ImVec2& pos, bool showInCorner) {
    using namespace ImGuiExt::Literals;

    // Get position of the notification's bottom-right corner relative to the viewport
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 viewportSize = viewport->Size;
    ImVec2 windowPos = viewport->Pos + viewportSize - pos;

    if (showInCorner) {
        if (visibility & Fading) opacity -= 5_dt;

        // Set notification opacity, if it is zero then it should be hidden
        if (opacity > 0) {
            ImGui::SetNextWindowBgAlpha(opacity);
        } else {
            visibility &= ~Fading;
            return pos.y;
        }

        // Set window position
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, { 1, 1 });

        // Begin the containing window for the notification
        // The Tooltip flag is added to make it stay above other windows.
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav
            | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Tooltip;
        ImGui::Begin(id.c_str(), nullptr, flags);
    }

    // Draw icon
    using enum NotificationType;
    switch (type) {
        case Warning:
            ImGui::TextColored({ 0.98f, 0.74f, 0.02f, 1 }, "\uea21");
            break;
        case Error:
            ImGui::TextColored({ 0.82f, 0, 0, 1 }, "\ueb97");
            break;
        case Info:
            ImGui::TextColored({ 0.0f, 0.45f, 0.81f, 1 }, "\uee59");
            break;
        case Success:
            ImGui::TextColored({ 0.08f, 0.54f, 0.06f, 1 }, "\ueb81");
    }

    const float textWidth = 20_fh; // Width of text before it is wrapped

    // Text wrapping position in window coordinates
    // If the notifications are shown in a parent window, the text is wrapped within the window.
    float wrapPos = showInCorner ? ImGui::GetCursorPosX() + textWidth : ImGui::GetWindowWidth() - 2_fh;

    // Draw text
    ImGui::SameLine();
    ImGui::PushTextWrapPos(wrapPos);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopTextWrapPos();

    // Styles for close button
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, 2 });
    ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.82f, 0, 0, 1 });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.64f, 0, 0, 1 });

    // Draw close button
    // The cursor position is set to make each notification have the same width
    ImGui::SameLine();
    ImGui::SetCursorPosX(wrapPos + ImGui::GetStyle().ItemSpacing.x);
    ImGui::PushID(id.c_str());

    if (ImGui::Button("\ueb99")) visibility = Erased | Fading;

    ImGui::PopID();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    if (showInCorner) {
        ImVec2 windowSize = ImGui::GetWindowSize();

        if (visibility == Visible && timeout > 0.0f) {
            // If the timeout is 0, skip the countdown
            // Calculate the percent of time since the notification was created
            auto timePercent = static_cast<float>((ImGui::GetTime() - timeAdded) / timeout);

            // If this percent reaches 1, fade out the notification
            if (timePercent >= 1.0f) setFadeOut(false);

            // Draw the countdown line
            // The length is proportional to the entry width and the amount of time remaining before automatic closure
            ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
            ImVec2 lineStart{ windowPos.x - windowSize.x, windowPos.y };
            ImVec2 lineEnd{ windowPos.x - windowSize.x * timePercent, windowPos.y };
            ImGui::GetForegroundDrawList()->AddLine(lineStart, lineEnd, color, 2);
        }

        ImGui::End();
        return pos.y + windowSize.y;
    }

    return 0;
}

// Draws all notifications that have not been explicitly closed as part of an enclosing window.
void drawNotificationContents(bool* open) {
    bool clearAll = false;

    if (notifications.empty()) ImGui::Text("No Notifications");
    else if (ImGui::Button("\uec2a")) clearAll = true;

    // Display "pop out" button if applicable
    if (open) {
        ImGui::SameLine();
        if (ImGui::Button("\uecaf")) {
            *open = true;
            ImGui::SetWindowFocus(notificationsWindowTitle);
        }
    }

    // Child window to contain entries
    ImGui::BeginChild("##content");

    // Clear (if needed) and render in one loop
    for (auto& i : notifications) {
        if (clearAll) i.setFadeOut(true);

        i.update({}, false);
    }

    ImGui::EndChild();
}

void ImGuiExt::addNotification(std::string_view s, NotificationType type, float timeout) {
    notifications.emplace_back(s, type, timeout);
}

void ImGuiExt::drawNotifications() {
    static const float notificationSpacing = 10;
    float yPos = notificationSpacing;

    float workHeightHalf = ImGui::GetMainViewport()->WorkSize.y / 2.0f;

    // Erase inactive notifications
    std::erase_if(notifications, [](const Notification& n) { return n.erased(); });

    // Index of the first notification that overflows
    auto overflowIter = notifications.end();

    for (auto i = notifications.begin(); i < notifications.end(); i++) {
        // Draw notification and move Y position up
        if (!i->hidden()) yPos = i->update({ notificationSpacing, yPos }, true);

        // Check for overflow if the Y position grows too large
        if (yPos > workHeightHalf) {
            overflowIter = i;
            break;
        }
    }

    // On each frame, decrease the opacity of older notifications if there is an overflow
    // This makes a fade-out animation and the notifications will be removed once opacity is 0.
    if (overflowIter < notifications.end())
        for (auto i = notifications.begin(); i < overflowIter; i++) i->setFadeOut(false);
}

void ImGuiExt::drawNotificationsWindow(bool& open) {
    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(22_fh * 30_fh, ImGuiCond_Appearing);

    if (ImGui::Begin(notificationsWindowTitle, &open)) drawNotificationContents(nullptr);
    ImGui::End();
}

void ImGuiExt::drawNotificationsMenu(bool& notificationsOpen) {
    std::string content;

    // Get the display number for the menu
    if (!notifications.empty()) {
        if (notifications.size() < 10) content = std::to_string(notifications.size());
        else content = "9+";
    }

    // Draw menu
    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(20_fh * 20_fh);
    if (ImGui::BeginMenu(std::format("\uef93 {}###Notifications", content).c_str())) {
        drawNotificationContents(&notificationsOpen);
        ImGui::EndMenu();
    }

    // Position cursor to draw next menu
    using namespace ImGuiExt::Literals;
    ImGui::SetCursorPosX(ImGui::GetCursorStartPos().x + 3_fh);
}
