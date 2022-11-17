// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "btutils_internal.hpp"

void BTUtils::init() {}

void BTUtils::cleanup() {}

Net::DeviceDataList BTUtils::getPaired() { return {}; }

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool) {
    return {};
}
#endif
