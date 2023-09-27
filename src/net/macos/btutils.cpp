// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <bit>
#include <string_view>

#include "objc/btutils_bridge.hpp"

module net.btutils;
import net.device;
import net.enums;

BTUtils::Instance::Instance() = default;

BTUtils::Instance::~Instance() = default;

DeviceList BTUtils::getPaired() {
    auto list = BTUtilsBridge::getPaired();
    DeviceList ret;

    for (const auto& i : list) ret.emplace_back(ConnectionType::None, i.name, i.addr, 0);
    return ret;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool flushCache) {
    auto list = BTUtilsBridge::sdpLookup(addr, uuid.data(), flushCache);
    SDPResultList ret;

    for (const auto& i : list) ret.push_back(*std::bit_cast<SDPResult*>(&i));
    return ret;
}
#endif
