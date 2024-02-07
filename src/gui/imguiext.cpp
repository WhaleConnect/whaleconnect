// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module gui.imguiext;
import external.imgui;
import external.std;

int stringCallback(ImGuiInputTextCallbackData* data) {
    // Get user data (assume it's a string pointer)
    auto& str = *reinterpret_cast<std::string*>(data->UserData);

    // Resize the string, then set the callback data buffer
    str.resize(data->BufTextLen);
    data->Buf = str.data();
    return 0;
}

bool ImGuiExt::inputText(std::string_view label, std::string& s, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label.data(), s.data(), s.capacity() + 1, flags, stringCallback, &s);
}

bool ImGuiExt::inputTextMultiline(std::string_view label, std::string& s, const ImVec2& size,
    ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputTextMultiline(label.data(), s.data(), s.capacity() + 1, size, flags, stringCallback, &s);
}

void ImGuiExt::helpMarker(std::string_view desc) {
    // Adapted from imgui_demo.cpp.
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (!ImGui::IsItemHovered()) return;

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    textUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void ImGuiExt::spinner() {
    // Text settings
    float textSizeHalf = ImGui::GetTextLineHeight() / 2;
    ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

    // Current time (multiplied to make the spinner faster)
    auto time = static_cast<float>(ImGui::GetTime() * 10);

    // Position to draw the spinner
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 center{ cursorPos.x + textSizeHalf, cursorPos.y + textSizeHalf };

    // Draw the spinner, arc from 0 radians to (3pi / 2) radians (270 degrees)
    constexpr auto arcLength = static_cast<float>(std::numbers::pi * (3.0 / 2.0));
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    drawList.PathArcTo(center, textSizeHalf, time, time + arcLength);
    drawList.PathStroke(textColor, 0, textSizeHalf / 2);
}
