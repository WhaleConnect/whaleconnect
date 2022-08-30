// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A basic class to manage system handles with RAII
*/

#include <functional>
#include <type_traits>

/**
 * @brief A basic class to manage system handles with RAII.
 * @tparam T The type of the managed handle
 *
 * This is a minimal class designed to avert memory leaks. It should only be used to replace raw handle variables.
*/
template <class T>
class HandleWrapper {
    T _handle; // The managed system handle
    std::function<void(T)> _deleter; // The function to free the handle

public:
    /**
     * @brief Constructs an object not owning a handle.
     * @tparam Fn The type of the deleter function
     * @param deleter The handle's deleter function
    */
    template <class Fn>
    explicit HandleWrapper(const Fn& deleter) : HandleWrapper({}, deleter) {}

    /**
     * @brief Constructs an object owning a handle.
     * @tparam Fn The type of the deleter function
     * @param handle The handle to take ownership of
     * @param deleter The handle's deleter function
    */
    template <class Fn>
    HandleWrapper(const T& handle, const Fn& deleter) : _handle(handle), _deleter(deleter) {}

    /**
     * @brief Calls the deleter function on the owned handle.
    */
    ~HandleWrapper() {
        if (_handle) _deleter(_handle);
    }

    /**
     * @brief Gets the managed handle.
     * @return A reference to the handle
    */
    T& get() { return _handle; }

    /**
     * @brief Gets a pointer to the managed handle.
     * @return A pointer to the handle
    */
    T* ptr() { return &_handle; }

    /**
     * @brief Gets the managed handle if it is of a pointer type.
     * @return The handle
    */
    T operator->() requires std::is_pointer_v<T> { return _handle; }
};
