// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string> // std::string

/// <summary>
/// Namespace containing add-on functions for Dear ImGui. Members of this namespace may not necessarily follow the style
/// guide in order to remain consistent with Dear ImGui's naming conventions. Unused return values or parameters present
/// in the original Dear ImGui API of which these functions are derived from may be removed to better fit the usage of
/// the functions.
/// </summary>
namespace ImGui {
	/// <summary>
	/// Wrapper function for ImGui::Text() to allow a std::string parameter.
	/// </summary>
	/// <param name="s">The string to display</param>
	void Text(const std::string& s);

	/// <summary>
	/// Wrapper function for ImGui::Text() to allow a uint16_t parameter.
	/// </summary>
	/// <param name="i">The uint16_t to display</param>
	void Text(uint16_t i);

	/// <summary>
	/// An adapted InputScalar() function with operator handling removed. Only works with uint8_t/uint16_t types.
	/// </summary>
	/// <typeparam name="T">The type of the integer passed to the function (8- or 16-bit uint)</typeparam>
	/// <param name="label">The text to show next to the input</param>
	/// <param name="val">The variable to pass to the internal InputText()</param>
	/// <param name="min">The minimum value to allow (optional)</param>
	/// <param name="max">The maximum value to allow (optional)</param>
	template<class T>
	void UnsignedInputScalar(const char* label, T& val, unsigned long min = 0, unsigned long max = 0);

	/// <summary>
	/// An adapted InputText() to use a std::string passed by reference.
	/// </summary>
	/// <param name="label">The text to show next to the input</param>
	/// <param name="s">A pointer to the std::string buffer to use</param>
	/// <param name="flags">A set of ImGuiInputTextFlags to change how the textbox behaves</param>
	/// <returns>The value from InputText() called internally</returns>
	/// <remarks>
	/// The implementation for this function is copied from /misc/cpp/imgui_stdlib.cpp in the Dear ImGui GitHub repo,
	/// with slight modifications.
	/// </remarks>
	bool InputText(const char* label, std::string* s, ImGuiInputTextFlags flags = 0);

	/// <summary>
	/// Create a (?) mark which shows a tooltip on hover. Placed next to an element to provide more details about it.
	/// </summary>
	/// <param name="desc">The text in the tooltip</param>
	/// <remarks>
	/// Adapted from imgui_demo.cpp.
	/// </remarks>
	void HelpMarker(const char* desc);
}
