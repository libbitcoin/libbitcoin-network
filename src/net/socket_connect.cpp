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

#include <utility>
#include <variant>
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
// initializes the asio::acceptor with the network strand. So the call to
// acceptor.async_accept invokes its handler on that strand as well.

void socket::accept(asio::acceptor& acceptor,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(!get_base().is_open(),
        "accept on open socket");
    try
    {
        // Dispatches on the acceptor's strand (which should be network).
        // Cannot move handler due to catch block invocation.
        acceptor.async_accept(get_base(),
            std::bind(&socket::handle_accept,
                shared_from_this(), _1, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ accept: " << e.what());
        handler(error::accept_failed);
    }
}

void socket::handle_accept(boost_code ec,
    const result_handler& handler) NOEXCEPT
{
    // This is running in the acceptor (not socket) execution context.
    // socket_ and endpoint_ are not guarded here, see comments on accept.

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec)
    {
        const auto code = error::asio_to_error_code(ec);
        if (code == error::unknown) logx("accept", ec);
        handler(code);
        return;
    }

    endpoint_ = { get_base().remote_endpoint(ec) };
    address_ = endpoint_;

    if (ec)
    {
        const auto code = error::asio_to_error_code(ec);
        if (code == error::unknown) logx("remote", ec);
        handler(code);
        return;
    }

    // Not in socket strand.
    do_handshake(handler);
}

// Connect.
// ----------------------------------------------------------------------------

void socket::connect(const asio::endpoints& range,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&socket::do_connect,
            shared_from_this(), range, std::move(handler)));
}

void socket::do_connect(const asio::endpoints& range,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!websocket(), "socket is upgraded");
    BC_ASSERT_MSG(!get_base().is_open(),
        "connect on open socket");

    try
    {
        // Establishes a socket connection by trying each endpoint in sequence.
        boost::asio::async_connect(get_base(), range,
            std::bind(&socket::handle_connect,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_connect: " << e.what());
        handler(error::connect_failed);
    }
}

void socket::handle_connect(const boost_code& ec, const asio::endpoint& peer,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // For socks proxy, peer will be the server's local binding.
    if (!proxied_)
        endpoint_ = peer;

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    if (ec)
    {
        const auto code = error::asio_to_error_code(ec);
        if (code == error::unknown) logx("connect", ec);
        handler(code);
        return;
    }

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
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_handshake,
            shared_from_this(), std::move(handler)));
}

// Invokes handler on acceptor (network) or connector (socket) strand.
void socket::do_handshake(const result_handler& handler) NOEXCEPT
{
    ////BC_ASSERT(stranded());

    if (encrypted())
    {
        // The accepted peer is detected as v1 or v2 before upgrade.
        if (inbound_)
        {
            boost::asio::async_read(get_base(),
                detection_.prepare(privacy::stream::detection_size),
                std::bind(&socket::handle_detection,
                    shared_from_this(), _1, handler));
            return;
        }

        // Extract to temporary to avoid dangling reference after destruction.
        auto socket = std::move(get_base());

        // P2PS (bip324) context is applied to the socket.
        socket_.emplace<privacy::stream>(std::move(socket),
            std::get<cref<privacy::context>>(context_).get());

        // Posts handler to socket strand.
        get_p2ps().async_handshake(true,
            std::bind(&socket::handle_encrypted_handshake,
                shared_from_this(), _1, handler));
        return;
    }

    if (secure())
    {
        const auto direction = inbound_ ?
            boost::asio::ssl::stream_base::server :
            boost::asio::ssl::stream_base::client;

        // Extract to temporary to avoid dangling reference after destruction.
        auto socket = std::move(get_base());

        // TLS context is applied to the socket.
        socket_.emplace<asio::ssl::socket>(std::move(socket),
            std::get<ref<asio::ssl::context>>(context_));

        // Posts handler to socket strand.
        get_ssl().async_handshake(direction,
            std::bind(&socket::handle_handshake,
                shared_from_this(), _1, handler));
        return;
    }

    handler(error::success);
}

// Invoked on the socket strand (asio handler).
void socket::handle_detection(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        const auto code = error::asio_to_error_code(ec);
        if (code == error::unknown) logx("detection", ec);
        handler(code);
        return;
    }

    constexpr auto size = privacy::stream::detection_size;
    detection_.commit(size);
    const auto& context = std::get<cref<privacy::context>>(context_).get();
    const auto data = system::pointer_cast<const uint8_t>(detection_.data().data());
    const std::span<const uint8_t> prefix{ data, size };

    // A v1 peer is served without upgrade, the buffer retains the prefix.
    if (privacy::stream::detected_v1(prefix, context.identifier))
    {
        handler(error::success);
        return;
    }

    // The prefix is consumed by the upgrade (a partial peer key).
    system::data_chunk key{ prefix.begin(), prefix.end() };
    detection_.consume(size);

    // Extract to temporary to avoid dangling reference after destruction.
    auto socket = std::move(get_base());

    // P2PS (bip324) context is applied to the socket.
    socket_.emplace<privacy::stream>(std::move(socket), context,
        std::move(key));

    // Posts handler to socket strand.
    get_p2ps().async_handshake(false,
        std::bind(&socket::handle_encrypted_handshake,
            shared_from_this(), _1, handler));
}

void socket::handle_encrypted_handshake(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("encrypted handshake", ec);
    handler(code);
}

void socket::handle_handshake(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

////    // Diagnostic block for backend-specific error retrieval.
////    // Boost maps detailed wolfssl errors to a generic error code.
////    if (ec)
////    {
////#ifdef HAVE_SSL
////        const auto handle = std::get<asio::ssl::socket>(socket_).native_handle();
////        char buffer[WOLFSSL_MAX_ERROR_SZ]{};
////        const auto error = ::wolfSSL_get_error(handle, 0);
////        ::wolfSSL_ERR_error_string_n(error, &buffer[0], sizeof(buffer));
////        LOGF("wolfSSL handshake error: code=" << error << ", desc='" << buffer << "'");
////#else
////        // OpenSSL recommends 120 bytes for strings.
////        char buffer[120]{};
////        const auto error = ::ERR_get_error();
////        ::ERR_error_string_n(error, &buffer[0], sizeof(buffer));
////        LOGF("OpenSSL handshake error: code=" << error << ", desc='" << buffer << "'");
////#endif
////    }

    const auto code = error::ssl_to_error_code(ec);
    if (code == error::unknown) logx("handshake", ec);
    handler(code);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
