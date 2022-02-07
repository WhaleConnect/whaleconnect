// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string> // std::string

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "imguiext.hpp"

/// <summary>
/// A resize callback for InputText* functions for std::string.
/// </summary>
/// <param name="data">The callback data passed by the InputText</param>
/// <returns>Always returns 0</returns>
static int stringCallback(ImGuiInputTextCallbackData* data) {
    // Get user data (assume it's a string pointer)
    std::string& str = *reinterpret_cast<std::string*>(data->UserData);

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
    SameLine();
    TextDisabled("(?)");
    if (IsItemHovered()) {
        BeginTooltip();
        PushTextWrapPos(GetFontSize() * 35.0f);
        TextUnformatted(desc);
        PopTextWrapPos();
        EndTooltip();
    }
}
