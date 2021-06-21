// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string> // std::string
#include <cstring> // std::strncmp()
#include <algorithm> // std::clamp() with C++17 or higher
#include <exception> // std::exception
#include <stdexcept> // std::invalid_argument

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "imguiext.hpp"

void ImGui::TextUnformatted(const std::string& s) {
    TextUnformatted(s.c_str());
}

void ImGui::Text(uint16_t i) {
    Text("%hu", i);
}

bool ImGui::InputText(const char* label, std::string* s, ImGuiInputTextFlags flags) {
    return InputText(label, const_cast<char*>(s->c_str()), s->capacity() + 1, flags
        | ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) {
        std::string* str = reinterpret_cast<std::string*>(data->UserData);
        IM_ASSERT(std::strncmp(data->Buf, str->c_str(), data->BufTextLen) == 0);
        str->resize(data->BufTextLen);
        data->Buf = const_cast<char*>(str->c_str());
        return 0;
    }, s);
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
