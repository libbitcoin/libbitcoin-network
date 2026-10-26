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
#include <bitcoin/network/io/socket.hpp>

#include <atomic>
#include <bitcoin/network/define.hpp>
#include "backend.hpp"
#include "native.hpp"

namespace libbitcoin {
namespace network {
namespace io {

using namespace system;
using namespace detail;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_REINTERPRET_CAST)

#if defined(HAVE_WINDOWS)
    constexpr int shutdown_receive = SD_RECEIVE;
    constexpr int shutdown_send = SD_SEND;
    constexpr int shutdown_both = SD_BOTH;
    constexpr int send_flags = 0;
#else
    constexpr int shutdown_receive = SHUT_RD;
    constexpr int shutdown_send = SHUT_WR;
    constexpr int shutdown_both = SHUT_RDWR;
#if defined(MSG_NOSIGNAL)
    constexpr int send_flags = MSG_NOSIGNAL;
#else
    constexpr int send_flags = 0;
#endif
#endif

static descriptor_ptr make_descriptor(const executor& executor) NOEXCEPT
{
    return std::make_shared<descriptor>(executor.context().scheduler());
}

static detail::backend& backend_of(const executor& executor) NOEXCEPT
{
    return executor.context().scheduler().backend();
}

static code native_code() NOEXCEPT
{
    return to_code(last_error());
}

template <typename Option>
static code set_socket_option(native_socket socket, int level, int name,
    Option value) NOEXCEPT
{
    if (::setsockopt(socket, level, name, pointer_cast<const char>(&value),
        sizeof(value)) != 0)
        return native_code();

    return error::success;
}

// Deliver the result on the executor (never within the initiating call).
static void post_result(const executor& executor,
    const socket::io_handler& handler, const code& ec) NOEXCEPT
{
    executor.post([handler, ec]() NOEXCEPT
    {
        handler(ec, zero);
    });
}

template <typename Buffers>
static size_t total_size(const Buffers& buffers) NOEXCEPT
{
    size_t total{};
    for (const auto& buffer: buffers)
        total = ceilinged_add(total, buffer.size());

    return total;
}

#if defined(HAVE_WINDOWS)
template <typename Buffers>
static std::vector<WSABUF> to_native(const Buffers& buffers) NOEXCEPT
{
    std::vector<WSABUF> out{};
    out.reserve(buffers.size());
    for (const auto& buffer: buffers)
        out.push_back(
        {
            possible_narrow_cast<ULONG>(buffer.size()),
            pointer_cast<CHAR>(const_cast<void*>(buffer.data()))
        });

    return out;
}
#else
template <typename Buffers>
static std::vector<iovec> to_native(const Buffers& buffers) NOEXCEPT
{
    std::vector<iovec> out{};
    out.reserve(buffers.size());
    for (const auto& buffer: buffers)
        out.push_back(
        {
            const_cast<void*>(static_cast<const void*>(buffer.data())),
            buffer.size()
        });

    return out;
}
#endif

// tcp
// ----------------------------------------------------------------------------

tcp::tcp(bool v6) NOEXCEPT
  : v6_(v6)
{
}

tcp tcp::v4() NOEXCEPT
{
    return tcp{ false };
}

tcp tcp::v6() NOEXCEPT
{
    return tcp{ true };
}

tcp tcp::of(const endpoint& point) NOEXCEPT
{
    return tcp{ point.address().is_v6() };
}

bool tcp::is_v6() const NOEXCEPT
{
    return v6_;
}

// socket
// ----------------------------------------------------------------------------

socket::socket(const executor& executor) NOEXCEPT
  : executor_(executor), descriptor_(make_descriptor(executor))
{
}

socket::socket(socket&& other) NOEXCEPT
  : executor_(other.executor_), descriptor_(std::move(other.descriptor_))
{
    other.descriptor_ = make_descriptor(other.executor_);
}

socket& socket::operator=(socket&& other) NOEXCEPT
{
    if (this != &other)
    {
        code ignore{};
        close(ignore);
        executor_ = other.executor_;
        descriptor_ = std::move(other.descriptor_);
        other.descriptor_ = make_descriptor(other.executor_);
    }

    return *this;
}

socket::~socket() NOEXCEPT
{
    code ignore{};
    close(ignore);
}

socket::executor_type socket::get_executor() const NOEXCEPT
{
    return executor_;
}

bool socket::is_open() const NOEXCEPT
{
    return descriptor_->handle != invalid_socket;
}

socket::native_handle_type socket::native_handle() const NOEXCEPT
{
    return static_cast<native_handle_type>(descriptor_->handle);
}

void socket::open(const tcp& protocol, code& ec) NOEXCEPT
{
    if (is_open())
    {
        ec = error::connect_failed;
        return;
    }

    int native{};
    backend_of(executor_).open(descriptor_, protocol.is_v6(), native);
    ec = to_code(native);
}

void socket::bind(const endpoint& point, code& ec) NOEXCEPT
{
    const auto address = to_socket_address(point);
    if (!is_zero(::bind(descriptor_->handle, address.data(), address.size)))
    {
        ec = native_code();
        return;
    }

    descriptor_->bound = true;
    ec = error::success;
}

void socket::close(code& ec) NOEXCEPT
{
    if (is_open())
        backend_of(executor_).close(*descriptor_);

    ec = error::success;
}

void socket::close() NOEXCEPT
{
    code ignore{};
    close(ignore);
}

void socket::cancel(code& ec) NOEXCEPT
{
    if (!is_open())
    {
        ec = error::bad_stream;
        return;
    }

    backend_of(executor_).cancel(*descriptor_);
    ec = error::success;
}

void socket::shutdown(shutdown_type what, code& ec) NOEXCEPT
{
    const auto how = what == shutdown_type::receive ? shutdown_receive :
        (what == shutdown_type::send ? shutdown_send : shutdown_both);

    ec = !is_zero(::shutdown(descriptor_->handle, how)) ? native_code() :
        code{ error::success };
}

void socket::set_option(const no_delay& option, code& ec) NOEXCEPT
{
    ec = set_socket_option(descriptor_->handle, IPPROTO_TCP, TCP_NODELAY,
        to_int(option.value));
}

void socket::set_option(const keep_alive& option, code& ec) NOEXCEPT
{
    ec = set_socket_option(descriptor_->handle, SOL_SOCKET, SO_KEEPALIVE,
        to_int(option.value));
}

void socket::non_blocking(bool value, code& ec) NOEXCEPT
{
    // Posix sockets are always nonblocking (blocking calls wait on poll).
#if defined(HAVE_WINDOWS)
    int native{};
    if (!set_blocking(descriptor_->handle, !value, native))
    {
        ec = to_code(native);
        return;
    }
#endif

    descriptor_->non_blocking = value;
    ec = error::success;
}

bool socket::non_blocking() const NOEXCEPT
{
    return descriptor_->non_blocking;
}

size_t socket::available(code& ec) const NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    u_long count{};
    if (!is_zero(::ioctlsocket(descriptor_->handle, FIONREAD, &count)))
#else
    int count{};
    if (!is_zero(::ioctl(descriptor_->handle, FIONREAD, &count)))
#endif
    {
        ec = native_code();
        return zero;
    }

    ec = error::success;
#if defined(HAVE_WINDOWS)
    return possible_wide_cast<size_t>(count);
#else
    return possible_wide_cast<size_t>(sign_cast<unsigned int>(count));
#endif
}

endpoint socket::local_endpoint(code& ec) const NOEXCEPT
{
    socket_address address{};
    address.size = sizeof(address.storage);
    const auto handle = descriptor_->handle;
    if (!is_zero(::getsockname(handle, address.data(), &address.size)))
    {
        ec = native_code();
        return {};
    }

    ec = error::success;
    return to_endpoint(address.data());
}

endpoint socket::remote_endpoint(code& ec) const NOEXCEPT
{
    socket_address address{};
    address.size = sizeof(address.storage);
    const auto handle = descriptor_->handle;
    if (!is_zero(::getpeername(handle, address.data(), &address.size)))
    {
        ec = native_code();
        return {};
    }

    ec = error::success;
    return to_endpoint(address.data());
}

endpoint socket::local_endpoint() const NOEXCEPT
{
    code ignore{};
    return local_endpoint(ignore);
}

endpoint socket::remote_endpoint() const NOEXCEPT
{
    code ignore{};
    return remote_endpoint(ignore);
}

bool socket::half_closed() const NOEXCEPT
{
    if (!is_open())
        return false;

#if defined(HAVE_WINDOWS)
    TCP_INFO_v0 info{};
    DWORD version{};
    DWORD size{};
    if (is_zero(::WSAIoctl(descriptor_->handle, SIO_TCP_INFO, &version,
        sizeof(version), &info, sizeof(info), &size, nullptr, nullptr)))
        return info.State == TCPSTATE_CLOSE_WAIT;

#elif defined(HAVE_APPLE)
    struct tcp_connection_info info{};
    socklen_t size = sizeof(info);
    if (is_zero(::getsockopt(descriptor_->handle, IPPROTO_TCP,
        TCP_CONNECTION_INFO, &info, &size)))
        return info.tcpi_state == TCPS_CLOSE_WAIT;

#elif defined(HAVE_LINUX)
    struct tcp_info info{};
    socklen_t size = sizeof(info);
    if (is_zero(::getsockopt(descriptor_->handle, IPPROTO_TCP, TCP_INFO,
        &info, &size)))
        return info.tcpi_state == TCP_CLOSE_WAIT ||
            (info.tcpi_state == TCP_CLOSE && !is_zero(info.tcpi_rcv_mss));

#elif defined(HAVE_BSD)
    struct tcp_info info{};
    socklen_t size = sizeof(info);
    if (is_zero(::getsockopt(descriptor_->handle, IPPROTO_TCP, TCP_INFO,
        &info, &size)))
        return info.tcpi_state == TCPS_CLOSE_WAIT;
#endif
    return false;
}

void socket::async_connect(const endpoint& point,
    result_handler&& handler) NOEXCEPT
{
    io_handler completion =
        [handler = std::move(handler)](const code& ec, size_t) NOEXCEPT
        {
            handler(ec);
        };

    if (!is_open())
    {
        code ec{};
        open(tcp::of(point), ec);
        if (ec)
        {
            post_result(executor_, completion, ec);
            return;
        }
    }

    constexpr auto kind = operation::kind::connect;
    const auto op = new operation{ kind, descriptor_, executor_ };
    op->target = to_socket_address(point);
    op->handler = std::move(completion);
    backend_of(executor_).start(op);
}

void socket::do_read(mutable_buffers&& buffers, io_handler&& handler) NOEXCEPT
{
    if (!is_open())
    {
        post_result(executor_, handler, error::bad_stream);
        return;
    }

    constexpr auto kind = operation::kind::read;
    const auto op = new operation{ kind, descriptor_, executor_ };
    op->handler = std::move(handler);
    op->requested = total_size(buffers);
#if defined(HAVE_WINDOWS)
    op->buffers = to_native(buffers);
#else
    op->in = std::move(buffers);
#endif
    backend_of(executor_).start(op);
}

void socket::do_write(const_buffers&& buffers, io_handler&& handler) NOEXCEPT
{
    if (!is_open())
    {
        post_result(executor_, handler, error::bad_stream);
        return;
    }

    constexpr auto kind = operation::kind::write;
    const auto op = new operation{ kind, descriptor_, executor_ };
    op->handler = std::move(handler);
    op->requested = total_size(buffers);
#if defined(HAVE_WINDOWS)
    op->buffers = to_native(buffers);
#else
    op->out = std::move(buffers);
#endif
    backend_of(executor_).start(op);
}

// Synchronous.
// ----------------------------------------------------------------------------

void socket::connect(const endpoint& point, code& ec) NOEXCEPT
{
    if (!is_open())
    {
        open(tcp::of(point), ec);
        if (ec)
            return;
    }

    const auto address = to_socket_address(point);
    const auto handle = descriptor_->handle;
    if (is_zero(::connect(handle, address.data(), address.size)))
    {
        ec = error::success;
        return;
    }

    auto native = last_error();
    if (!is_in_progress(native) || descriptor_->non_blocking)
    {
        ec = to_code(native);
        return;
    }

    if (!wait_ready(handle, true, native))
    {
        ec = to_code(native);
        return;
    }

    int result{};
    socklen_t size = sizeof(result);
    const auto value = pointer_cast<char>(&result);
    if (!is_zero(::getsockopt(handle, SOL_SOCKET, SO_ERROR, value, &size)))
        result = last_error();

    ec = to_code(result);
}

size_t socket::do_read_some(const mutable_buffers& buffers, code& ec) NOEXCEPT
{
    const auto requested = total_size(buffers);
    if (is_zero(requested))
    {
        ec = error::success;
        return zero;
    }

    auto native_buffers = to_native(buffers);
    while (true)
    {
#if defined(HAVE_WINDOWS)
        DWORD bytes{};
        DWORD flags{};
        const auto result = ::WSARecv(descriptor_->handle,
            native_buffers.data(),
            possible_narrow_cast<DWORD>(native_buffers.size()),
            &bytes, &flags, nullptr, nullptr);

        const auto failed = result == SOCKET_ERROR;
        const size_t count = bytes;
#else
        const auto result = ::readv(descriptor_->handle, native_buffers.data(),
            possible_narrow_sign_cast<int>(native_buffers.size()));

        const auto failed = is_negative(result);
        const auto count = failed ? zero :
            possible_narrow_sign_cast<size_t>(result);
#endif
        if (!failed)
        {
            ec = is_zero(count) ? error::peer_disconnect : error::success;
            return count;
        }

        auto native = last_error();
#if !defined(HAVE_WINDOWS)
        if (native == EINTR)
            continue;
#endif
        if (is_would_block(native) && !descriptor_->non_blocking &&
            wait_ready(descriptor_->handle, false, native))
            continue;

        ec = to_code(native);
        return zero;
    }
}

size_t socket::do_write_some(const const_buffers& buffers, code& ec) NOEXCEPT
{
    const auto requested = total_size(buffers);
    if (is_zero(requested))
    {
        ec = error::success;
        return zero;
    }

    const auto handle = descriptor_->handle;
    auto native_buffers = to_native(buffers);
    while (true)
    {
#if defined(HAVE_WINDOWS)
        DWORD bytes{};
        const auto result = ::WSASend(handle,
            native_buffers.data(),
            possible_narrow_cast<DWORD>(native_buffers.size()), &bytes,
            send_flags, nullptr, nullptr);

        const auto failed = result == SOCKET_ERROR;
        const size_t count = bytes;
#else
        msghdr message{};
        message.msg_iov = native_buffers.data();
        message.msg_iovlen = native_buffers.size();
        const auto result = ::sendmsg(handle, &message, send_flags);

        const auto failed = is_negative(result);
        const auto count = failed ? zero :
            possible_narrow_sign_cast<size_t>(result);
#endif
        if (!failed)
        {
            ec = error::success;
            return count;
        }

        auto native = last_error();
#if !defined(HAVE_WINDOWS)
        if (native == EINTR)
            continue;
#endif
        if (is_would_block(native) && !descriptor_->non_blocking &&
            wait_ready(descriptor_->handle, true, native))
            continue;

        ec = to_code(native);
        return zero;
    }
}

// acceptor
// ----------------------------------------------------------------------------

acceptor::acceptor(const executor& executor) NOEXCEPT
  : executor_(executor), descriptor_(make_descriptor(executor))
{
}

acceptor::acceptor(const executor& executor, const endpoint& point) NOEXCEPT
  : acceptor(executor)
{
    code ec{};
    open(tcp::of(point), ec);
    if (!ec) set_option(reuse_address{ true }, ec);
    if (!ec) bind(point, ec);
    if (!ec) listen(ec);
    if (ec) close(ec);
}

acceptor::~acceptor() NOEXCEPT
{
    code ignore{};
    close(ignore);
}

acceptor::executor_type acceptor::get_executor() const NOEXCEPT
{
    return executor_;
}

bool acceptor::is_open() const NOEXCEPT
{
    return descriptor_->handle != invalid_socket;
}

acceptor::native_handle_type acceptor::native_handle() const NOEXCEPT
{
    return static_cast<native_handle_type>(descriptor_->handle);
}

void acceptor::open(const tcp& protocol, code& ec) NOEXCEPT
{
    if (is_open())
    {
        ec = error::connect_failed;
        return;
    }

    int native{};
    backend_of(executor_).open(descriptor_, protocol.is_v6(), native);
    ec = to_code(native);
}

void acceptor::set_option(const reuse_address& option, code& ec) NOEXCEPT
{
    ec = set_socket_option(descriptor_->handle, SOL_SOCKET, SO_REUSEADDR,
        to_int(option.value));
}

void acceptor::set_option(const v6_only& option, code& ec) NOEXCEPT
{
    ec = set_socket_option(descriptor_->handle, IPPROTO_IPV6, IPV6_V6ONLY,
        to_int(option.value));
}

void acceptor::bind(const endpoint& point, code& ec) NOEXCEPT
{
    const auto address = to_socket_address(point);
    if (!is_zero(::bind(descriptor_->handle, address.data(), address.size)))
    {
        ec = native_code();
        return;
    }

    descriptor_->bound = true;
    ec = error::success;
}

void acceptor::listen(code& ec) NOEXCEPT
{
    ec = !is_zero(::listen(descriptor_->handle, SOMAXCONN)) ? native_code() :
        code{ error::success };
}

void acceptor::close(code& ec) NOEXCEPT
{
    if (is_open())
        backend_of(executor_).close(*descriptor_);

    ec = error::success;
}

void acceptor::close() NOEXCEPT
{
    code ignore{};
    close(ignore);
}

void acceptor::cancel(code& ec) NOEXCEPT
{
    if (!is_open())
    {
        ec = error::bad_stream;
        return;
    }

    backend_of(executor_).cancel(*descriptor_);
    ec = error::success;
}

endpoint acceptor::local_endpoint(code& ec) const NOEXCEPT
{
    socket_address address{};
    address.size = sizeof(address.storage);
    const auto handle = descriptor_->handle;
    if (!is_zero(::getsockname(handle, address.data(), &address.size)))
    {
        ec = native_code();
        return {};
    }

    ec = error::success;
    return to_endpoint(address.data());
}

endpoint acceptor::local_endpoint() const NOEXCEPT
{
    code ignore{};
    return local_endpoint(ignore);
}

void acceptor::async_accept(socket& peer, result_handler&& handler) NOEXCEPT
{
    socket::io_handler completion =
        [handler = std::move(handler)](const code& ec, size_t) NOEXCEPT
        {
            handler(ec);
        };

    if (!is_open() || peer.is_open())
    {
        const auto ec = is_open() ? error::connect_failed : error::bad_stream;
        post_result(executor_, completion, ec);
        return;
    }

    constexpr auto kind = operation::kind::accept;
    const auto op = new operation{ kind, descriptor_, executor_ };
    op->peer = peer.descriptor_;
    op->handler = std::move(completion);
    backend_of(executor_).start(op);
}

void acceptor::accept(socket& peer, code& ec) NOEXCEPT
{
    if (!is_open() || peer.is_open())
    {
        ec = is_open() ? error::connect_failed : error::bad_stream;
        return;
    }

    while (true)
    {
        const auto accepted = ::accept(descriptor_->handle, nullptr, nullptr);
        if (accepted != invalid_socket)
        {
            int native{};
#if !defined(HAVE_WINDOWS)
            ::fcntl(accepted, F_SETFD, FD_CLOEXEC);
            if (!set_blocking(accepted, false, native))
            {
                close_socket(accepted);
                ec = to_code(native);
                return;
            }
#endif
            if (!backend_of(executor_).adopt(peer.descriptor_, accepted,
                descriptor_->v6, native))
            {
                close_socket(accepted);
                ec = to_code(native);
                return;
            }

            ec = error::success;
            return;
        }

        auto native = last_error();
#if !defined(HAVE_WINDOWS)
        if (native == EINTR || native == ECONNABORTED)
            continue;
#endif
        if (is_would_block(native) && !descriptor_->non_blocking &&
            wait_ready(descriptor_->handle, false, native))
            continue;

        ec = to_code(native);
        return;
    }
}

// resolver
// ----------------------------------------------------------------------------

struct resolver::state
{
    std::atomic<size_t> cancels{};
};

static code resolve_code(int result) NOEXCEPT
{
    switch (result)
    {
        case 0:
            return error::success;
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
        case EAI_NODATA:
#endif
        case EAI_FAIL:
            return error::operation_failed;
        default:
            return error::resolve_failed;
    }
}

resolver::resolver(const executor& executor) NOEXCEPT
  : executor_(executor), state_(std::make_shared<state>())
{
}

resolver::~resolver() NOEXCEPT
{
    cancel();
}

resolver::executor_type resolver::get_executor() const NOEXCEPT
{
    return executor_;
}

void resolver::async_resolve(const std::string& host,
    const std::string& service, resolve_handler&& handler) NOEXCEPT
{
    auto& scheduler = executor_.context().scheduler();
    scheduler.work_started();

    const auto started = state_->cancels.load();
    scheduler.resolve([&scheduler, state = state_, started,
        executor = executor_, host, service,
        handler = std::move(handler)]() NOEXCEPT
    {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_ADDRCONFIG;

        addrinfo* results{};
        const auto result = ::getaddrinfo(host.c_str(), service.c_str(),
            &hints, &results);

        endpoints range{};
        for (auto it = results; it != nullptr; it = it->ai_next)
            if (it->ai_family == AF_INET || it->ai_family == AF_INET6)
                range.push_back(to_endpoint(it->ai_addr));

        if (results != nullptr)
            ::freeaddrinfo(results);

        scheduler.complete([state, started, executor, handler,
            ec = resolve_code(result), range = std::move(range)]() NOEXCEPT
        {
            const auto canceled = state->cancels.load() != started;
            const code value = canceled ? error::operation_canceled : ec;
            executor.dispatch([handler, value, range]() NOEXCEPT
            {
                handler(value, value ? endpoints{} : range);
            });
        });
    });
}

void resolver::cancel() NOEXCEPT
{
    ++state_->cancels;
}

// async_connect (range)
// ----------------------------------------------------------------------------

namespace {

struct range_connect
  : std::enable_shared_from_this<range_connect>
{
    range_connect(io::socket& socket, const endpoints& range,
        connect_handler&& handler) NOEXCEPT
      : socket(socket), range(range), handler(std::move(handler))
    {
    }

    void next() NOEXCEPT
    {
        if (index == range.size())
        {
            const auto ec = last ? last : code{ error::resolve_failed };
            socket.get_executor().post(
                [self = shared_from_this(), ec]() NOEXCEPT
                {
                    self->handler(ec, {});
                });

            return;
        }

        code ignore{};
        socket.close(ignore);
        socket.async_connect(range.at(index),
            [self = shared_from_this()](const code& ec) NOEXCEPT
            {
                self->handle(ec);
            });
    }

    void handle(const code& ec) NOEXCEPT
    {
        if (!ec || ec == error::operation_canceled)
        {
            handler(ec, ec ? endpoint{} : range.at(index));
            return;
        }

        if (!socket.is_open())
        {
            handler(error::operation_canceled, {});
            return;
        }

        last = ec;
        ++index;
        next();
    }

    io::socket& socket;
    const endpoints range;
    const connect_handler handler;
    size_t index{};
    code last{};
};

} // namespace

void async_connect(io::socket& socket, const endpoints& range,
    connect_handler&& handler) NOEXCEPT
{
    std::make_shared<range_connect>(socket, range,
        std::move(handler))->next();
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace io
} // namespace network
} // namespace libbitcoin
