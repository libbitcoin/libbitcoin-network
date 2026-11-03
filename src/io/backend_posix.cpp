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
#include "backend.hpp"

#if !defined(HAVE_WINDOWS)

#include <chrono>
#include <limits>
#include <bitcoin/network/define.hpp>

#if defined(HAVE_APPLE)
    #include <sys/event.h>
    #include <sys/time.h>
#else
    #include <sys/epoll.h>
    #include <sys/eventfd.h>
#endif

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_REINTERPRET_CAST)

#if defined(MSG_NOSIGNAL)
    constexpr int send_flags = MSG_NOSIGNAL;
#else
    constexpr int send_flags = 0;
#endif

// The registration key of the interrupter (descriptors are keyed from one).
constexpr uint64_t interrupter_key = 0;

static void set_close_on_exec(int socket) NOEXCEPT
{
    ::fcntl(socket, F_SETFD, FD_CLOEXEC);
}

backend::backend(detail::scheduler& scheduler) NOEXCEPT
  : next_key_(add1(interrupter_key)), scheduler_(scheduler)
{
#if defined(HAVE_APPLE)
    if (::pipe(interrupter_) == 0)
    {
        int native{};
        set_close_on_exec(interrupter_[0]);
        set_close_on_exec(interrupter_[1]);
        set_blocking(interrupter_[0], false, native);
        set_blocking(interrupter_[1], false, native);
    }

    handle_ = ::kqueue();
    set_close_on_exec(handle_);
    struct kevent event{};
    EV_SET(&event, interrupter_[0], EVFILT_READ, EV_ADD, 0, 0,
        reinterpret_cast<void*>(interrupter_key));
    ::kevent(handle_, &event, 1, nullptr, 0, nullptr);
#else
    interrupter_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
    handle_ = ::epoll_create1(EPOLL_CLOEXEC);
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.u64 = interrupter_key;
    ::epoll_ctl(handle_, EPOLL_CTL_ADD, interrupter_, &event);
#endif
}

backend::~backend() NOEXCEPT
{
    ::close(handle_);
#if defined(HAVE_APPLE)
    ::close(interrupter_[0]);
    ::close(interrupter_[1]);
#else
    ::close(interrupter_);
#endif
}

// Wait.
// ----------------------------------------------------------------------------

void backend::wait(const timeout& duration,
    std::vector<handler>& out) NOEXCEPT
{
    size_t capacity{};
    {
        std::unique_lock lock{ mutex_ };
        capacity = add1(descriptors_.size());
    }

#if defined(HAVE_APPLE)
    timespec time{};
    if (duration.has_value())
    {
        const auto seconds = std::chrono::floor<std::chrono::seconds>(
            duration.value());

        time.tv_sec = seconds.count();
        time.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
            duration.value() - seconds).count();
    }

    // Each descriptor has a read and a write filter.
    std::vector<struct kevent> events(capacity * two);
    const auto count = ::kevent(handle_, nullptr, 0, events.data(),
        possible_narrow_sign_cast<int>(events.size()),
        duration.has_value() ? &time : nullptr);
#else
    auto milliseconds = -1;
    if (duration.has_value())
    {
        const auto value = std::chrono::ceil<std::chrono::milliseconds>(
            duration.value()).count();

        milliseconds = possible_narrow_cast<int>(std::clamp<int64_t>(value,
            0, std::numeric_limits<int>::max()));
    }

    std::vector<epoll_event> events(capacity);
    const auto count = ::epoll_wait(handle_, events.data(),
        possible_narrow_sign_cast<int>(events.size()), milliseconds);
#endif

    for (auto index = 0; index < count; ++index)
    {
        const auto& event = events.at(index);

#if defined(HAVE_APPLE)
        const auto key = reinterpret_cast<uint64_t>(event.udata);
        const auto read = event.filter == EVFILT_READ;
        const auto write = event.filter == EVFILT_WRITE;
#else
        const auto key = event.data.u64;
        const auto flags = event.events;
        const auto failed = is_nonzero(flags & (EPOLLERR | EPOLLHUP));
        const auto read = failed || is_nonzero(flags & (EPOLLIN | EPOLLRDHUP));
        const auto write = failed || is_nonzero(flags & EPOLLOUT);
#endif

        if (key == interrupter_key)
        {
#if defined(HAVE_APPLE)
            uint8_t sink[sizeof(uint64_t)]{};
            while (::read(interrupter_[0], sink, sizeof(sink)) > 0);
#else
            uint64_t unit{};
            const auto ignore = ::read(interrupter_, &unit, sizeof(unit));
            static_cast<void>(ignore);
#endif
            continue;
        }

        descriptor_ptr descriptor{};
        {
            std::unique_lock lock{ mutex_ };
            const auto it = descriptors_.find(key);
            if (it != descriptors_.end())
                descriptor = it->second.lock();
        }

        if (descriptor)
            ready(descriptor, read, write, out);
    }
}

void backend::interrupt() NOEXCEPT
{
#if defined(HAVE_APPLE)
    const uint8_t byte{};
    const auto ignore = ::write(interrupter_[1], &byte, sizeof(byte));
#else
    const uint64_t unit{ 1 };
    const auto ignore = ::write(interrupter_, &unit, sizeof(unit));
#endif
    static_cast<void>(ignore);
}

// Descriptors.
// ----------------------------------------------------------------------------

bool backend::open(const descriptor_ptr& descriptor, bool v6,
    int& native) NOEXCEPT
{
    const auto socket = create_socket(v6, native);
    if (socket == invalid_socket)
        return false;

    descriptor->handle = socket;
    descriptor->v6 = v6;
    descriptor->bound = false;
    if (!add(descriptor, native))
    {
        close_socket(socket);
        descriptor->handle = invalid_socket;
        return false;
    }

    return true;
}

bool backend::adopt(const descriptor_ptr& descriptor, native_socket socket,
    bool v6, int& native) NOEXCEPT
{
#if defined(SO_NOSIGPIPE)
    const int enable{ 1 };
    ::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enable, sizeof(enable));
#endif

    descriptor->handle = socket;
    descriptor->v6 = v6;
    descriptor->bound = true;
    if (!add(descriptor, native))
    {
        descriptor->handle = invalid_socket;
        return false;
    }

    return true;
}

void backend::cancel(descriptor& descriptor) NOEXCEPT
{
    ++descriptor.cancels;
    std::vector<operation*> canceled{};
    {
        std::unique_lock lock{ descriptor.mutex };
        canceled.insert(canceled.end(), descriptor.reads.begin(),
            descriptor.reads.end());
        canceled.insert(canceled.end(), descriptor.writes.begin(),
            descriptor.writes.end());
        descriptor.reads.clear();
        descriptor.writes.clear();
    }

    for (const auto op: canceled)
    {
        op->ec = error::operation_canceled;
        op->bytes = zero;
        scheduler_.complete(to_completion(op));
    }
}

void backend::close(descriptor& descriptor) NOEXCEPT
{
    cancel(descriptor);
    {
        std::unique_lock lock{ mutex_ };
        descriptors_.erase(descriptor.key);
    }

    // Closing the socket removes it from the epoll or kqueue set.
    close_socket(descriptor.handle);
    descriptor.handle = invalid_socket;
    descriptor.bound = false;
}

// private
bool backend::add(const descriptor_ptr& descriptor, int& native) NOEXCEPT
{
    {
        std::unique_lock lock{ mutex_ };
        descriptor->key = next_key_++;
        descriptors_.emplace(descriptor->key, descriptor);
    }

#if defined(HAVE_APPLE)
    struct kevent events[2]{};
    const auto key = reinterpret_cast<void*>(descriptor->key);
    EV_SET(&events[0], descriptor->handle, EVFILT_READ, EV_ADD | EV_CLEAR, 0,
        0, key);
    EV_SET(&events[1], descriptor->handle, EVFILT_WRITE, EV_ADD | EV_CLEAR,
        0, 0, key);
    const auto result = ::kevent(handle_, events, 2, nullptr, 0, nullptr);
#else
    epoll_event event{};
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    event.data.u64 = descriptor->key;
    const auto result = ::epoll_ctl(handle_, EPOLL_CTL_ADD,
        descriptor->handle, &event);
#endif

    if (result < 0)
    {
        native = last_error();
        std::unique_lock lock{ mutex_ };
        descriptors_.erase(descriptor->key);
        return false;
    }

    native = 0;
    return true;
}

// Operations.
// ----------------------------------------------------------------------------

void backend::start(operation* op) NOEXCEPT
{
    scheduler_.work_started();
    op->cancels = op->owner->cancels.load();
    auto& owner = *op->owner;

    // Reads and writes of no bytes complete immediately.
    const auto transfer = op->type == operation::kind::read ||
        op->type == operation::kind::write;

    if (transfer && is_zero(op->requested))
    {
        set_result(op, error::success, zero);
        scheduler_.complete(to_completion(op));
        return;
    }

    // A connect starts now and completes when writable.
    if (op->type == operation::kind::connect)
    {
        if (::connect(owner.handle, op->target.data(), op->target.size) == 0)
        {
            set_result(op, error::success, zero);
            scheduler_.complete(to_completion(op));
            return;
        }

        const auto native = last_error();
        if (!is_in_progress(native))
        {
            set_result(op, to_code(native), zero);
            scheduler_.complete(to_completion(op));
            return;
        }
    }

    const auto read = op->type == operation::kind::read ||
        op->type == operation::kind::accept;

    // An operation that can complete now (with no queued predecessor) does.
    // Under the descriptor lock, an edge reported before the operation is
    // queued is observed here, and one reported after finds it queued.
    std::unique_lock lock{ owner.mutex };
    auto& queue = read ? owner.reads : owner.writes;
    if (queue.empty() && perform(op))
    {
        lock.unlock();
        scheduler_.complete(to_completion(op));
        return;
    }

    queue.push_back(op);
}

// private
// True if the operation is complete (its result set), false if it would block.
bool backend::perform(operation* op) NOEXCEPT
{
    auto& owner = *op->owner;
    while (true)
    {
        switch (op->type)
        {
            case operation::kind::read:
            {
                std::vector<iovec> buffers{};
                for (const auto& buffer: op->in)
                    buffers.push_back({ buffer.data(), buffer.size() });

                const auto result = ::readv(owner.handle, buffers.data(),
                    possible_narrow_sign_cast<int>(buffers.size()));

                if (result > 0)
                {
                    set_result(op, error::success,
                        possible_narrow_sign_cast<size_t>(result));
                    return true;
                }

                // A read of no bytes is the end of the stream.
                if (is_zero(result))
                {
                    set_result(op, error::peer_disconnect, zero);
                    return true;
                }

                break;
            }
            case operation::kind::write:
            {
                std::vector<iovec> buffers{};
                for (const auto& buffer: op->out)
                    buffers.push_back({ const_cast<void*>(buffer.data()),
                        buffer.size() });

                msghdr message{};
                message.msg_iov = buffers.data();
                message.msg_iovlen = buffers.size();
                const auto result = ::sendmsg(owner.handle, &message,
                    send_flags);

                if (result >= 0)
                {
                    set_result(op, error::success,
                        possible_narrow_sign_cast<size_t>(result));
                    return true;
                }

                break;
            }
            case operation::kind::accept:
            {
                const auto accepted = ::accept(owner.handle, nullptr, nullptr);
                if (accepted >= 0)
                {
                    int native{};
                    set_close_on_exec(accepted);
                    if (!set_blocking(accepted, false, native) ||
                        !adopt(op->peer, accepted, owner.v6, native))
                    {
                        close_socket(accepted);
                        set_result(op, to_code(native), zero);
                        return true;
                    }

                    set_result(op, error::success, zero);
                    return true;
                }

                // An aborted connection is skipped.
                if (last_error() == ECONNABORTED)
                    continue;

                break;
            }
            case operation::kind::connect:
            {
                // A connect in progress is not yet writable.
                pollfd writable{ owner.handle, POLLOUT, 0 };
                const auto polled = ::poll(&writable, 1, 0);
                if (polled < 0)
                    break;

                if (is_zero(polled))
                    return false;

                int result{};
                socklen_t size = sizeof(result);
                if (::getsockopt(owner.handle, SOL_SOCKET, SO_ERROR, &result,
                    &size) != 0)
                    result = last_error();

                set_result(op, to_code(result), zero);
                return true;
            }
        }

        const auto native = last_error();
        if (native == EINTR)
            continue;

        if (is_would_block(native))
            return false;

        set_result(op, to_code(native), zero);
        return true;
    }
}

// private
void backend::ready(const descriptor_ptr& descriptor, bool read, bool write,
    std::vector<handler>& out) NOEXCEPT
{
    std::unique_lock lock{ descriptor->mutex };
    const auto perform_queue = [&](std::deque<operation*>& queue) NOEXCEPT
    {
        while (!queue.empty() && perform(queue.front()))
        {
            out.push_back(to_completion(queue.front()));
            queue.pop_front();
        }
    };

    if (read)
        perform_queue(descriptor->reads);

    if (write)
        perform_queue(descriptor->writes);
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
