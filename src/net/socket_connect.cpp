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
#include <bitcoin/network/net/socket.hpp>

#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Accept.
// ----------------------------------------------------------------------------
// Closure of the acceptor, not the socket, releases the async_accept handler.
// The socket is not guarded during async_accept. This is required so the
// acceptor may be guarded from its own strand while preserving hiding of
// socket internals. This makes concurrent calls unsafe, however only the
// acceptor (a socket factory) requires access to the socket at this time.
// network::acceptor both invokes this call in the network strand and
// initializes the io::acceptor with the network strand. So the call to
// acceptor.async_accept invokes its handler on that strand as well.

void socket::accept(io::acceptor& acceptor,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(!get_base().is_open(), "accept on open socket");
    // Dispatches on the acceptor's strand (which should be network).
    acceptor.async_accept(get_base(),
        std::bind(&socket::handle_accept,
            shared_from_this(), _1, handler));
}

void socket::handle_accept(code ec,
    const result_handler& handler) NOEXCEPT
{
    // This is running in the acceptor (not socket) execution context.
    // socket_ and endpoint_ are not guarded here, see comments on accept.

    if (ec == error::operation_canceled)
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec)
    {
        if (ec == error::unknown) logx("accept", ec);
        handler(ec);
        return;
    }

    endpoint_ = { get_base().remote_endpoint(ec) };
    address_ = endpoint_;

    if (ec)
    {
        if (ec == error::unknown) logx("remote", ec);
        handler(ec);
        return;
    }

    code ignore{};
    binding_ = { get_base().local_endpoint(ignore) };

    // Defer handshake to the acceptor when connection is proxied.
    if (proxied_)
    {
        handler(error::success);
        return;
    }

    // Not in socket strand.
    do_handshake(handler);
}

// Connect.
// ----------------------------------------------------------------------------

void socket::connect(const io::endpoints& range,
    result_handler&& handler) NOEXCEPT
{
    io::post(strand_,
        std::bind(&socket::do_connect,
            shared_from_this(), range, std::move(handler)));
}

void socket::do_connect(const io::endpoints& range,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!websocket(), "socket is upgraded");
    BC_ASSERT_MSG(!get_base().is_open(), "connect on open socket");

    if (!bind_.address().is_unspecified())
    {
        do_connect_bound(range, zero, handler);
        return;
    }

    // Establishes a socket connection by trying each endpoint in sequence.
    io::async_connect(get_base(), range,
        std::bind(&socket::handle_connect,
            shared_from_this(), _1, _2, handler));
}

// The range connect closes the socket before each attempt, discarding the
// binding, so bound endpoints (of the bound family) are attempted singly.
void socket::do_connect_bound(const io::endpoints& range, size_t index,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    index = next_bound(range, index);
    if (index == range.size())
    {
        handler(error::resolve_failed);
        return;
    }

    const auto peer = *std::next(range.begin(), index);
    auto& base = get_base();
    code ec{};
    base.close(ec);
    base.open(io::tcp::of(peer), ec);

    if (!ec)
        base.bind(bind_, ec);

    if (ec)
    {
        if (ec == error::unknown) logx("bind", ec);
        handler(ec);
        return;
    }

    base.async_connect(peer,
        std::bind(&socket::handle_connect_bound,
            shared_from_this(), _1, range, add1(index), peer, handler));
}

void socket::handle_connect_bound(const code& ec,
    const io::endpoints& range, size_t next, const io::endpoint& peer,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec && ec != error::operation_canceled && !stopped_.load() &&
        next_bound(range, next) != range.size())
    {
        do_connect_bound(range, next, handler);
        return;
    }

    handle_connect(ec, peer, handler);
}

// private
size_t socket::next_bound(const io::endpoints& range,
    size_t index) const NOEXCEPT
{
    const auto v4 = bind_.address().is_v4();
    for (auto it = std::next(range.begin(), index); it != range.end();
        ++it, ++index)
        if (it->address().is_v4() == v4)
            break;

    return index;
}

void socket::handle_connect(const code& ec, const io::endpoint& peer,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // The endpoint is the intended target, the address is the resolved peer.
    // An address dial retains its metadata, a proxied peer is the local
    // binding of the proxy server, and is set by its handshake.
    if (!proxied_ && !address_)
        address_ = { peer };

    if (ec == error::operation_canceled)
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec)
    {
        if (ec == error::unknown) logx("connect", ec);
        handler(ec);
        return;
    }

    code ignore{};
    binding_ = { get_base().local_endpoint(ignore) };

    // Defer handshake to the connector when connection is proxied.
    if (proxied_)
    {
        handler(error::success);
        return;
    }

    // In socket strand.
    do_handshake(handler);
}

// Handshake (accept & connect).
// ----------------------------------------------------------------------------

void socket::handshake(result_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_handshake,
            shared_from_this(), std::move(handler)));
}

// Invokes handler on acceptor (network) or connector (socket) strand.
void socket::do_handshake(const result_handler& handler) NOEXCEPT
{
    ////BC_ASSERT(stranded());

    if (!inbound_ || is_zero(timer_->timeout().count()))
    {
        do_upgrade(handler);
        return;
    }

    timer_->start(std::bind(&socket::handle_handshake_timer,
        shared_from_this(), _1));

    do_upgrade(std::bind(&socket::handle_handshaked,
        shared_from_this(), _1, handler));
}

// private
void socket::handle_handshaked(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    timer_->stop();
    handler(ec);
}

// private
// Socket stop cancels the pending handshake, which completes the handler.
void socket::handle_handshake_timer(const code& ec) NOEXCEPT
{
    if (!ec)
        stop();
}

// private
void socket::do_upgrade(const result_handler& handler) NOEXCEPT
{
    if (std::holds_alternative<cref<p2ps::context>>(context_))
    {
        // The accepted peer is detected as v1 or v2 before upgrade.
        if (inbound_)
        {
            io::async_read(get_base(),
                detection_.prepare(p2ps::stream::detection_size),
                std::bind(&socket::handle_detection,
                    shared_from_this(), _1, handler));
            return;
        }

        // Extract to temporary to avoid dangling reference after destruction.
        auto socket = std::move(get_base());

        // P2PS (bip324) context is applied to the socket.
        socket_.emplace<p2ps::stream>(std::move(socket),
            std::get<cref<p2ps::context>>(context_).get());

        // Posts handler to socket strand.
        get_p2ps().async_handshake(
            std::bind(&socket::handle_encrypted_handshake,
                shared_from_this(), _1, handler));
        return;
    }

    if (std::holds_alternative<cref<zmtp::context>>(context_))
    {
        // Extract to temporary to avoid dangling reference after destruction.
        auto socket = std::move(get_base());

        // ZMTP (native) context is applied to the socket.
        auto& stream = socket_.emplace<zmtp::stream>(std::move(socket),
            std::get<cref<zmtp::context>>(context_).get(), role_);

        // Posts handler to socket strand.
        stream.async_handshake(inbound_,
            std::bind(&socket::handle_publisher_handshake,
                shared_from_this(), _1, handler));
        return;
    }

    if (std::holds_alternative<cref<tls::context>>(context_))
    {
        // The client role is not implemented.
        if (!inbound_)
        {
            handler(error::tls_alert_handshake_failure);
            return;
        }

        // Extract to temporary to avoid dangling reference after destruction.
        auto socket = std::move(get_base());

        // TLS context is applied to the socket.
        auto& stream = socket_.emplace<tls::stream>(std::move(socket),
            std::get<cref<tls::context>>(context_).get());

        // Posts handler to socket strand.
        stream.async_handshake(
            std::bind(&socket::handle_handshake,
                shared_from_this(), _1, handler));
        return;
    }

    handler(error::success);
}

// Invoked on the socket strand.
void socket::handle_detection(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        if (ec == error::unknown) logx("detection", ec);
        handler(ec);
        return;
    }

    using namespace system;
    constexpr auto size = p2ps::stream::detection_size;

    detection_.commit(size);
    const auto& context = std::get<cref<p2ps::context>>(context_).get();
    const auto data = pointer_cast<const uint8_t>(detection_.data().data());
    const std::span<const uint8_t> prefix{ data, size };

    // A v1 peer is served without upgrade, the buffer retains the prefix.
    if (p2ps::stream::detected_v1(prefix, context.identifier))
    {
        handler(error::success);
        return;
    }

    // The prefix is consumed by the upgrade (a partial peer key).
    data_chunk key{ prefix.begin(), prefix.end() };
    detection_.consume(size);

    // Extract to temporary to avoid dangling reference after destruction.
    auto socket = std::move(get_base());

    // P2PS (bip324) context is applied to the socket.
    socket_.emplace<p2ps::stream>(std::move(socket), context);

    // Posts handler to socket strand.
    get_p2ps().async_handshake(std::move(key),
        std::bind(&socket::handle_encrypted_handshake,
            shared_from_this(), _1, handler));
}

void socket::handle_encrypted_handshake(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec == error::operation_canceled)
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec == error::unknown) logx("encrypted handshake", ec);
    handler(ec);
}

void socket::handle_publisher_handshake(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec == error::operation_canceled)
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec == error::unknown) logx("publisher handshake", ec);
    handler(ec);
}

void socket::handle_handshake(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec == error::operation_canceled)
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec == error::unknown) logx("handshake", ec);
    handler(ec);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
