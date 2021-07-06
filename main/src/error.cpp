// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept> // std::out_of_range

#include "error.hpp"

Sockets::NamedError Sockets::getErr(int code) {
    try {
        // Attempt to get the element specified by the given code.
        return errors.at(code);
    } catch (const std::out_of_range&) {
        // std::unordered_map::at() throws an exception when the key is invalid.
        // This means the error code is not contained in the data structure and no NamedError corresponds to it.
        return { "UNKNOWN ERROR CODE", "No string is implemented for this error code." };
    }
}
