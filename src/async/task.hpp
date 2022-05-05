// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to use as an asynchronous coroutine's return object
*/

#pragma once

#include <coroutine>
#include <exception>
#include <type_traits>

/**
 * @brief A class to use as an asynchronous coroutine's return object.
 * @tparam T The datatype of the value(s) produced by the coroutine
*/
template <class T = void>
class Task {
    // If this template type is void-returning
    static constexpr bool _isVoid = std::is_void_v<T>;

    // Base class containing definitions for a void-returning coroutine.
    struct PromiseTypeVoid {
        void return_void() noexcept {}
    };

    // Base class containing definitions for a value-returning coroutine.
    template <class U>
    struct PromiseTypeValue {
        U data; // The value(s) produced by the coroutine

        void return_value(U value) noexcept {
            // The value parameter is the value in the co_return expression.
            data = std::move(value);
        }
    };

    // The promise object containing the coroutine's information (returned value, exceptions).
    // This class inherits from either the "void" base or the "value" base depending on if T is void. This inheritance
    // will give the class the appropriate return function for each template instantiation.
    struct PromiseType : std::conditional_t<_isVoid, PromiseTypeVoid, PromiseTypeValue<T>> {
        // In the case of another coroutine calling this one, keep track of the caller. This allows us to resume the
        // caller when this one exits.
        std::coroutine_handle<> continuation;

        std::exception_ptr exception; // Any exception that was thrown in the coroutine

        // Called first when a coroutine is entered. This specifies the Task object returned from a coroutine function.
        Task get_return_object() noexcept {
            return *this;
        }

        // Called second when a coroutine is entered. This dictates how the coroutine starts.
        // Returning suspend_never starts the coroutine immediately.
        std::suspend_never initial_suspend() const noexcept { return {}; }

        // Handles any exceptions thrown in a coroutine.
        void unhandled_exception() noexcept { exception = std::current_exception(); }

        // Called when a coroutine is about to complete.
        auto final_suspend() const noexcept {
            struct Awaiter {
                bool await_ready() const noexcept { return false; }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> current) noexcept {
                    // Get the caller coroutine's handle (stored in the private variable)
                    auto continuation = current.promise().continuation;

                    // Return the handle, or a no-op handle if there is no caller coroutine
                    // Returning the caller's handle allows it to be resumed.
                    return (continuation) ? continuation : std::noop_coroutine();
                }

                void await_resume() const noexcept {}
            };
            return Awaiter{};
        }
    };

    // The handle for the coroutine performing work with this task object
    std::coroutine_handle<PromiseType> _handle;

    // Constructs a task object from a coroutine promise object.
    Task(PromiseType& promiseType) : _handle(std::coroutine_handle<PromiseType>::from_promise(promiseType)) {}

public:
    /**
     * @brief Typedef for the promise object for use by the compiler.
    */
    using promise_type = PromiseType;

    // The three await methods below allow us to co_await a task.

    /**
     * @brief Determines whether the coroutine needs to be suspended.
     * @return If this task has completed or has nothing to do
     *
     * Called first when the coroutine is awaited.
    */
    bool await_ready() const noexcept {
        // There is no need to suspend if this task has no work to do.
        // If done() is false, this coroutine is suspended and await_suspend is called.
        return _handle.done();
    }

    /**
     * @brief Keeps track of the current coroutine to resume on suspend.
     * @param current A handle to the current coroutine
     *
     * Called when the coroutine is suspended.
    */
    void await_suspend(std::coroutine_handle<> current) const noexcept {
        // Keep track of the current coroutine so it can be resumed in final_suspend
        _handle.promise().continuation = current;
    }

    /**
     * @brief Returns the result of the entire co_await expression.
     * @return The value that the coroutine produced (for a value-returning coroutine)
     *
     * Called last when the coroutine is awaited.
    */
    T await_resume() const {
        // Propagate any exception that was thrown inside the coroutine to the caller
        const auto& exception = _handle.promise().exception;
        if (exception) std::rethrow_exception(exception);

        // Return a value if this template instantiation is non-void
        if constexpr (!_isVoid) return std::move(_handle.promise().data);
    }
};
