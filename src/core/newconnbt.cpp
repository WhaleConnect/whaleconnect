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

    auto sortSpec = sortSpecs->Specs[0];
    auto proj = (sortSpec.ColumnIndex == 0) ? &Sockets::DeviceData::name : &Sockets::DeviceData::address;

    if (sortSpec.SortDirection == ImGuiSortDirection_Ascending)
        std::ranges::stable_sort(devices, std::less{}, proj);
    else
        std::ranges::stable_sort(devices, std::greater{}, proj);

    sortSpecs->SpecsDirty = false;
}

// Draws a menu composed of the paired Bluetooth devices.
static const Sockets::DeviceData* drawPairedDevices(Sockets::DeviceDataList& devices, bool showAddrs, bool needsSort) {
    const Sockets::DeviceData* ret = nullptr;

    int numCols = showAddrs ? 3 : 2;
    if (!ImGui::BeginTable("paired", numCols,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) return ret;

    ImGui::TableSetupColumn("Name");
    if (showAddrs) ImGui::TableSetupColumn("Address");
    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_NoSort);
    ImGui::TableSetupScrollFreeze(numCols, 1);
    ImGui::TableHeadersRow();

    if (needsSort) ImGui::TableSetColumnSortDirection(0, ImGuiSortDirection_Ascending, false);
    sortTable(devices);

    for (const auto& i : devices) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s", i.name.c_str());

        if (showAddrs) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", i.address.c_str());
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Connect")) ret = &i;
    }

    ImGui::EndTable();
    return ret;
}

void drawBTConnectionTab(WindowList& connections) {
    if (!ImGui::BeginTabItem("Bluetooth")) return;

    static std::variant<std::monostate, Sockets::DeviceDataList, System::SystemError> pairedDevices; // Paired devices

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
        [needsSort](Sockets::DeviceDataList& deviceList) {
            if (deviceList.empty()) {
                ImGui::Text("No paired devices.");
                return;
            }

            // Checkbox to display device addresses
            static bool showAddrs = false;
            ImGui::SameLine();
            ImGui::Checkbox("Show Addresses", &showAddrs);
            ImGui::Spacing();

            // There are devices, display them
            drawPairedDevices(deviceList, showAddrs, needsSort);
        },
        [](const System::SystemError& error) {
            // Error occurred
            ImGui::TextWrapped("Error %s", error.formatted().c_str());
        }
    }, pairedDevices);

    ImGui::EndTabItem();
}
