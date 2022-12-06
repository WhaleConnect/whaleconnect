// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "os/btutils_internal.hpp"
#include "plat_macos_objc/paired.hpp"
#include "plat_macos_objc/sdp.hpp"

void BTUtils::init() {}

void BTUtils::cleanup() {}

DeviceList BTUtils::getPaired() {
    auto info = ObjC::getPaired();

    DeviceList ret;
    for (const auto& i : info) ret.push_back({ ConnectionType::None, i.name, i.addr, 0 });

    return ret;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool flushCache) {
    IOReturn lookupRes = ObjC::sdpLookup(addr.data(), uuid.data(), flushCache);
    SDPResultList ret;
    return ret;
}
#endif
