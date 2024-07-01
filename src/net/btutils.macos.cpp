// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "btutils.hpp"

#include <cstring>
#include <vector>

#include <BluetoothMacOS-Swift.h>
#include <IOKit/IOReturn.h>

#include "btutils.internal.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "os/errcheck.hpp"
#include "os/error.hpp"

BTUtils::Instance::Instance() = default;

BTUtils::Instance::~Instance() = default;

std::vector<Device> BTUtils::getPaired() {
    auto list = BluetoothMacOS::getPairedDevices();
    std::vector<Device> ret;

    // TODO: Replace with emplace_back when Apple Clange supports P0960
    for (const auto& i : list) ret.push_back({ ConnectionType::None, i.getName(), i.getAddress(), 0 });
    return ret;
}

std::vector<BTUtils::SDPResult> BTUtils::sdpLookup(std::string_view addr, UUIDs::UUID128 uuid, bool flushCache) {
    auto list = check(
        BluetoothMacOS::sdpLookup(addr.data(), uuid.data(), flushCache),
        [](const BluetoothMacOS::LookupResult& result) { return result.getResult() == kIOReturnSuccess; },
        [](const BluetoothMacOS::LookupResult& result) { return result.getResult(); }, System::ErrorType::IOReturn)
                    .getList();

    std::vector<BTUtils::SDPResult> ret;

    for (const auto& i : list) {
        SDPResult result{ .port = i.getPort(), .name = i.getName(), .desc = i.getDesc() };

        for (const auto& proto : i.getProtoUUIDs()) result.protoUUIDs.push_back(proto);

        for (const auto& service : i.getServiceUUIDs()) {
            UUIDs::UUID128 uuid;
            std::memcpy(uuid.data(), service, 16);
            result.serviceUUIDs.push_back(uuid);
        }

        for (const auto& profile : i.getProfileDescs()) {
            ProfileDesc desc;
            desc.uuid = profile.getUuid();
            Internal::extractVersionNums(profile.getVersion(), desc);
            result.profileDescs.push_back(desc);
        }

        ret.push_back(result);
    }

    return ret;
}
