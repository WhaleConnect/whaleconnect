// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A class to use as an asynchronous coroutine's return object

#pragma once

#include <coroutine>
#include <type_traits>

/// <summary>
/// A class to use as the return type for a coroutine performing asynchronous work.
/// </summary>
/// <typeparam name="T">The datatype of the value(s) produced by the coroutine</typeparam>
template <class T = void>
class Task {
    // If this template type is void-returning
    static constexpr bool _isVoid = std::is_void_v<T>;

    /// <summary>
    /// Base class containing definitions for a void-returning coroutine.
    /// </summary>
    struct PromiseTypeVoid {
        /// <summary>
        /// Called when the coroutine terminates.
        /// </summary>
        void return_void() noexcept {}
    };

    /// <summary>
    /// Base class containing definitions for a value-containing coroutine.
    /// </summary>
    /// <typeparam name="U">The type of the returned value (should not be void)</typeparam>
    template <class U>
    struct PromiseTypeValue {
    protected:
        // The value(s) produced by the coroutine
        U _data;

    public:
        /// <summary>
        /// Called when the coroutine returns.
        /// </summary>
        /// <param name="value">The value returned from the coroutine (the X in `co_return X;`)</param>
        void return_value(U value) noexcept {
            _data = std::move(value);
        }
    };

    /// <summary>
    /// The promise object containing the coroutine's information (returned value, exceptions).
    /// </summary>
    /// <remarks>
    /// This class inherits from either the "void" base or the "value" base depending on if T is void. This inheritance
    /// will give the class the appropriate return function for each template instantiation.
    /// </remarks>
    class PromiseType : std::conditional_t<_isVoid, PromiseTypeVoid, PromiseTypeValue<T>> {
        // In the case of another coroutine calling this one, keep track of the caller. This allows us to resume the
        // caller when this one exits (i.e. when `final_suspend()::await_suspend()` is hit).
        std::coroutine_handle<> _continuation;

    public:
        /// <summary>
        /// Called first when a coroutine is entered. This specifies the Task object returned from a coroutine function.
        /// </summary>
        /// <returns>A Task object with its coroutine handle derived from this promise object</returns>
        /// <remarks>
        /// Given a coroutine `Task myCoro();` the call `auto x = myCoro();` will return what's declared in here.
        /// </remarks>
        Task get_return_object() noexcept {
            return { ._handle = std::coroutine_handle<PromiseType>::from_promise(*this) };
        }

        /// <summary>
        /// Called second when a coroutine is entered. This dictates how the coroutine starts.
        /// </summary>
        /// <returns>`suspend_never` since we want to do work immediately</returns>
        std::suspend_never initial_suspend() const noexcept { return {}; }

        /// <summary>
        /// Handle any exceptions thrown in a coroutine.
        /// </summary>
        void unhandled_exception() {}

        /// <summary>
        /// Called when a coroutine is about to complete.
        /// </summary>
        /// <returns>An awaiter with final actions and logic to perform</returns>
        auto final_suspend() const noexcept {
            /// <summary>
            /// The awaiter containing final logic to complete.
            /// </summary>
            /// <remarks>
            /// This will be co_await-ed, so it must define all of `await_{ready, suspend, resume}()`.
            /// </remarks>
            struct Awaiter {
                /// <summary>
                /// Called first when this awaiter is co_await-ed.
                /// </summary>
                /// <returns>False to suspend the coroutine and invoke `await_suspend()`</returns>
                bool await_ready() const noexcept { return false; }

                /// <summary>
                /// Called when the coroutine is suspended.
                /// </summary>
                /// <param name="current">A coroutine handle for the current coroutine</param>
                /// <returns>A handle to the current coroutine's caller coroutine to resume</returns>
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> current) noexcept {
                    // Get the caller coroutine's handle (stored in the private variable)
                    auto continuation = current.promise()._continuation;

                    // Return the handle, or a no-op handle if there is no caller coroutine
                    return (continuation) ? continuation : std::noop_coroutine();
                }

                /// <summary>
                /// Called last when this awaiter is co_await-ed.
                /// </summary>
                void await_resume() const noexcept {}
            };
            return Awaiter{};
        }
    };

    // The handle for the coroutine performing work with this task object
    std::coroutine_handle<PromiseType> _handle;

public:
    /// <summary>
    /// Public typedef for the promise object.
    /// </summary>
    using promise_type = PromiseType;

    // The three await methods below allow us to co_await a task.

    /// <summary>
    /// Called first when this task is co_await-ed. This determines whether the coroutine needs to be suspended.
    /// </summary>
    /// <returns>If this task has nothing left to do</returns>
    bool await_ready() const noexcept {
        // There is no need to suspend if this task has no work to do.
        // If `done()` is false, this coroutine is suspended and `await_suspend()` is called.
        return _handle.done();
    }

    /// <summary>
    /// Called when the coroutine is suspended.
    /// </summary>
    /// <param name="current">A coroutine handle for the current coroutine</param>
    void await_suspend(std::coroutine_handle<> current) const noexcept {
        // Keep track of the current coroutine so it can be resumed in final_suspend
        _handle.promise()._continuation = current;
    }

    /// <summary>
    /// Called last when this task is co_await-ed. This is the result of the entire co_await expression.
    /// </summary>
    /// <returns>The value that the coroutine produced (for a value-returning coroutine)</returns>
    T await_resume() const noexcept {
        // Return a value if this template instantiation is non-void
        if constexpr (!_isVoid) return std::move(_handle.promise()._data);
    }
};
