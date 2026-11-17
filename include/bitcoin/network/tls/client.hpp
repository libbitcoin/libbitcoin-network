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
#ifndef LIBBITCOIN_NETWORK_TLS_CLIENT_HPP
#define LIBBITCOIN_NETWORK_TLS_CLIENT_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/connection.hpp>
#include <bitcoin/network/tls/context.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// TLS 1.3 client connection (rfc8446), offer of context, no psk or early data.
/// Server authentication ECDSA P-256/P-384 or RSA-PSS (anchored or not),
/// optional client authentication (P-256/P-384). Sends client hello on
/// construction. Not thread safe.
class BCT_API client final
  : public connection
{
public:
    DELETE_COPY_MOVE(client);

    typedef system::x25519::key key;
    typedef system::data_array<32> random;

    /// Context must outlive client.
    client(const context& context) NOEXCEPT;

    /// Fixed client random and x25519 ephemeral secret (test vectors).
    client(const context& context, const random& random,
        const key& ephemeral) NOEXCEPT;
    ~client() NOEXCEPT override;

    /// Server sent HelloRetryRequest.
    bool is_retried() const NOEXCEPT;

    /// Server requested client certificate.
    bool is_requested() const NOEXCEPT;

protected:
    bool handle_message(uint8_t type, const span& message,
        const span& body) NOEXCEPT override;
    bool is_handshaking() const NOEXCEPT override;

private:
    typedef context::codes codes;

    enum class state
    {
        server_hello,
        encrypted_extensions,
        certificate,
        certificate_verify,
        finished
    };

    void send_hello() NOEXCEPT;
    void send_compatibility() NOEXCEPT;
    void send_flight() NOEXCEPT;
    bool handle_server_hello(const span& message, const span& body) NOEXCEPT;
    bool handle_retry(const span& message, bool keyed, uint16_t group,
        const span& cookie) NOEXCEPT;
    bool handle_encrypted_extensions(const span& message,
        const span& body) NOEXCEPT;
    bool handle_certificate_request(const span& message,
        const span& body) NOEXCEPT;
    bool handle_certificate(const span& message, const span& body) NOEXCEPT;
    bool handle_certificate_verify(const span& message,
        const span& body) NOEXCEPT;
    bool handle_finished(const span& message, const span& body) NOEXCEPT;
    system::data_chunk share(uint16_t group) NOEXCEPT;
    bool agree(system::data_chunk& shared, uint16_t group,
        const span& peer) const NOEXCEPT;

    // Configuration.
    const random random_;
    key secret_;
    system::data_chunk session_{};
    system::secp256r1::secret_t secret256_{};
    system::secp384r1::secret_t secret384_{};

    // Handshake state.
    codes shared_;
    state state_{ state::server_hello };
    system::data_chunk hello_{};
    system::data_chunk cookie_{};
    bool retried_{};
    bool requested_{};
    bool authenticate_{};
    bool compatibility_sent_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
