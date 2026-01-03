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
#ifndef LIBBITCOIN_NETWORK_NET_CONNECTOR_SOCKS_HPP
#define LIBBITCOIN_NETWORK_NET_CONNECTOR_SOCKS_HPP

#include <atomic>
#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, virtual.
/// Create outbound socket connections via a socks5 proxy.
/// All public/protected methods must be called from strand.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API connector_socks
  : public connector,
    protected tracker<connector_socks>
{
public:
    typedef std::shared_ptr<connector_socks> ptr;

    DELETE_COPY_MOVE_DESTRUCT(connector_socks);

    /// Resolves socks5 endpoint and stores address as member for each connect.
    connector_socks(const logger& log, asio::strand& strand,
        asio::io_context& service, const config::endpoint& socks5_proxy,
        const steady_clock::duration& timeout, size_t maximum_request,
        std::atomic_bool& suspended) NOEXCEPT;

protected:
    void start(const std::string& hostname, uint16_t port,
        const config::address& host, socket_handler&& handler) NOEXCEPT override;

    /// Connector overrides.
    void handle_connected(const code& ec, const finish_ptr& finish,
        socket::ptr socket) NOEXCEPT override;
    void handle_timer(const code& ec, const finish_ptr& finish,
        const socket::ptr& socket) NOEXCEPT override;

private:
    template <size_t Size>
    using data_ptr = std::shared_ptr<system::data_array<Size>>;
    template <size_t Size>
    using data_cptr = std::shared_ptr<const system::data_array<Size>>;

    // socks5 handshake
    void do_socks(const code& ec, const socket::ptr& socket) NOEXCEPT;
    void handle_socks_greeting_write(const code& ec, size_t size,
        const socket::ptr& socket, const data_cptr<3>& greeting) NOEXCEPT;
    void handle_socks_method_read(const code& ec, size_t size,
        const socket::ptr& socket, const data_ptr<2>& response) NOEXCEPT;
    void handle_socks_connect_write(const code& ec, size_t size,
        const socket::ptr& socket, const system::chunk_ptr& request) NOEXCEPT;
    void handle_socks_response_read(const code& ec, size_t size,
        const socket::ptr& socket, const data_ptr<4>& response) NOEXCEPT;
    void handle_socks_length_read(const code& ec, size_t size,
        const socket::ptr& socket, const data_ptr<1>& host_length) NOEXCEPT;
    void handle_socks_address_read(const code& ec, size_t size,
        const socket::ptr& socket, const system::chunk_ptr& address) NOEXCEPT;
    void do_socks_finish(const code& ec, const socket::ptr& socket) NOEXCEPT;
    void socks_finish(const code& ec, const socket::ptr& socket) NOEXCEPT;

    // This is protected by strand.
    const config::endpoint socks5_;
};

typedef std_vector<connector_socks::ptr> socks_connectors;
typedef std::shared_ptr<socks_connectors> socks_connectors_ptr;

} // namespace network
} // namespace libbitcoin

#endif
