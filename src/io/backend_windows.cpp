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

#if defined(HAVE_WINDOWS)

#include <chrono>
#include <limits>
#include <mutex>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_REINTERPRET_CAST)

// The address space AcceptEx requires for each of the two addresses.
constexpr DWORD accept_address_size = sizeof(sockaddr_storage) + 16;

template <typename Function>
static bool load_extension(native_socket socket, GUID guid,
    Function& function) NOEXCEPT
{
    DWORD bytes{};
    return ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid,
        sizeof(guid), &function, sizeof(function), &bytes, nullptr,
        nullptr) != SOCKET_ERROR;
}

static DWORD to_milliseconds(const timeout& duration) NOEXCEPT
{
    if (!duration.has_value())
        return INFINITE;

    const auto count = std::chrono::ceil<std::chrono::milliseconds>(
        duration.value()).count();

    // INFINITE is the maximum, so the bound is one less.
    constexpr int64_t maximum = sub1(std::numeric_limits<DWORD>::max());
    return possible_narrow_sign_cast<DWORD>(std::clamp<int64_t>(count, 0,
        maximum));
}

backend::backend(detail::scheduler& scheduler) NOEXCEPT
  : scheduler_(scheduler)
{
    static std::once_flag startup{};
    std::call_once(startup, []() NOEXCEPT
    {
        WSADATA data{};
        ::WSAStartup(MAKEWORD(2, 2), &data);
    });

    port_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    int native{};
    const auto probe = create_socket(false, native);
    load_extension(probe, WSAID_ACCEPTEX, accept_ex_);
    load_extension(probe, WSAID_CONNECTEX, connect_ex_);
    close_socket(probe);
}

backend::~backend() NOEXCEPT
{
    if (port_ != nullptr)
        ::CloseHandle(port_);
}

// Completion.
// ----------------------------------------------------------------------------

void backend::wait(const timeout& duration,
    std::vector<handler>& out) NOEXCEPT
{
    DWORD bytes{};
    ULONG_PTR key{};
    LPOVERLAPPED overlapped{};
    const auto success = ::GetQueuedCompletionStatus(port_, &bytes, &key,
        &overlapped, to_milliseconds(duration));

    // Timeout or interruption (posted without overlapped).
    if (overlapped == nullptr)
        return;

    const auto op = reinterpret_cast<operation*>(overlapped);
    const auto native = success ? 0 : static_cast<int>(::GetLastError());

    switch (op->type)
    {
        case operation::kind::read:
        {
            // A completed read of no bytes is the end of the stream.
            const auto end = is_zero(native) && is_zero(bytes) &&
                is_nonzero(op->requested);

            set_result(op, end ? code{ error::peer_disconnect } :
                to_code(native), bytes);
            break;
        }
        case operation::kind::write:
            set_result(op, to_code(native), bytes);
            break;
        case operation::kind::accept:
            set_result(op, to_code(native), zero);
            finish_accept(op);
            break;
        case operation::kind::connect:
            set_result(op, to_code(native), zero);
            finish_connect(op);
            break;
    }

    out.push_back(to_completion(op));
}

void backend::interrupt() NOEXCEPT
{
    ::PostQueuedCompletionStatus(port_, 0, 0, nullptr);
}

// Descriptors.
// ----------------------------------------------------------------------------

bool backend::open(const descriptor_ptr& descriptor, bool v6,
    int& native) NOEXCEPT
{
    const auto socket = create_socket(v6, native);
    if (socket == invalid_socket)
        return false;

    if (!associate(socket, native))
    {
        close_socket(socket);
        return false;
    }

    descriptor->handle = socket;
    descriptor->v6 = v6;
    descriptor->bound = false;
    return true;
}

bool backend::adopt(const descriptor_ptr& descriptor, native_socket socket,
    bool v6, int& native) NOEXCEPT
{
    if (!associate(socket, native))
        return false;

    descriptor->handle = socket;
    descriptor->v6 = v6;
    descriptor->bound = true;
    return true;
}

void backend::cancel(descriptor& descriptor) NOEXCEPT
{
    ++descriptor.cancels;
    if (descriptor.handle != invalid_socket)
        ::CancelIoEx(reinterpret_cast<HANDLE>(descriptor.handle), nullptr);
}

void backend::close(descriptor& descriptor) NOEXCEPT
{
    cancel(descriptor);
    close_socket(descriptor.handle);
    descriptor.handle = invalid_socket;
    descriptor.bound = false;
}

// private
bool backend::associate(native_socket socket, int& native) NOEXCEPT
{
    if (::CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), port_, 0,
        0) == nullptr)
    {
        native = static_cast<int>(::GetLastError());
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

    switch (op->type)
    {
        case operation::kind::read:
            start_read(op);
            break;
        case operation::kind::write:
            start_write(op);
            break;
        case operation::kind::accept:
            start_accept(op);
            break;
        case operation::kind::connect:
            start_connect(op);
            break;
    }
}

// private
// An operation that fails to start completes without a completion packet.
void backend::fail(operation* op, int native) NOEXCEPT
{
    set_result(op, to_code(native), zero);
    scheduler_.complete(to_completion(op));
}

// private
// A read of no bytes completes immediately.
void backend::start_read(operation* op) NOEXCEPT
{
    if (is_zero(op->requested))
    {
        set_result(op, error::success, zero);
        scheduler_.complete(to_completion(op));
        return;
    }

    DWORD flags{};
    if (::WSARecv(op->owner->handle, op->buffers.data(),
        possible_narrow_cast<DWORD>(op->buffers.size()), nullptr, &flags,
        &op->overlapped, nullptr) == SOCKET_ERROR)
    {
        const auto native = last_error();
        if (native != WSA_IO_PENDING)
            fail(op, native);
    }
}

// private
void backend::start_write(operation* op) NOEXCEPT
{
    if (is_zero(op->requested))
    {
        set_result(op, error::success, zero);
        scheduler_.complete(to_completion(op));
        return;
    }

    if (::WSASend(op->owner->handle, op->buffers.data(),
        possible_narrow_cast<DWORD>(op->buffers.size()), nullptr, 0,
        &op->overlapped, nullptr) == SOCKET_ERROR)
    {
        const auto native = last_error();
        if (native != WSA_IO_PENDING)
            fail(op, native);
    }
}

// private
void backend::start_accept(operation* op) NOEXCEPT
{
    int native{};
    op->accepted = create_socket(op->owner->v6, native);
    if (op->accepted == invalid_socket)
    {
        fail(op, native);
        return;
    }

    op->addresses.resize(2u * accept_address_size);
    DWORD bytes{};
    if (accept_ex_(op->owner->handle, op->accepted, op->addresses.data(), 0,
        accept_address_size, accept_address_size, &bytes,
        &op->overlapped) == FALSE)
    {
        native = last_error();
        if (native != ERROR_IO_PENDING)
        {
            close_socket(op->accepted);
            op->accepted = invalid_socket;
            fail(op, native);
        }
    }
}

// private
void backend::finish_accept(operation* op) NOEXCEPT
{
    int native{};
    if (!op->ec)
    {
        auto listener = op->owner->handle;
        if (::setsockopt(op->accepted, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
            pointer_cast<const char>(&listener), sizeof(listener)) ==
                SOCKET_ERROR)
            native = last_error();
        else if (adopt(op->peer, op->accepted, op->owner->v6, native))
            return;

        op->ec = to_code(native);
    }

    close_socket(op->accepted);
    op->accepted = invalid_socket;
}

// private
// ConnectEx requires a bound socket, bound here to any address if unbound.
void backend::start_connect(operation* op) NOEXCEPT
{
    auto& owner = *op->owner;
    if (!owner.bound)
    {
        socket_address any{};
        any.storage.ss_family = owner.v6 ? AF_INET6 : AF_INET;
        any.size = owner.v6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
        if (::bind(owner.handle, any.data(), any.size) == SOCKET_ERROR)
        {
            fail(op, last_error());
            return;
        }

        owner.bound = true;
    }

    if (connect_ex_(owner.handle, op->target.data(), op->target.size,
        nullptr, 0, nullptr, &op->overlapped) == FALSE)
    {
        const auto native = last_error();
        if (native != ERROR_IO_PENDING)
            fail(op, native);
    }
}

// private
void backend::finish_connect(operation* op) NOEXCEPT
{
    if (!op->ec && ::setsockopt(op->owner->handle, SOL_SOCKET,
        SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) == SOCKET_ERROR)
        op->ec = to_code(last_error());
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
