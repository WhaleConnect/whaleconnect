// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class BooleanLock {
    bool& flag;

public:
    BooleanLock(bool& flag) : flag(flag) {
        this->flag = true;
    }

    ~BooleanLock() {
        flag = false;
    }
};
