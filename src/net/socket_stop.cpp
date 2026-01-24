/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace std::placeholders;

// The socket does not (must not) stop itself.

// Stop (async).
// ----------------------------------------------------------------------------
// Hard stop (no graceful websocket/ssl closure).

void socket::stop() NOEXCEPT
{
    if (stopped_.load())
        return;

    // Stop flag accelerates work stoppage, as it does not wait on strand.
    stopped_.store(true);

    // Stop is posted to strand to protect the socket.
    boost::asio::post(strand_,
        std::bind(&socket::do_stop, shared_from_this()));
}

void socket::do_stop() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (is_websocket())
    {
        // Release ws control callback handler.
        VARIANT_DISPATCH_METHOD(get_ws(), control_callback());
    }

    boost_code ignore{};
    auto& layer = get_base();
    layer.shutdown(asio::socket::shutdown_both, ignore);
    layer.close(ignore);
}

// Stop (lazy).
// ----------------------------------------------------------------------------
// Lazy stop (graceful websocket/ssl closure).

void socket::lazy_stop() NOEXCEPT
{
    if (stopped_.load())
        return;

    // Stop flag accelerates work stoppage, as it does not wait on strand.
    stopped_.store(true);

    // Async stop is dispatched to strand to protect the socket.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_ws_stop, shared_from_this()));
}

void socket::do_ws_stop() NOEXCEPT
{
    BC_ASSERT(stranded());
    if (!is_websocket())
    {
        do_ssl_stop();
        return;
    }

    VARIANT_DISPATCH_METHOD(get_ws(),
        async_close(boost::beast::websocket::close_code::normal,
            std::bind(&socket::handle_ws_close,
                shared_from_this(), _1)));
}

void socket::handle_ws_close(const boost_code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (ec && ec != boost::asio::error::eof)
        logx("ws_close", ec);

    do_ssl_stop();
}

void socket::do_ssl_stop() NOEXCEPT
{
    BC_ASSERT(stranded());
    if (!is_secure())
    {
        do_stop();
        return;
    }

    get_ssl().async_shutdown(
        std::bind(&socket::handle_ssl_close,
            shared_from_this(), _1));
}

void socket::handle_ssl_close(const boost_code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (ec && ec != boost::asio::error::eof)
        logx("ssl_stop", ec);

    do_stop();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
