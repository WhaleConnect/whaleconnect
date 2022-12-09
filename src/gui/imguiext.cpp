// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imguiext.hpp"

#include <bit>
#include <numbers> // std::numbers::pi

#include <imgui.h>

static int stringCallback(ImGuiInputTextCallbackData* data) {
    // Get user data (assume it's a string pointer)
    auto& str = *std::bit_cast<std::string*>(data->UserData);

    // Resize the string, then set the callback data buffer
    str.resize(data->BufTextLen);
    data->Buf = str.data();
    return 0;
}

bool ImGui::InputText(std::string_view label, std::string& s, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return InputText(label.data(), s.data(), s.capacity() + 1, flags, stringCallback, &s);
}

bool ImGui::InputTextMultiline(std::string_view label, std::string& s, const ImVec2& size, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return InputTextMultiline(label.data(), s.data(), s.capacity() + 1, size, flags, stringCallback, &s);
}

void ImGui::HelpMarker(std::string_view desc) {
    // Adapted from imgui_demo.cpp.
    SameLine();
    TextDisabled("(?)");
    if (!IsItemHovered()) return;

    BeginTooltip();
    PushTextWrapPos(GetFontSize() * 35.0f);
    TextUnformatted(desc);
    PopTextWrapPos();
    EndTooltip();
}

void ImGui::Spinner() {
    // Text settings
    float textSizeHalf = GetTextLineHeight() / 2;
    ImU32 textColor = GetColorU32(ImGuiCol_Text);

    auto time = static_cast<float>(GetTime() * 10); // Current time (multiplied to make the spinner faster)

    // Position to draw the spinner
    ImVec2 cursorPos = GetCursorScreenPos();
    ImVec2 center{ cursorPos.x + textSizeHalf, cursorPos.y + textSizeHalf };

    // Draw the spinner, arc from 0 radians to (3pi / 2) radians (270 degrees)
    constexpr auto arcLength = static_cast<float>(std::numbers::pi * (3.0 / 2.0));
    ImDrawList& drawList = *GetWindowDrawList();
    drawList.PathArcTo(center, textSizeHalf, time, time + arcLength);
    drawList.PathStroke(textColor, 0, textSizeHalf / 2);
}
