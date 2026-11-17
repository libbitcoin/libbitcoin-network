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
#ifndef LIBBITCOIN_NETWORK_TLS_SERVER_HPP
#define LIBBITCOIN_NETWORK_TLS_SERVER_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/connection.hpp>
#include <bitcoin/network/tls/context.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// The server side of one TLS 1.3 connection (rfc8446): x25519, P-256 or P-384
/// key exchange, ECDSA P-256 or P-384 server authentication, optional client
/// authentication (P-256 or P-384), AES-128-GCM, AES-256-GCM or
/// ChaCha20-Poly1305, without pre-shared keys or early data. Not thread safe.
class BCT_API server final
  : public connection
{
public:
    DELETE_COPY_MOVE(server);

    typedef system::x25519::key key;
    typedef system::data_array<32> random;

    /// The context must outlive the server.
    server(const context& context) NOEXCEPT;

    /// Fixed server random and x25519 ephemeral secret (test vectors).
    server(const context& context, const random& random,
        const key& ephemeral) NOEXCEPT;
    ~server() NOEXCEPT override;

protected:
    bool handle_message(uint8_t type, const span& message,
        const span& body) NOEXCEPT override;
    bool is_handshaking() const NOEXCEPT override;

private:
    enum class state
    {
        client_hello,
        retry_hello,
        client_certificate,
        client_certificate_verify,
        client_finished
    };

    bool handle_client_hello(const span& message, const span& body) NOEXCEPT;
    bool handle_certificate(const span& message, const span& body) NOEXCEPT;
    bool handle_certificate_verify(const span& message,
        const span& body) NOEXCEPT;
    bool handle_finished(const span& message, const span& body) NOEXCEPT;
    void send_retry(const span& client_hello, const span& session) NOEXCEPT;
    void send_hello(const random& value, const span& session,
        const span& key_share) NOEXCEPT;
    void send_flight() NOEXCEPT;
    void send_compatibility(const span& session) NOEXCEPT;
    bool exchange(system::data_chunk& shared, system::data_chunk& own,
        const span& share) NOEXCEPT;

    // Configuration.
    const random random_;
    key secret_;

    // Handshake state.
    state state_{ state::client_hello };
    bool compatibility_sent_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
