// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to represent a Dear ImGui window
*/

#pragma once

#include <string>
#include <string_view>

#include <imgui.h>

class Window {
    std::string _title;
    ImVec2 _size;
    bool _open = true;

    bool _initialized = false;

    virtual void _init() { /* May optionally be overridden in derived classes */ }

    virtual void _updateContents() = 0;

public:
    Window(std::string_view title, const ImVec2& size = {}) : _title(title), _size(size) {}

    virtual ~Window() = default;

    std::string_view getTitle() const { return _title; }

    bool isOpen() const { return _open; }

    void init() {
        if (_initialized) return;

        _init();
        _initialized = true;
    }

    void update() {
        if ((_size.x > 0) && (_size.y > 0)) ImGui::SetNextWindowSize(_size, ImGuiCond_FirstUseEver);

        if (ImGui::Begin(_title.c_str(), &_open)) _updateContents();
        ImGui::End();
    }
};
