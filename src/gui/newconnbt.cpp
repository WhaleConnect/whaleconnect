// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <algorithm>
#include <map>
#include <variant>

#ifdef _MSC_VER
// Prevents an error in stable_sort with modules
#include <ranges>
#endif

#include <imgui.h>
#include <imgui_internal.h>

module gui.newconnbt;
import components.sdpwindow;
import components.windowlist;
import net.btutils;
import net.device;
import os.error;
import utils.overload;

void sortTable(DeviceList& devices) {
    // A sort is only needed for 2 or more entries
    if (devices.size() < 2) return;

    // Get sort specs
    auto sortSpecs = ImGui::TableGetSortSpecs();
    if (!sortSpecs) return;
    if (!sortSpecs->SpecsDirty) return;

    // Get sort data
    auto spec = sortSpecs->Specs[0];
    auto proj = spec.ColumnIndex == 0 ? &Device::name : &Device::address;

    // Sort according to user specified direction
    if (spec.SortDirection == ImGuiSortDirection_Ascending) std::ranges::stable_sort(devices, std::less{}, proj);
    else std::ranges::stable_sort(devices, std::greater{}, proj);

    sortSpecs->SpecsDirty = false;
}

// Draws a menu composed of the paired Bluetooth devices.
const Device* drawPairedDevices(DeviceList& devices, bool needsSort) {
    // Using a pointer so the return value can be nullable without copying large objects.
    const Device* ret = nullptr;

    // Setup table
    int numCols = 3;
    ImGuiTableFlags flags
        = ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
    if (!ImGui::BeginTable("paired", numCols, flags)) return ret;

    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Address");
    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_NoSort);
    ImGui::TableSetupScrollFreeze(numCols, 1);
    ImGui::TableHeadersRow();

    // The default sort is ascending according to column 0
    if (needsSort) ImGui::TableSetColumnSortDirection(0, ImGuiSortDirection_Ascending, false);
    sortTable(devices);

    // Render each table row
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

    static std::variant<std::monostate, DeviceList, System::SystemError> pairedDevices; // Paired devices

    // Map of UUIDs, associating a UUID value with a user-given name
    static std::map<std::string, BTUtils::UUID128, std::less<>> uuidList = {
        { "L2CAP", BTUtils::createUUIDFromBase(0x0100) },
        { "RFCOMM", BTUtils::createUUIDFromBase(0x0003) },
    };

    bool needsSort = false;

    // Get paired devices if the button is clicked or if the variant currently holds nothing
    if (ImGui::Button("Refresh List") || pairedDevices.index() == 0) {
        try {
            pairedDevices = BTUtils::getPaired();
            needsSort = true;
        } catch (const System::SystemError& error) {
            pairedDevices = error;
        }
    }

    // Check paired devices list
    Overload visitor{
        [](std::monostate) { /* Nothing to do */ },
        [needsSort, &connections, &sdpWindows](DeviceList& deviceList) {
            // Check if the device list is empty
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
            ImGui::TextWrapped("Error %s", error.what());
        },
    };

    std::visit(visitor, pairedDevices);
    ImGui::EndTabItem();
}
