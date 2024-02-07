// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "guibridge.hpp"

import gui.menu;
import external.imgui;
import external.std;

void setWindowFocus(const std::string& title) {
    ImGui::SetWindowFocus(title.c_str());
}

void openSettingsWindow() {
    Menu::settingsOpen = true;
}

void openNewConnectionWindow() {
    Menu::newConnectionOpen = true;
}

void openNewServerWindow() {
    Menu::newServerOpen = true;
}

void openNotificationsWindow() {
    Menu::notificationsOpen = true;
}

void openAboutWindow() {
    Menu::aboutOpen = true;
}
