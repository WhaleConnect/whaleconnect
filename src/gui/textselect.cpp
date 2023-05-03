// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#define IMGUI_DEFINE_MATH_OPERATORS

#include "textselect.hpp"

#include <memory>
#include <numeric>

#include <imgui.h>
#include <imgui_internal.h>
#include <unicode/brkiter.h>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

// Word break iterator for double-click selection, created inside an IIFE to avoid leaking symbols to the global scope
static const auto bi = [] {
    using icu::BreakIterator;

    UErrorCode status = U_ZERO_ERROR;
    return std::unique_ptr<BreakIterator>{ BreakIterator::createWordInstance(icu::Locale::getRoot(), status) };
}();

// Gets the display width of a substring of a UnicodeString.
static float substringSizeX(const icu::UnicodeString& s, int32_t start, int32_t length = INT32_MAX) {
    std::string sub;

    s.tempSubString(start, length).toUTF8String(sub);
    return ImGui::CalcTextSize(sub.c_str()).x;
}

// Gets the index of the character the mouse cursor is over.
static int32_t getCharIndex(const icu::UnicodeString& s, float cursorPosX, int32_t start, int32_t end) {
    // Ignore cursor position when it is invalid
    if (cursorPosX < 0) return 0;

    // Check for exit conditions
    if (s.isEmpty()) return 0;
    if (end < start) return s.length();

    // Midpoint of given string range
    int32_t midIdx = std::midpoint(start, end);

    // Display width of the entire string up to the midpoint, gives the x-position where the (midIdx + 1)th char starts
    float widthToMid = substringSizeX(s, 0, midIdx + 1);

    // Same as above but exclusive, gives the x-position where the (midIdx)th char starts
    float widthToMidEx = substringSizeX(s, 0, midIdx);

    // Perform a recursive binary search to find the correct index
    // If the mouse position is between the (midIdx)th and (midIdx + 1)th character positions, the search ends
    if (cursorPosX < widthToMidEx) return getCharIndex(s, cursorPosX, start, midIdx - 1);
    else if (cursorPosX > widthToMid) return getCharIndex(s, cursorPosX, midIdx + 1, end);
    else return midIdx;
}

// Wrapper for getCharIndex providing the inditial bounds.
static int32_t getCharIndex(const icu::UnicodeString& s, float cursorPosX) {
    return getCharIndex(s, cursorPosX, 0, s.length());
}

// Gets the scroll delta for the given cursor position and window bounds.
static float getScrollDelta(float v, float min, float max) {
    const float scrollDelta = 3.0f;

    if (v < min) return -scrollDelta;
    else if (v > max) return scrollDelta;

    return 0.0f;
}

TextSelect::Selection TextSelect::_getSelection() const {
    // Start and end may be out of order (ordering is based on Y position)
    bool startBeforeEnd = _selectStart.y <= _selectEnd.y;

    // Reorder X points if necessary
    int startX = startBeforeEnd ? _selectStart.x : _selectEnd.x;
    int endX = startBeforeEnd ? _selectEnd.x : _selectStart.x;

    // Get min and max Y positions for start and end
    int startY = std::min(_selectStart.y, _selectEnd.y);
    int endY = std::max(_selectStart.y, _selectEnd.y);

    return { startX, startY, endX, endY };
}

void TextSelect::_handleMouseDown(const ImVec2& cursorPosStart) {
    const float textHeight = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 mousePos = ImGui::GetMousePos() - cursorPosStart;

    // Get Y position of mouse cursor, in terms of line number (capped to the index of the last line)
    int y = std::min(static_cast<int>(std::floor(mousePos.y / textHeight)), static_cast<int>(_getNumLines() - 1));

    icu::UnicodeString currentLine = _getLineAtIdx(y);
    int x = getCharIndex(currentLine, mousePos.x);

    // Get mouse click count and determine action
    if (int mouseClicks = ImGui::GetMouseClickedCount(ImGuiMouseButton_Left); mouseClicks > 0) {
        if (mouseClicks % 3 == 0) {
            // Triple click - select line
            _selectStart = { 0, y };
            _selectEnd = { currentLine.length(), y };
        } else if (mouseClicks % 2 == 0) {
            // Double click - select word
            bi->setText(currentLine);
            _selectStart = { bi->preceding(x + 1), y };
            _selectEnd = { bi->next(), y };
        } else if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
            // Single click with shift - select text from start to click
            // The selection starts from the beginning if no start position exists
            if (_selectStart.isInvalid()) _selectStart = { 0, 0 };

            _selectEnd = { x, y };
        } else {
            // Single click - set start position, invalidate end position
            _selectStart = { x, y };
            _selectEnd = { -1, -1 };
        }
    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        // Mouse dragging - set end position
        _selectEnd = { x, y };
    }
}

void TextSelect::_handleScrolling() const {
    // Window boundaries
    ImVec2 windowMin = ImGui::GetWindowPos();
    ImVec2 windowMax = windowMin + ImGui::GetWindowSize();

    // Get current and active window information from Dear ImGui state
    const ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
    const ImGuiWindow* activeWindow = GImGui->ActiveIdWindow;

    // If there is no active window or the current window is not active, do not handle scrolling
    if ((activeWindow == nullptr) || (activeWindow->ID != currentWindow->ID)) return;

    // Get scroll deltas from mouse position
    ImVec2 mousePos = ImGui::GetMousePos();
    float scrollXDelta = getScrollDelta(mousePos.x, windowMin.x, windowMax.x);
    float scrollYDelta = getScrollDelta(mousePos.y, windowMin.y, windowMax.y);

    // If there is a nonzero delta, scroll in that direction
    if (std::abs(scrollXDelta) > 0.0f) ImGui::SetScrollX(ImGui::GetScrollX() + scrollXDelta);
    if (std::abs(scrollYDelta) > 0.0f) ImGui::SetScrollY(ImGui::GetScrollY() + scrollYDelta);
}

void TextSelect::_drawSelection(const ImVec2& cursorPosStart) const {
    if (!hasSelection()) return;

    // Start and end positions
    auto [startX, startY, endX, endY] = _getSelection();

    // Add a rectangle to the draw list for each line contained in the selection
    for (int i = startY; i <= endY; i++) {
        icu::UnicodeString line = _getLineAtIdx(i);

        // Display sizes
        // The width of the space character is used for the width of newlines.
        const float newlineWidth = ImGui::CalcTextSize(" ").x;
        const float textHeight = ImGui::GetTextLineHeightWithSpacing();

        // The first and last rectangles should only extend to the selection boundaries
        // The middle rectangles (if any) enclose the entire line + some extra width for the newline.
        float minX = (i == startY) ? substringSizeX(line, 0, startX) : 0;
        float maxX = (i == endY) ? substringSizeX(line, 0, endX) : substringSizeX(line, 0) + newlineWidth;

        // Rectangle height equals text height
        float minY = static_cast<float>(i) * textHeight;
        float maxY = static_cast<float>(i + 1) * textHeight;

        // Get rectangle corner points offset from the cursor's start position in the window
        ImVec2 rectMin = cursorPosStart + ImVec2{ minX, minY };
        ImVec2 rectMax = cursorPosStart + ImVec2{ maxX, maxY };

        // Draw the rectangle
        ImU32 color = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
        ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, color);
    }
}

void TextSelect::copy() const {
    if (!hasSelection()) return;

    auto [startX, startY, endX, endY] = _getSelection();

    // Collect selected text in a single string
    icu::UnicodeString selectedText;

    for (int i = startY; i <= endY; i++) {
        // Similar logic to drawing selections
        int32_t subStart = (i == startY) ? startX : 0;
        int32_t subEnd = (i == endY) ? endX : INT32_MAX;

        icu::UnicodeString line = _getLineAtIdx(i);
        selectedText += line.tempSubStringBetween(subStart, subEnd);
    }

    // Convert the string to UTF-8 for copying
    std::string clipboardText;
    selectedText.toUTF8String(clipboardText);
    ImGui::SetClipboardText(clipboardText.c_str());
}

void TextSelect::selectAll() {
    size_t lastLineIdx = _getNumLines() - 1;
    icu::UnicodeString lastLine = _getLineAtIdx(lastLineIdx);

    // Set the selection range from the beginning to the end of the last line
    _selectStart = { 0, 0 };
    _selectEnd = { lastLine.length(), static_cast<int>(lastLineIdx) };
}

void TextSelect::update() {
    // ImGui::GetCursorStartPos() is in window coordinates so it is added to the window position
    ImVec2 cursorPosStart = ImGui::GetWindowPos() + ImGui::GetCursorStartPos();

    // Switch cursors if the window is hovered
    bool hovered = ImGui::IsWindowHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

    // Handle mouse events
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (hovered) _handleMouseDown(cursorPosStart);
        else _handleScrolling();
    }

    _drawSelection(cursorPosStart);

    ImGuiID windowID = ImGui::GetCurrentWindow()->ID;

    // Keyboard shortcuts
    if (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_A, windowID)) selectAll();
    else if (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_C, windowID)) copy();
}
