/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_IO_CONTEXT_HPP
#define LIBBITCOIN_NETWORK_IO_CONTEXT_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {
namespace io {

namespace detail { class scheduler; class strand_state; }

/// A completion handler (move-only, so it may own move-only captures).
class handler
{
public:
    DELETE_COPY(handler);

    handler() NOEXCEPT = default;
    handler(std::nullptr_t) NOEXCEPT
    {
    }

    template <typename Function, typename = std::enable_if_t<
        !std::is_same_v<std::decay_t<Function>, handler>>>
    handler(Function&& function) NOEXCEPT
      : function_(std::make_unique<model<std::decay_t<Function>>>(
          std::forward<Function>(function)))
    {
    }

    handler(handler&&) NOEXCEPT = default;
    handler& operator=(handler&&) NOEXCEPT = default;
    ~handler() NOEXCEPT = default;

    handler& operator=(std::nullptr_t) NOEXCEPT
    {
        function_.reset();
        return *this;
    }

    void operator()() NOEXCEPT
    {
        function_->invoke();
    }

    explicit operator bool() const NOEXCEPT
    {
        return static_cast<bool>(function_);
    }

private:
    struct callable
    {
        virtual ~callable() NOEXCEPT = default;
        virtual void invoke() NOEXCEPT = 0;
    };

    template <typename Function>
    struct model final
      : callable
    {
        template <typename Argument>
        model(Argument&& argument) NOEXCEPT
          : function(std::forward<Argument>(argument))
        {
        }

        void invoke() NOEXCEPT override
        {
            function();
        }

        Function function;
    };

    std::unique_ptr<callable> function_{};
};

class context;

/// An executor of a context, or of a strand of a context. Copies are
/// equivalent executors. Handlers posted to a strand run in post order and
/// never concurrently with each other.
class BCT_API executor
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(executor);

    executor(io::context& context) NOEXCEPT;
    executor(io::context& context,
        const std::shared_ptr<detail::strand_state>& strand) NOEXCEPT;

    /// The context of the executor.
    io::context& context() const NOEXCEPT;

    /// Queue the handler for execution (never invoked by this call).
    void post(handler&& handler) const NOEXCEPT;

    /// Invoke the handler now if this thread is running on the executor (or
    /// on its context while the strand is idle), otherwise post it.
    void dispatch(handler&& handler) const NOEXCEPT;

    /// This thread is running a handler of the executor (of the strand, or of
    /// the context if not a strand).
    bool running_in_this_thread() const NOEXCEPT;

    bool operator==(const executor& other) const NOEXCEPT;

private:
    io::context* context_;
    std::shared_ptr<detail::strand_state> strand_;
};

/// Not thread safe for destruction, otherwise thread safe.
/// Runs handlers and completes operations on the threads that call run().
class BCT_API context
{
public:
    DELETE_COPY_MOVE(context);

    using executor_type = executor;

    context() NOEXCEPT;
    ~context() NOEXCEPT;

    /// Run handlers until stopped or there is no work (queued handlers,
    /// pending operations or work guards), returning the number run.
    size_t run() NOEXCEPT;

    /// Make run() return as soon as possible (pending work is retained).
    void stop() NOEXCEPT;

    /// Stopped (by stop() or running out of work).
    bool stopped() const NOEXCEPT;

    /// Clear the stopped state (required before running again).
    void restart() NOEXCEPT;

    executor_type get_executor() NOEXCEPT;

    /// Queue the handler for execution (never invoked by this call).
    void post(handler&& handler) NOEXCEPT;

    /// Invoke the handler now if this thread is running the context,
    /// otherwise post it.
    void dispatch(handler&& handler) NOEXCEPT;

    /// This thread is running the context.
    bool running_in_this_thread() const NOEXCEPT;

    /// The scheduler of the context (io objects).
    detail::scheduler& scheduler() NOEXCEPT;

private:
    std::unique_ptr<detail::scheduler> scheduler_;
};

/// Keeps a context running (as work) while it exists and is not reset.
class BCT_API work_guard
{
public:
    DELETE_COPY(work_guard);

    work_guard(context& context) NOEXCEPT;
    work_guard(work_guard&& other) NOEXCEPT;
    ~work_guard() NOEXCEPT;

    void reset() NOEXCEPT;

private:
    context* context_;
};

/// Queue a handler on a context or executor.
BCT_API void post(context& context, handler&& handler) NOEXCEPT;
BCT_API void post(const executor& executor, handler&& handler) NOEXCEPT;

/// Invoke a handler now if running on the context or executor, else post it.
BCT_API void dispatch(context& context, handler&& handler) NOEXCEPT;
BCT_API void dispatch(const executor& executor, handler&& handler) NOEXCEPT;

} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
