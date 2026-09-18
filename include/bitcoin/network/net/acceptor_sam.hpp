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
#ifndef LIBBITCOIN_NETWORK_NET_ACCEPTOR_SAM_HPP
#define LIBBITCOIN_NETWORK_NET_ACCEPTOR_SAM_HPP

#include <atomic>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/acceptor.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, virtual.
/// Accept inbound socket connections forwarded by an i2p sam bridge.
/// All public/protected methods must be called from strand.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API acceptor_sam
  : public acceptor,
    protected tracker<acceptor_sam>
{
public:
    typedef std::shared_ptr<acceptor_sam> ptr;

    DELETE_COPY_MOVE_DESTRUCT(acceptor_sam);

    /// The bridge forwards each stream to a loopback listener.
    acceptor_sam(const logger& log, asio::strand& strand,
        asio::context& service, std::atomic_bool& suspended,
        parameters&& parameters, const settings::sam& sam) NOEXCEPT;

    /// Acceptor overrides (the local binding is ignored).
    code start(const config::authority& local) NOEXCEPT override;
    void stop() NOEXCEPT override;
    void accept(socket_handler&& handler) NOEXCEPT override;

protected:
    static code sam_result(const std::string& value) NOEXCEPT;

    /// Acceptor overrides.
    bool proxied() const NOEXCEPT override;

    /// Reads the peer destination line prefixed to each forwarded stream.
    void handle_accept(const code& ec, const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT override;

private:
    typedef std::shared_ptr<std::string> line_ptr;
    typedef std::shared_ptr<std::array<char, 1>> char_ptr;
    typedef std::function<void(const code&, const socket::ptr&,
        const line_ptr&)> line_handler;

    // sam line protocol
    void do_sam_request(const socket::ptr& socket,
        const std::string& request, line_handler&& handler) NOEXCEPT;
    void handle_sam_write(const code& ec, size_t size,
        const socket::ptr& socket, const line_ptr& request,
        const line_handler& handler) NOEXCEPT;
    void do_sam_read(const socket::ptr& socket, const line_ptr& line,
        const line_handler& handler) NOEXCEPT;
    void handle_sam_read(const code& ec, size_t size,
        const socket::ptr& socket, const char_ptr& byte,
        const line_ptr& line, const line_handler& handler) NOEXCEPT;

    // sam session (control socket)
    void handle_session_connect(const code& ec, const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT;
    void handle_session_hello(const code& ec, const socket::ptr& socket,
        const line_ptr& line, const socket_handler& handler) NOEXCEPT;
    void handle_session_create(const code& ec, const socket::ptr& socket,
        const line_ptr& line, bool transient,
        const socket_handler& handler) NOEXCEPT;
    void do_session_ready(const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT;

    // sam forward (control socket)
    void handle_forward_connect(const code& ec, const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT;
    void handle_forward_hello(const code& ec, const socket::ptr& socket,
        const line_ptr& line, uint16_t port,
        const socket_handler& handler) NOEXCEPT;
    void handle_forward_status(const code& ec, const socket::ptr& socket,
        const line_ptr& line, const socket_handler& handler) NOEXCEPT;
    void do_forward_ready(const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT;

    // sam teardown (either control socket closed by the bridge)
    void handle_close(const code& ec) NOEXCEPT;
    void do_close(const code& ec) NOEXCEPT;
    void do_teardown() NOEXCEPT;

    // sam accept (forwarded data socket)
    void handle_peer(const code& ec, const socket::ptr& socket,
        const line_ptr& line, const socket_handler& handler) NOEXCEPT;
    void handle_handshake(const code& ec, const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT;

    // key persistence
    bool save_key(const std::string& in) const NOEXCEPT;

    // This is thread safe.
    const settings::sam& sam_;

    // These are protected by strand.
    connector::ptr connector_{};
    socket::ptr session_{};
    socket::ptr forward_{};
    std::string id_{};
};

} // namespace network
} // namespace libbitcoin

#endif
