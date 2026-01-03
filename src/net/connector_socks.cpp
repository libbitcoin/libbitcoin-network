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
#include <bitcoin/network/net/connector_socks.hpp>

#include <algorithm>
#include <atomic>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_ARRAY_INDEXING)

using namespace system;
using namespace std::placeholders;

connector_socks::connector_socks(const logger& log, asio::strand& strand,
    asio::io_context& service, const config::endpoint& socks5_proxy,
    const steady_clock::duration& timeout, size_t maximum_request,
    std::atomic_bool& suspended) NOEXCEPT
  : connector(log, strand, service, timeout, maximum_request, suspended),
    socks5_(socks5_proxy),
    tracker<connector_socks>(log)
{
}

// protected/override
void connector_socks::start(const std::string&, uint16_t,
    const config::address& host, socket_handler&& handler) NOEXCEPT
{
    // hostname and port are redundant with host for outgoing connections.
    connector::start(socks5_.host(), socks5_.port(), host, std::move(handler));
}

// protected/override
void connector_socks::handle_connected(const code& ec, const finish_ptr&,
    socket::ptr socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Perform socks5 handshake on the socket strand.
    do_socks(ec, socket);
}

// protected/override
void connector_socks::handle_timer(const code& ec, const finish_ptr&,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Connector strand cannot access the socket at this point.
    // Post to the socks connector strand to protect the socket.
    boost::asio::post(socket->strand(),
        std::bind(&connector_socks::do_socks_finish,
            shared_from_base<connector_socks>(), ec, socket));
}

// socks5 handshake
// ----------------------------------------------------------------------------
// private

// datatracker.ietf.org/doc/html/rfc1928
enum socks : uint8_t
{
    /// flags
    version = 0x05,
    connect = 0x01,
    reserved = 0x00,

    /// reply type
    success = 0x00,
    failure = 0x01,
    disallowed = 0x02,
    net_unreachable = 0x03,
    host_unreachable = 0x04,
    connection_refused = 0x05,
    connection_expired = 0x06,
    unsupported_command = 0x07,
    unsupported_address = 0x08,

    /// method (authentication) type
    method_none = 0xff,
    method_clear = 0x00,
    method_gssapi = 0x01,
    method_password = 0x02,

    /// command type
    command_connect = 0x01,
    command_bind = 0x02,
    command_udp = 0x03,

    /// address type
    address_ipv4 = 0x01,
    address_fqdn = 0x03,
    address_ipv6 = 0x04
};

void connector_socks::do_socks(const code& ec,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        socks_finish(result, socket);
        return;
    }

    // +----+----------+----------+
    // |VER | NMETHODS | METHODS  |
    // +----+----------+----------+
    // | 1  |    1     | 1 to 255 |
    // +----+----------+----------+
    const auto greeting = to_shared<data_array<3>>(
    {
        socks::version,
        1_u8,
        socks::method_clear
    });

    // Start of socket strand sequence.
    socket->write({ greeting->data(), greeting->size() },
        std::bind(&connector_socks::handle_socks_greeting_write,
            shared_from_base<connector_socks>(),
                _1, _2, socket, greeting));
}

void connector_socks::handle_socks_greeting_write(const code& ec, size_t size,
    const socket::ptr& socket, const data_cptr<3>& greeting) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, socket);
        return;
    }

    if (size != sizeof(*greeting))
    {
        do_socks_finish(error::connect_failed, socket);
        return;
    }

    const auto response = emplace_shared<data_array<2>>();

    socket->read({ response->data(), response->size() },
        std::bind(&connector_socks::handle_socks_method_read,
            shared_from_base<connector_socks>(),
                _1, _2, socket, response));
}

void connector_socks::handle_socks_method_read(const code& ec, size_t size,
    const socket::ptr& socket, const data_ptr<2>& response) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, socket);
        return;
    }

    const auto& in = *response;

    // +----+--------+
    // |VER | METHOD |
    // +----+--------+
    // | 1  |   1    |
    // +----+--------+
    if (size != sizeof(*response) ||
        in[0] != socks::version ||
        in[1] != socks::method_clear)
    {
        do_socks_finish(error::connect_failed, socket);
        return;
    }

    // +----+-----+-------+------+----------+----------+
    // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    // +----+-----+-------+------+----------+----------+
    // | 1  |  1  | X'00' |  1   | Variable |    2     |
    // +----+-----+-------+------+----------+----------+
    const auto host = socket->address().to_host();
    const auto port = to_big_endian(socket->address().port());
    const auto length = 5u + host.length() + sizeof(uint16_t);
    const auto request = emplace_shared<data_chunk>(length);
    auto& out = *request;
    out[0] = socks::version;
    out[1] = socks::command_connect;
    out[2] = socks::reserved;
    out[3] = socks::address_fqdn;
    out[4] = narrow_cast<uint8_t>(host.size());
    auto it = std::next(request->begin(), 5u);
    it = std::copy(host.begin(), host.end(), it);
    it = std::copy(port.begin(), port.end(), it);

    socket->write({ request->data(), request->size() },
        std::bind(&connector_socks::handle_socks_connect_write,
            shared_from_base<connector_socks>(),
                _1, _2, socket, request));
}

void connector_socks::handle_socks_connect_write(const code& ec, size_t size,
    const socket::ptr& socket, const system::chunk_ptr& request) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, socket);
        return;
    }

    if (size != request->size())
    {
        do_socks_finish(error::connect_failed, socket);
        return;
    }

    const auto response = emplace_shared<data_array<4>>();

    socket->read({ response->data(), response->size() },
        std::bind(&connector_socks::handle_socks_response_read,
            shared_from_base<connector_socks>(),
                _1, _2, socket, response));
}

void connector_socks::handle_socks_response_read(const code& ec, size_t size,
    const socket::ptr& socket, const data_ptr<4>& response) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, socket);
        return;
    }

    const auto& in = *response;

    // +----+-----+-------+------+
    // |VER | REP |  RSV  | ATYP |
    // +----+-----+-------+------+
    // | 1  |  1  | X'00' |  1   |
    // +----+-----+-------+------+
    if (size != sizeof(*response) ||
        in[0] != socks::version ||
        in[1] != socks::success ||
        in[2] != socks::reserved)
    {
        do_socks_finish(error::connect_failed, socket);
        return;
    }

    switch (in[3])
    {
        case socks::address_ipv4:
        {
            // A version-4 IP address with length of 4 octets.
            const auto address = emplace_shared<data_chunk>(4_size);

            socket->read({ address.get(), sizeof(uint8_t) },
                std::bind(&connector_socks::handle_socks_address_read,
                    shared_from_base<connector_socks>(),
                        _1, _2, socket, address));
            return;
        }
        case socks::address_ipv6:
        {
            // A version-6 IP address with length of 16 octets.
            const auto address = emplace_shared<data_chunk>(16_size);

            socket->read({ address->data(), address->size() },
                std::bind(&connector_socks::handle_socks_address_read,
                    shared_from_base<connector_socks>(),
                        _1, _2, socket, address));
            return;
        }
        case socks::address_fqdn:
        {
            // The address field contains a fully-qualified domain name. The
            // first octet of the address field contains the number of octets
            // of name that follow (and excludes two byte length of the port).
            const auto length = emplace_shared<data_array<1>>();

            socket->read({ length.get(), sizeof(uint8_t) },
                std::bind(&connector_socks::handle_socks_length_read,
                    shared_from_base<connector_socks>(),
                        _1, _2, socket, length));
            return;
        }
        default:
        {
            do_socks_finish(error::operation_failed, socket);
            return;
        }
    }
}

void connector_socks::handle_socks_length_read(const code& ec, size_t size,
    const socket::ptr& socket, const data_ptr<1>& host_length) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, socket);
        return;
    }

    if (size != sizeof(*host_length))
    {
        do_socks_finish(error::connect_failed, socket);
        return;
    }

    const auto bytes = host_length->front() + sizeof(uint16_t);
    const auto address = emplace_shared<data_chunk>(bytes);

    socket->read({ address->data(), address->size() },
        std::bind(&connector_socks::handle_socks_address_read,
            shared_from_base<connector_socks>(),
                _1, _2, socket, address));
}

void connector_socks::handle_socks_address_read(const code& ec, size_t size,
    const socket::ptr& socket, const chunk_ptr& address) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, socket);
        return;
    }

    // +----------+----------+
    // | BND.ADDR | BND.PORT |
    // +----------+----------+
    // | Variable |    2     |
    // +----------+----------+
    if (size != address->size())
    {
        do_socks_finish(error::connect_failed, socket);
        return;
    }

    // address:port isn't used (could convert to config::address, add setter),
    // but the outbound address_/authority_ members are set by connect().
    ////socket->set_address(address);

    do_socks_finish(error::success, socket);
}

void connector_socks::do_socks_finish(const code& ec,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    // Either socks operation error or operation_canceled from connector timer.
    // The socket is stopped here in case this is invoked by the timer (race).
    // That causes any above asynchronous operation to cancel and post handler.
    if (ec)
        socket->stop();

    // End of socket strand sequence.
    boost::asio::post(strand_,
        std::bind(&connector_socks::socks_finish,
            shared_from_base<connector_socks>(), ec, socket));
}

void connector_socks::socks_finish(const code& ec,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Stops the timer in all cases, and stops/resets the socket if ec set.
    connector::handle_connected(ec, move_shared(false), socket);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
