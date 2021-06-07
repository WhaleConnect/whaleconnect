// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string> // std::string
#include <cstring> // std::strncmp()
#include <algorithm> // std::clamp() with C++17 or higher
#include <exception> // std::exception
#include <stdexcept> // std::invalid_argument

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "imguiext.hpp"

void ImGui::Text(const std::string& s) {
	Text(s.c_str());
}

void ImGui::Text(uint16_t i) {
	Text("%hu", i);
}

template<class T>
void ImGui::UnsignedInputScalar(const char* label, T& val, unsigned long min, unsigned long max) {
	// Decide data type of parameter
	ImGuiDataType dt;
	if (std::is_same_v<uint8_t, T>) dt = ImGuiDataType_U8; // Type of T is uint8_t
	else if (std::is_same_v<uint16_t, T>) dt = ImGuiDataType_U16; // Type of T is uint16_t
	else throw std::invalid_argument("This function only supports uint8_t/uint16_t data types");

	// Minimum and maximum
	if (max == 0) max = (dt == ImGuiDataType_U8) ? 255UL : 65535UL;

	// Char buffer to hold input
	constexpr int bufLen = 6;
	char buf[bufLen] = "";
	std::snprintf(buf, bufLen, "%d", val);

	BeginGroup();
	PushID(label);

	// Text filtering callback - only allow numeric digits (not including operators with InputCharsDecimal)
	auto filter = [](ImGuiInputTextCallbackData* data) -> int {
		return !(data->EventChar < 256 && std::strchr("0123456789", static_cast<char>(data->EventChar)));
	};

	// InputText widget
	SetNextItemWidth(50);
	if (InputText("", buf, bufLen, ImGuiInputTextFlags_CallbackCharFilter, filter)) {
		try {
			// Convert buffer to unsigned long
			val = static_cast<T>(std::clamp(std::stoul(buf), min, max));
		} catch (std::exception&) {
			// Exception thrown during conversion, set variable to minimum
			val = static_cast<T>(min);
		}
	}

	// Style config
	const float widgetSpacing = 2;
	const ImVec2 sz = { GetFrameHeight(), GetFrameHeight() }; // Button size

	// Step buttons: Subtract
	SameLine(0, widgetSpacing);
	if (ButtonEx("-", sz, ImGuiButtonFlags_Repeat)) if (val > min) val--;

	// Step buttons: Add
	SameLine(0, widgetSpacing);
	if (ButtonEx("+", sz, ImGuiButtonFlags_Repeat)) if (val < max) val++;

	// Widget label
	SameLine(0, widgetSpacing);
	Text(label);

	PopID();
	EndGroup();
}

// Explicit template declarations to avoid linker errors because the implementation for the function above is in the
// source file. Also, these are the only types it supports.
template void ImGui::UnsignedInputScalar<uint8_t>(const char*, uint8_t&, unsigned long, unsigned long);
template void ImGui::UnsignedInputScalar<uint16_t>(const char*, uint16_t&, unsigned long, unsigned long);

bool ImGui::InputText(const char* label, std::string* s, ImGuiInputTextFlags flags) {
	return InputText(label, const_cast<char*>(s->c_str()), s->capacity() + 1, flags
		| ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) {
		std::string* str = reinterpret_cast<std::string*>(data->UserData);
		IM_ASSERT(std::strncmp(data->Buf, str->c_str(), data->BufTextLen) == 0);
		str->resize(data->BufTextLen);
		data->Buf = const_cast<char*>(str->c_str());
		return 0;
	}, s);
}

void ImGui::HelpMarker(const char* desc) {
	SameLine();
	TextDisabled("(?)");
	if (IsItemHovered()) {
		BeginTooltip();
		Text(desc);
		EndTooltip();
	}
}
