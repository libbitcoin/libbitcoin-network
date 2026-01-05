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
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace std::placeholders;

constexpr auto port_size = sizeof(uint16_t);
constexpr auto length_size = sizeof(uint8_t);

enum socks : uint8_t
{
    // flags
    version = 0x05,
    connect = 0x01,
    reserved = 0x00,

    // reply type
    success = 0x00,
    failure = 0x01,
    disallowed = 0x02,
    net_unreachable = 0x03,
    host_unreachable = 0x04,
    connection_refused = 0x05,
    connection_expired = 0x06,
    unsupported_command = 0x07,
    unsupported_address = 0x08,

    // method (authentication) type
    method_none = 0xff,
    method_clear = 0x00,
    method_gssapi = 0x01,
    method_basic = 0x02,

    // method basic (subnegotiation)
    method_basic_version = 0x01,
    method_basic_success = 0x00,

    // command type
    command_connect = 0x01,
    command_bind = 0x02,
    command_udp = 0x03,

    // address type
    address_ipv4 = 0x01,
    address_fqdn = 0x03,
    address_ipv6 = 0x04
};

// static
code connector_socks::socks_response(uint8_t value) NOEXCEPT
{
    switch (value)
    {
        case socks::success: return error::success;
        case socks::disallowed: return error::socks_disallowed;
        case socks::net_unreachable: return error::socks_net_unreachable;
        case socks::host_unreachable: return error::socks_host_unreachable;
        case socks::connection_refused: return error::socks_connection_refused;
        case socks::connection_expired: return error::socks_connection_expired;
        case socks::unsupported_command: return error::socks_unsupported_command;
        case socks::unsupported_address: return error::socks_unsupported_address;
        default:
        case socks::failure: return error::socks_failure;
    };
}

// Caller can avoid proxied_ condition by using connector when not proxied.
connector_socks::connector_socks(const logger& log, asio::strand& strand,
    asio::io_context& service, const steady_clock::duration& timeout,
    size_t maximum_request, std::atomic_bool& suspended,
    const settings::socks5& socks) NOEXCEPT
  : connector(log, strand, service, timeout, maximum_request, suspended),
    socks5_(socks),
    proxied_(socks.proxied()),
    method_(socks.authenticated() ? socks::method_basic : socks::method_clear),
    tracker<connector_socks>(log)
{
}

// protected/override
bool connector_socks::proxied() const NOEXCEPT
{
    return proxied_;
}

// protected/override
void connector_socks::start(const std::string& hostname, uint16_t port,
    const config::address& host, socket_handler&& handler) NOEXCEPT
{
    if (proxied())
    {
        const auto& sox = socks5_.socks;
        connector::start(sox.host(), sox.port(), host, std::move(handler));
        return;
    }

    connector::start(hostname, port, host, std::move(handler));
}

// protected/override
void connector_socks::handle_connect(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (proxied())
    {
        do_socks_greeting_write(ec, finish, socket);
        return;
    }

    connector::handle_connect(ec, finish, socket);
}

// socks5 handshake (private)
// ----------------------------------------------------------------------------
// datatracker.ietf.org/doc/html/rfc1928 (socks5)
// datatracker.ietf.org/doc/html/rfc1929 (basic authentication)

void connector_socks::do_socks_greeting_write(const code& ec,
    const finish_ptr& finish, const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        socks_finish(result, finish, socket);
        return;
    }

    // +----+----------+----------+
    // |VER | NMETHODS | METHODS  |
    // +----+----------+----------+
    // | 1  |    1     | 1 to 255 |
    // +----+----------+----------+
    const auto greeting = to_shared<data_array<3>>(
    {
        socks::version, 1_u8, method_
    });

    // Start of socket strand sequence.
    // All socket operations are dispatched to its own strand, so this write
    // will be posted before invocation. This makes socket calls thread safe.
    socket->write({ greeting->data(), greeting->size() },
        std::bind(&connector_socks::handle_socks_greeting_write,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, greeting));
}

void connector_socks::handle_socks_greeting_write(const code& ec, size_t size,
    const finish_ptr& finish, const socket::ptr& socket,
    const data_cptr<3>& greeting) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    if (size != sizeof(*greeting))
    {
        do_socks_finish(error::operation_failed, finish, socket);
        return;
    }

    const auto response = emplace_shared<data_array<2>>();

    socket->read({ response->data(), response->size() },
        std::bind(&connector_socks::handle_socks_method_read,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, response));
}

void connector_socks::handle_socks_method_read(const code& ec, size_t size,
    const finish_ptr& finish, const socket::ptr& socket,
    const data_ptr<2>& response) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    // +----+--------+
    // |VER | METHOD |
    // +----+--------+
    // | 1  |   1    |
    // +----+--------+
    if (size != sizeof(*response) ||
        response->at(0) != socks::version ||
        response->at(1) != method_)
    {
        do_socks_finish(error::socks_method, finish, socket);
        return;
    }

    // Bypass authentication.
    if (method_ == socks::method_clear)
    {
        do_socks_connect_write(finish, socket);
        return;
    }

    const auto& username = socks5_.username;
    const auto username_length = username.size();

    // Socks5 limits valid username length to one byte.
    if (is_limited<uint8_t>(username_length))
    {
        do_socks_finish(error::socks_username, finish, socket);
        return;
    }

    const auto& password = socks5_.password;
    const auto password_length = password.size();

    // Socks5 limits valid  password length to one byte.
    if (is_limited<uint8_t>(password_length))
    {
        do_socks_finish(error::socks_password, finish, socket);
        return;
    }

    // +----+------+----------+------+----------+
    // |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
    // +----+------+----------+------+----------+
    // | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
    // +----+------+----------+------+----------+
    const auto length = 1u + 1u + username_length + 1u + password_length;
    const auto authenticator = emplace_shared<data_chunk>(length);
    auto it = authenticator->begin();
    *it++ = socks::method_basic_version;
    *it++ = narrow_cast<uint8_t>(username_length);
    it = std::copy(username.begin(), username.end(), it);
    *it++ = narrow_cast<uint8_t>(password_length);
    it = std::copy(password.begin(), password.end(), it);

    socket->write({ authenticator->data(), authenticator->size() },
        std::bind(&connector_socks::handle_socks_authentication_write,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, authenticator));
}

void connector_socks::handle_socks_authentication_write(const code& ec,
    size_t size, const finish_ptr& finish, const socket::ptr& socket,
    const chunk_ptr& authenticator) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    if (size != authenticator->size())
    {
        do_socks_finish(error::operation_failed, finish, socket);
        return;
    }

    const auto auth_res = emplace_shared<data_array<2>>();

    socket->read({ auth_res->data(), auth_res->size() },
        std::bind(&connector_socks::handle_socks_authentication_read,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, auth_res));
}

void connector_socks::handle_socks_authentication_read(const code& ec,
    size_t size, const finish_ptr& finish, const socket::ptr& socket,
    const data_ptr<2>& response) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    // +----+--------+
    // |VER | STATUS |
    // +----+--------+
    // | 1  |   1    |
    // +----+--------+
    if (size != sizeof(*response) ||
        response->at(0) != socks::method_basic_version ||
        response->at(1) != socks::method_basic_success)
    {
        do_socks_finish(error::socks_authentication, finish, socket);
        return;
    }

    do_socks_connect_write(finish, socket);
}

void connector_socks::do_socks_connect_write(const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    const auto port = to_big_endian(socket->address().port());
    const auto host = socket->address().to_host();
    const auto host_length = host.length();

    // Socks5 limits valid host lengths to one byte.
    if (is_limited<uint8_t>(host_length))
    {
        do_socks_finish(error::socks_server_name, finish, socket);
        return;
    }

    // +----+-----+-------+------+----------+----------+
    // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    // +----+-----+-------+------+----------+----------+
    // | 1  |  1  | X'00' |  1   | Variable |    2     |
    // +----+-----+-------+------+----------+----------+
    const auto length = 5u + host_length + port_size;
    const auto request = emplace_shared<data_chunk>(length);
    auto it = request->begin();
    *it++ = socks::version;
    *it++ = socks::command_connect;
    *it++ = socks::reserved;
    *it++ = socks::address_fqdn;
    *it++ = narrow_cast<uint8_t>(host_length);
    it = std::copy(host.begin(), host.end(), it);
    it = std::copy(port.begin(), port.end(), it);

    socket->write({ request->data(), request->size() },
        std::bind(&connector_socks::handle_socks_connect_write,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, request));
}

void connector_socks::handle_socks_connect_write(const code& ec, size_t size,
    const finish_ptr& finish, const socket::ptr& socket,
    const chunk_ptr& request) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    if (size != request->size())
    {
        do_socks_finish(error::operation_failed, finish, socket);
        return;
    }

    const auto response = emplace_shared<data_array<4>>();

    socket->read({ response->data(), response->size() },
        std::bind(&connector_socks::handle_socks_response_read,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, response));
}

void connector_socks::handle_socks_response_read(const code& ec, size_t size,
    const finish_ptr& finish, const socket::ptr& socket,
    const data_ptr<4>& response) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    // +----+-----+-------+------+
    // |VER | REP |  RSV  | ATYP |
    // +----+-----+-------+------+
    // | 1  |  1  | X'00' |  1   |
    // +----+-----+-------+------+
    if (size != sizeof(*response) ||
        response->at(0) != socks::version ||
        response->at(2) != socks::reserved)
    {
        do_socks_finish(error::socks_response_invalid, finish, socket);
        return;
    }

    // Map response code to error code.
    if (const auto code = socks_response(response->at(1)))
    {
        do_socks_finish(code, finish, socket);
        return;
    }

    switch (response->at(3))
    {
        case socks::address_ipv4:
        {
            // A version-4 IP address with length of 4 octets.
            const auto address = emplace_shared<data_chunk>(4u + port_size);

            socket->read({ address->data(), address->size() },
                std::bind(&connector_socks::handle_socks_address_read,
                    shared_from_base<connector_socks>(),
                        _1, _2, finish, socket, address));
            return;
        }
        case socks::address_ipv6:
        {
            // A version-6 IP address with length of 16 octets.
            const auto address = emplace_shared<data_chunk>(16u + port_size);

            socket->read({ address->data(), address->size() },
                std::bind(&connector_socks::handle_socks_address_read,
                    shared_from_base<connector_socks>(),
                        _1, _2, finish, socket, address));
            return;
        }
        case socks::address_fqdn:
        {
            // The address field contains a fully-qualified domain name. The
            // first octet of the address field contains the number of octets
            // of name that follow (and excludes two byte length of the port).
            const auto length = emplace_shared<data_array<1>>();

            socket->read({ length->data(), sizeof(uint8_t) },
                std::bind(&connector_socks::handle_socks_length_read,
                    shared_from_base<connector_socks>(),
                        _1, _2, finish, socket, length));
            return;
        }
        default:
        {
            // There are no other types defined.
            do_socks_finish(error::socks_response_invalid, finish, socket);
            return;
        }
    }
}

void connector_socks::handle_socks_length_read(const code& ec, size_t size,
    const finish_ptr& finish, const socket::ptr& socket,
    const data_ptr<1>& host_length) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    if (size != sizeof(*host_length))
    {
        do_socks_finish(error::socks_response_invalid, finish, socket);
        return;
    }

    const auto length = host_length->front() + port_size;
    const auto address = emplace_shared<data_chunk>(length);

    socket->read({ address->data(), address->size() },
        std::bind(&connector_socks::handle_socks_address_read,
            shared_from_base<connector_socks>(),
                _1, _2, finish, socket, address));
}

void connector_socks::handle_socks_address_read(const code& ec, size_t size,
    const finish_ptr& finish, const socket::ptr& socket,
    const chunk_ptr& address) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        do_socks_finish(result, finish, socket);
        return;
    }

    // +----------+----------+
    // | BND.ADDR | BND.PORT |
    // +----------+----------+
    // | Variable |    2     |
    // +----------+----------+
    if (size != address->size())
    {
        do_socks_finish(error::socks_response_invalid, finish, socket);
        return;
    }

    // The address:port is the local binding by the socks5 server (unused).
    // The outbound address_/authority_ members are set by connect().
    do_socks_finish(error::success, finish, socket);
}

void connector_socks::do_socks_finish(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    // End of socket strand sequence.
    boost::asio::post(strand_,
        std::bind(&connector_socks::socks_finish,
            shared_from_base<connector_socks>(), ec, finish, socket));
}

void connector_socks::socks_finish(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());
    connector::handle_connect(ec, finish, socket);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
