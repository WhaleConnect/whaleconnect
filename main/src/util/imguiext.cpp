// Copyright 2021 the Network Socket Terminal contributors
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
    data->Buf = const_cast<char*>(str.c_str());
    return 0;
}

void ImGui::TextUnformatted(const std::string& s) {
    TextUnformatted(s.c_str());
}

bool ImGui::InputText(const char* label, std::string& s, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return InputText(label, const_cast<char*>(s.c_str()), s.capacity() + 1, flags, stringCallback, &s);
}

bool ImGui::InputTextMultiline(const char* label, std::string& s, const ImVec2& size, ImGuiInputTextFlags flags) {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return InputTextMultiline(label, const_cast<char*>(s.c_str()), s.capacity() + 1, size, flags, stringCallback, &s);
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

void ImGui::LoadingSpinner(const char* label) {
    Text("%s... %c", label, "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.05f) & 3]);
}

bool ImGui::BeginTabItemNoSpacing(const char* label) {
    PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
    bool result = BeginTabItem(label);
    PopStyleVar();
    return result;
}
