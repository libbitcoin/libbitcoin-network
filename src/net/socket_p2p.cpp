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
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace network::rpc;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// P2P Read.
// ----------------------------------------------------------------------------

void socket::p2p_read(const asio::mutable_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    // asio::mutable_buffer is essentially a data_slab.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_p2p_read,
            shared_from_this(), out, std::move(handler)));
}

void socket::do_p2p_read(const asio::mutable_buffer& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        VARIANT_DISPATCH_FUNCTION(boost::asio::async_read,
            get_tcp(), out,
                std::bind(&socket::handle_p2p,
                    shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

// P2P Write.
// ----------------------------------------------------------------------------

void socket::p2p_write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    // asio::const_buffer is essentially a data_slice.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_p2p_write,
            shared_from_this(), in, std::move(handler)));
}

void socket::do_p2p_write(const asio::const_buffer& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        VARIANT_DISPATCH_FUNCTION(boost::asio::async_write,
            get_tcp(), in,
                std::bind(&socket::handle_p2p,
                    shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

// P2P (both).
// ----------------------------------------------------------------------------

void socket::handle_p2p(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("p2p", ec);
    handler(code, size);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
