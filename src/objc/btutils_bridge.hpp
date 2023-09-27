// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS
#include <array>
#include <string>
#include <vector>

namespace BTUtilsBridge {
    struct ObjCDevice {
        std::string name;
        std::string addr;
    };

    std::vector<ObjCDevice> getPaired();

    using ObjCUUID128 = std::array<uint8_t, 16>;

    struct ObjCProfileDesc {
        uint16_t uuid = 0;
        uint8_t versionMajor = 0;
        uint8_t versionMinor = 0;
    };

    struct ObjCSDPResult {
        std::vector<uint16_t> protoUUIDs;
        std::vector<ObjCUUID128> serviceUUIDs;
        std::vector<ObjCProfileDesc> profileDescs;
        uint16_t port = 0;
        std::string name;
        std::string desc;
    };

    void extractVersionNums(uint16_t version, ObjCProfileDesc& pd);

    std::vector<ObjCSDPResult> sdpLookup(std::string_view addr, uint8_t* uuid, bool flushCache);
}
#endif
