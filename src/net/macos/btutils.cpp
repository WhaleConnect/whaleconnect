// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <string_view>

#include <BluetoothMacOS-Swift.h>
#include <IOKit/IOReturn.h>

#include "os/check.hpp"

module net.btutils;
import net.btutils.internal;
import net.device;
import net.enums;
import os.error;
import os.errcheck;

BTUtils::Instance::Instance() = default;

BTUtils::Instance::~Instance() = default;

DeviceList BTUtils::getPaired() {
    auto list = BluetoothMacOS::getPairedDevices();
    DeviceList ret;

    for (const auto& i : list) ret.emplace_back(ConnectionType::None, i.getName(), i.getAddress(), 0);
    return ret;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool flushCache) {
    IOReturn result = kIOReturnSuccess;
    auto list = CHECK(
                    BluetoothMacOS::sdpLookup(addr.data(), uuid.data(), flushCache),
                    [](const BluetoothMacOS::LookupResult& result) { return result.getResult() == kIOReturnSuccess; },
                    [](const BluetoothMacOS::LookupResult& result) { return result.getResult(); },
                    System::ErrorType::IOReturn)
                    .getList();

    if (result != kIOReturnSuccess) throw System::SystemError{ result, System::ErrorType::IOReturn, "sdpLookup" };

    SDPResultList ret;

    for (const auto& i : list) {
        SDPResult result;

        for (const auto& proto : i.getProtoUUIDs()) result.protoUUIDs.push_back(proto);

        for (const auto& service : i.getServiceUUIDs()) {
            UUID128 uuid;
            std::memcpy(uuid.data(), service, 16);
            result.serviceUUIDs.push_back(uuid);
        }

        for (const auto& profile : i.getProfileDescs()) {
            ProfileDesc desc;
            Internal::extractVersionNums(profile.getVersion(), desc);
            result.profileDescs.push_back(desc);
        }

        result.port = i.getPort();
        result.name = i.getName();
        result.desc = i.getDesc();
        ret.push_back(result);
    }

    return ret;
}
#endif
