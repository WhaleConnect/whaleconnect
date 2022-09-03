// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <variant>

#include <imgui.h>
#include <imgui_internal.h>

#include "gui/sdpwindow.hpp"
#include "gui/windowlist.hpp"
#include "net/btutils.hpp"
#include "net/sockets.hpp"
#include "util/formatcompat.hpp"
#include "util/imguiext.hpp"
#include "util/overload.hpp"

static void sortTable(Sockets::DeviceDataList& devices) {
    if (devices.size() < 2) return;

    auto sortSpecs = ImGui::TableGetSortSpecs();
    if (!sortSpecs) return;
    if (!sortSpecs->SpecsDirty) return;

    auto spec = sortSpecs->Specs[0];
    auto proj = (spec.ColumnIndex == 0) ? &Sockets::DeviceData::name : &Sockets::DeviceData::address;

    if (spec.SortDirection == ImGuiSortDirection_Ascending) std::ranges::stable_sort(devices, std::less{}, proj);
    else std::ranges::stable_sort(devices, std::greater{}, proj);

    sortSpecs->SpecsDirty = false;
}

// Draws a menu composed of the paired Bluetooth devices.
static const Sockets::DeviceData* drawPairedDevices(Sockets::DeviceDataList& devices, bool needsSort) {
    const Sockets::DeviceData* ret = nullptr;

    int numCols = 3;
    if (!ImGui::BeginTable("paired", numCols, ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable
                           | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) return ret;

    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Address");
    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_NoSort);
    ImGui::TableSetupScrollFreeze(numCols, 1);
    ImGui::TableHeadersRow();

    if (needsSort) ImGui::TableSetColumnSortDirection(0, ImGuiSortDirection_Ascending, false);
    sortTable(devices);

    for (const auto& i : devices) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s", i.name.c_str());

        ImGui::TableNextColumn();
        ImGui::Text("%s", i.address.c_str());

        ImGui::TableNextColumn();
        ImGui::PushID(i.address.c_str());
        if (ImGui::Button("Connect")) ret = &i;
        ImGui::PopID();
    }

    ImGui::EndTable();
    return ret;
}

void drawBTConnectionTab(WindowList& connections, WindowList& sdpWindows) {
    if (!ImGui::BeginTabItem("Bluetooth")) return;

    static std::variant<std::monostate, Sockets::DeviceDataList, System::SystemError> pairedDevices; // Paired devices

    // Map of UUIDs, associating a UUID value with a user-given name
    static std::map<std::string, UUID, std::less<>> uuidList = {
        { "L2CAP", BTUtils::createUUIDFromBase(0x0100) },
        { "RFCOMM", BTUtils::createUUIDFromBase(0x0003) }
    };

    bool needsSort = false;

    if (ImGui::Button("Refresh List") || (pairedDevices.index() == 0)) {
        try {
            pairedDevices = BTUtils::getPaired();
            needsSort = true;
        } catch (const System::SystemError& error) {
            pairedDevices = error;
        }
    }

    std::visit(Overload{
        [](std::monostate) { /* Nothing to do when paired devices not found */ },
        [needsSort, &connections, &sdpWindows](Sockets::DeviceDataList& deviceList) {
            if (deviceList.empty()) {
                ImGui::Text("No paired devices.");
                return;
            }

            ImGui::Spacing();

            // There are devices, display them
            auto device = drawPairedDevices(deviceList, needsSort);
            if (device) sdpWindows.add<SDPWindow>(*device, uuidList, connections);
        },
        [](const System::SystemError& error) {
            // Error occurred
            ImGui::TextWrapped("Error %s", error.formatted().c_str());
        }
    }, pairedDevices);

    ImGui::EndTabItem();
}
