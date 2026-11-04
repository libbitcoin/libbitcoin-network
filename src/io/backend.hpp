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
#ifndef LIBBITCOIN_NETWORK_IO_BACKEND_HPP
#define LIBBITCOIN_NETWORK_IO_BACKEND_HPP

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/io/buffer.hpp>
#include <bitcoin/network/io/context.hpp>
#include "native.hpp"
#include "scheduler.hpp"

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

struct operation;

/// The state of an open socket, shared by its operations.
struct descriptor
{
    descriptor(detail::scheduler& scheduler) NOEXCEPT
      : scheduler(scheduler)
    {
    }

    detail::scheduler& scheduler;
    native_socket handle{ invalid_socket };
    bool v6{};
    bool bound{};
    bool non_blocking{};

    // Incremented by cancel and close, so that the failure of an operation
    // started before is reported as operation_canceled.
    std::atomic<size_t> cancels{};

#if !defined(HAVE_WINDOWS)
    // The reactor registration and queues (protected by mutex).
    uint64_t key{};
    std::mutex mutex{};
    std::deque<operation*> reads{};
    std::deque<operation*> writes{};
#endif
};

using descriptor_ptr = std::shared_ptr<descriptor>;

/// A pending socket operation (deleted upon completion).
struct operation
{
    enum class kind
    {
        read,
        write,
        accept,
        connect
    };

    using io_handler = std::function<void(const code&, size_t)>;

    operation(kind type, const descriptor_ptr& owner,
        const io::executor& executor) NOEXCEPT
      : type(type), owner(owner), executor(executor)
    {
    }

#if defined(HAVE_WINDOWS)
    // The overlapped structure must be first (cast from the completion).
    OVERLAPPED overlapped{};
    std::vector<WSABUF> buffers{};
    native_socket accepted{ invalid_socket };
    std::vector<uint8_t> addresses{};
#else
    mutable_buffers in{};
    const_buffers out{};
#endif

    const kind type;
    const descriptor_ptr owner;
    descriptor_ptr peer{};
    socket_address target{};
    io::executor executor;
    io_handler handler{};
    size_t cancels{};
    size_t requested{};
    size_t bytes{};
    code ec{};
};

/// The platform completion port (windows) or reactor (epoll, kqueue).
class backend
{
public:
    DELETE_COPY_MOVE(backend);

    explicit backend(detail::scheduler& scheduler) NOEXCEPT;
    ~backend() NOEXCEPT;

    /// Threads may wait at once, a completion or interrupt returning one of
    /// them (otherwise one thread waits at a time).
#if defined(HAVE_APPLE)
    static constexpr bool concurrent = false;
#else
    static constexpr bool concurrent = true;
#endif

    /// Wait up to the timeout (none is unbounded) for completions, appending
    /// them to out. Interrupt returns a waiting call early.
    void wait(const timeout& duration, std::vector<handler>& out) NOEXCEPT;
    void interrupt() NOEXCEPT;

#if defined(HAVE_WINDOWS)
    /// Queue the handler on the completion port (returned by a wait).
    void post(handler&& work) NOEXCEPT;

    /// Destroy the handlers queued on the completion port without running
    /// them (true if any).
    bool drain() NOEXCEPT;
#endif

    /// Open a socket of the family into the closed descriptor.
    bool open(const descriptor_ptr& descriptor, bool v6, int& native) NOEXCEPT;

    /// Register an accepted socket into the closed descriptor.
    bool adopt(const descriptor_ptr& descriptor, native_socket socket,
        bool v6, int& native) NOEXCEPT;

    /// Cancel pending operations (complete with operation_canceled).
    void cancel(descriptor& descriptor) NOEXCEPT;

    /// Cancel pending operations and close the socket.
    void close(descriptor& descriptor) NOEXCEPT;

    /// Start the operation (counted as work until its completion has run).
    void start(operation* op) NOEXCEPT;

    /// The result of a completed operation (canceled if canceled since start).
    static void set_result(operation* op, const code& ec,
        size_t bytes) NOEXCEPT;

    /// The completion that delivers the operation's result to its executor.
    static handler to_completion(operation* op) NOEXCEPT;

private:

#if defined(HAVE_WINDOWS)
    void start_read(operation* op) NOEXCEPT;
    void start_write(operation* op) NOEXCEPT;
    void start_accept(operation* op) NOEXCEPT;
    void start_connect(operation* op) NOEXCEPT;
    void finish_accept(operation* op) NOEXCEPT;
    void finish_connect(operation* op) NOEXCEPT;
    bool associate(native_socket socket, int& native) NOEXCEPT;
    void fail(operation* op, int native) NOEXCEPT;

    HANDLE port_{};
    LPFN_ACCEPTEX accept_ex_{};
    LPFN_CONNECTEX connect_ex_{};
#else
    bool add(const descriptor_ptr& descriptor, int& native) NOEXCEPT;
    bool perform(operation* op) NOEXCEPT;
    void ready(const descriptor_ptr& descriptor, bool read, bool write,
        std::vector<handler>& out) NOEXCEPT;

    int handle_{ -1 };
#if defined(HAVE_APPLE)
    int interrupter_[2]{ -1, -1 };
#else
    int interrupter_{ -1 };
#endif

    // The registered descriptors (by key), protected by mutex_.
    std::mutex mutex_{};
    std::unordered_map<uint64_t, std::weak_ptr<descriptor>> descriptors_{};
    uint64_t next_key_{};
#endif

    detail::scheduler& scheduler_;
};

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
