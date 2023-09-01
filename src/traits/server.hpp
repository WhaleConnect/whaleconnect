// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "net/enums.hpp"

namespace Traits {
    template <auto Tag>
    struct Server {};

    template <>
    struct Server<SocketTag::IP> {
        IPType ip;
    };
}
