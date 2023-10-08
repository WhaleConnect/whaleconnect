// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <array>
#include <mutex>
#include <optional>
#include <string>

#include <imgui.h>

module gui.ioconsole;
import gui.imguiext;
import utils.strings;

void IOConsole::drawControls() {
    // "Clear output" button
    if (ImGui::Button("Clear output")) console.clear();

    // "Options" button
    ImGui::SameLine();
    if (ImGui::Button("Options...")) ImGui::OpenPopup("options");

    // Popup for more options
    if (ImGui::BeginPopup("options")) {
        console.drawOptions();

        // Options for the input textbox
        ImGui::Separator();
        ImGui::MenuItem("Send echoing", nullptr, &sendEchoing);
        ImGui::MenuItem("Clear texbox on send", nullptr, &clearTextboxOnSubmit);
        ImGui::MenuItem("Add final line ending", nullptr, &addFinalLineEnding);
        ImGui::EndPopup();
    }

    // Line ending combobox
    // The code used to calculate where to put the combobox is derived from
    // https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
    using namespace ImGui::Literals;

    float comboWidth = 10_fh;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));
    ImGui::SetNextItemWidth(comboWidth);
    ImGui::Combo("##lineEnding", &currentLE, "Newline\0Carriage return\0Both\0");
}

std::optional<std::string> IOConsole::update(int numLines) {
    std::optional<std::string> ret;

    // Apply foxus to textbox
    // An InputTextMultiline is an InputText contained within a child window so focus must be set before rendering it to
    // apply focus to the InputText.
    if (focusOnTextbox) {
        ImGui::SetKeyboardFocusHere();
        focusOnTextbox = false;
    }

    // Textbox
    using namespace ImGui::Literals;

    float textboxHeight = 4_fh; // Number of lines that can be displayed
    ImVec2 size{ ImGui::FILL, textboxHeight };
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue
                              | ImGuiInputTextFlags_AllowTabInput;

    if (ImGui::InputTextMultiline("##input", textBuf, size, flags)) {
        // Line ending
        std::array endings{ "\n", "\r", "\r\n" };
        auto selectedEnding = endings[currentLE];

        // InputTextMultiline() always uses \n as a line ending, replace all occurences of \n with the selected ending
        std::string sendString = Strings::replaceAll(textBuf, "\n", selectedEnding);

        // Add a final line ending if set
        if (addFinalLineEnding) sendString += selectedEnding;

        // Invoke the callback function if the string is not empty
        if (!sendString.empty()) {
            if (sendEchoing) console.addMessage(sendString, "SENT ", { 0.28f, 0.67f, 0.68f, 1 });

            ret = sendString;
        }

        // Blank out input textbox
        if (clearTextboxOnSubmit) textBuf.clear();

        focusOnTextbox = true;
    }

    // Reserve space at bottom
    float y = -static_cast<float>(numLines + 1) * ImGui::GetFrameHeightWithSpacing();
    console.update("console", ImVec2{ ImGui::FILL, y });
    drawControls();

    return ret;
}

void IOConsole::errorHandler(const System::SystemError& error) {
    // Check for non-fatal errors, then add error line to console
    // Don't handle errors caused by I/O cancellation
    if (error && !error.isCanceled()) {
        std::scoped_lock g{ consoleMutex };
        console.addError(error.what());
    }
}
