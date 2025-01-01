// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Menu {
    inline bool settingsOpen = false;
    inline bool newConnectionOpen = true;
    inline bool newServerOpen = false;
    inline bool notificationsOpen = false;
    inline bool aboutOpen = false;
    inline bool linksOpen = false;

    void setWindowFocus(const char* title);
}
