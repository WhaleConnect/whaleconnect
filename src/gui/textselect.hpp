// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <imgui.h>
#include <unicode/unistr.h>

// Manages text selection in a GUI window.
// This class only works if the window only has text, and line wrapping is not supported.
// The window should also have the "NoMove" flag set so mouse drags can be used to select text.
class TextSelect {
    // Cursor position in the window.
    struct CursorPos {
        int x = -1; // X index of character
        int y = -1; // Y index of character

        // Checks if this position is invalid.
        bool isInvalid() const {
            // Invalid cursor positions are indicated by (-1, -1)
            return (x == -1) || (y == -1);
        }
    };

    // Text selection in the window.
    struct Selection {
        int startX;
        int startY;
        int endX;
        int endY;
    };

    // Selection bounds
    // In a selection, the start and end positions may not be in order (the user can click and drag left/up which
    // reverses start and end).
    CursorPos _selectStart;
    CursorPos _selectEnd;

    // Accessor functions to get line information
    // This class only knows about line numbers so it must be provided with functions that give it text data.
    std::function<icu::UnicodeString(size_t)> _getLineAtIdx; // Gets the string given a line number
    std::function<size_t()> _getNumLines;                    // Gets the total number of lines

    // Gets the user selection. Start and end are guaranteed to be in order.
    Selection _getSelection() const;

    // Processes mouse down (click/drag) events.
    void _handleMouseDown(const ImVec2& cursorPosStart);

    // Processes scrolling events.
    void _handleScrolling() const;

    // Draws the text selection rectangle in the window.
    void _drawSelection(const ImVec2& cursorPosStart) const;

public:
    // Sets the text accessor functions.
    // getLineAtIdx: Function taking a size_t (line number) and returning the string in that line in a UnicodeString
    // getNumLines: Function returning a size_t (total number of lines of text)
    template <class T, class U>
    explicit TextSelect(const T& getLineAtIdx, const U& getNumLines) :
        _getLineAtIdx(getLineAtIdx), _getNumLines(getNumLines) {}

    // Checks if there is an active selection in the text.
    bool hasSelection() const {
        return !_selectStart.isInvalid() && !_selectEnd.isInvalid();
    }

    // Copies the selected text to the clipboard.
    void copy() const;

    // Selects all text in the window.
    void selectAll();

    // Draws the text selection rectangle and handles user input.
    void update();
};