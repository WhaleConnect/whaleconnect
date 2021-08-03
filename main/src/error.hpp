// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <unordered_map> // std::unordered_map

namespace Sockets {
    /// <summary>
    /// A structure to represent an error with a symbolic name and a description.
    /// </summary>
    /// <remarks>
    /// A symbolic name is a human-readable identifier for the error (e.g. "ENOMEM").
    /// A description is a short string describing the error (e.g. "No more memory").
    /// </remarks>
    struct NamedError {
        const char* name; // Symbolic name
        const char* desc; // Description
    };

    // The data structure to act as a lookup table for error codes. It maps numeric codes to NamedErrors.
    // It is an unordered_map to store key/value pairs and to provide constant O(1) access time complexity.
    extern const std::unordered_map<long, NamedError> errors;

    /// <summary>
    /// Get the corresponding NamedError from a numeric code.
    /// </summary>
    /// <param name="code">The numeric error code</param>
    /// <returns>The NamedError describing the error specified by the code</returns>
    NamedError getErr(int code);
}
