// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit>
#include <string>

#include <imgui.h>

#include "imguiext.hpp"

static int stringCallback(ImGuiInputTextCallbackData* data) {
    // Get user data (assume it's a string pointer)
    auto& str = *std::bit_cast<std::string*>(data->UserData);

    // Resize the string, then set the callback data buffer
    str.resize(data->BufTextLen);
    data->Buf = str.data();
    return 0;
}

bool ImGui::InputText(const char* label, std::string& s, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return InputText(label, s.data(), s.capacity() + 1, flags, stringCallback, &s);
}

bool ImGui::InputTextMultiline(const char* label, std::string& s, const ImVec2& size, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return InputTextMultiline(label, s.data(), s.capacity() + 1, size, flags, stringCallback, &s);
}

void ImGui::HelpMarker(const char* desc) {
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
