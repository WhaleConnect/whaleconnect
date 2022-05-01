// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A function to create a smart pointer owning a handle pointer

#include <functional>
#include <type_traits>

template <class T>
class HandleWrapper {
    T _handle{};
    std::function<void(T)> _deleter;

public:
    template <class Fn>
    HandleWrapper(Fn&& freeFn) : HandleWrapper({}, freeFn) {}

    template <class Fn>
    HandleWrapper(T handle, Fn&& freeFn) : _handle(handle), _deleter(freeFn) {}

    ~HandleWrapper() {
        if (_handle) _deleter(_handle);
    }

    T& get() { return _handle; }

    T* ptr() { return &_handle; }

    template <class = std::enable_if_t<std::is_pointer_v<T>>>
    T operator->() { return _handle; }
};
