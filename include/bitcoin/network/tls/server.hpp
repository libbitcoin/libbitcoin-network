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
#include <bitcoin/network/tls/context.hpp>
#include <bitcoin/network/tls/record.hpp>
#include <bitcoin/network/tls/schedule.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// The server side of one TLS 1.3 connection (rfc8446): x25519, ECDSA P-256
/// server authentication, optional client authentication (P-256 or P-384),
/// AES-128-GCM or ChaCha20-Poly1305, without pre-shared keys or early data.
/// Transport bytes are consumed by receive and produced into output. Not
/// thread safe.
class BCT_API server final
{
public:
    DELETE_COPY_MOVE(server);

    typedef system::x25519::key key;
    typedef system::data_array<32> random;

    /// The context must outlive the server.
    server(const context& context) NOEXCEPT;

    /// Fixed server random and ephemeral secret (test vectors).
    server(const context& context, const random& random,
        const key& ephemeral) NOEXCEPT;

    /// Consume received transport bytes. False once failed, when output holds
    /// any alert to be sent.
    bool receive(const const_byte_span& data) NOEXCEPT;

    /// Transport bytes to be sent (the caller consumes them).
    system::data_chunk& output() NOEXCEPT;

    /// Protect application data into output (once established).
    bool write(const const_byte_span& data) NOEXCEPT;

    /// Move up to out.size() received application bytes into out.
    size_t read(const byte_span& out) NOEXCEPT;

    /// Received application bytes not yet read.
    size_t readable() const NOEXCEPT;

    /// Queue close_notify into output (once).
    void close() NOEXCEPT;

    /// The handshake is complete (client finished verified).
    bool is_established() const NOEXCEPT;

    /// The peer sent close_notify.
    bool is_closed() const NOEXCEPT;

    /// This side sent close_notify.
    bool is_close_sent() const NOEXCEPT;

    /// The connection failed, with the alert sent or received.
    bool is_failed() const NOEXCEPT;
    uint8_t failure() const NOEXCEPT;
    bool is_failure_received() const NOEXCEPT;

    /// Negotiated cipher suite (once the client hello is accepted).
    uint16_t suite() const NOEXCEPT;

    /// Verified client certificate chain (empty if not authenticated).
    const system::x509::certificates& peer() const NOEXCEPT;

private:
    enum class state
    {
        client_hello,
        retry_hello,
        client_certificate,
        client_certificate_verify,
        client_finished,
        established,
        failed
    };

    typedef const_byte_span span;

    bool fail(uint8_t alert) NOEXCEPT;
    bool handle_record(uint8_t type, const span& header,
        const span& fragment) NOEXCEPT;
    bool handle_content(uint8_t type, const span& content) NOEXCEPT;
    bool handle_alert(const span& content) NOEXCEPT;
    bool handle_handshake(const span& content) NOEXCEPT;
    bool handle_message(uint8_t type, const span& message,
        const span& body) NOEXCEPT;
    bool handle_client_hello(const span& message, const span& body) NOEXCEPT;
    bool handle_certificate(const span& message, const span& body) NOEXCEPT;
    bool handle_certificate_verify(const span& message,
        const span& body) NOEXCEPT;
    bool handle_finished(const span& message, const span& body) NOEXCEPT;
    bool handle_key_update(const span& body) NOEXCEPT;

    void send_retry(const span& client_hello, const span& session) NOEXCEPT;
    void send_hello(const random& value, const span& session,
        const span& key_share) NOEXCEPT;
    void send_flight() NOEXCEPT;
    void send_compatibility(const span& session) NOEXCEPT;
    void send(uint8_t type, const span& data) NOEXCEPT;
    void send_update() NOEXCEPT;
    void set_receive(const schedule::secret& traffic) NOEXCEPT;
    void add_transcript(const span& message) NOEXCEPT;
    schedule::secret transcript() NOEXCEPT;

    // Configuration.
    const context& context_;
    const random random_;
    const key secret_;

    // Connection state.
    state state_{ state::client_hello };
    uint16_t suite_{};
    uint8_t failure_{};
    bool failure_received_{};
    bool closed_{};
    bool close_sent_{};
    bool compatibility_sent_{};
    size_t receive_epoch_{};
    system::x509::certificates peer_{};

    // Buffers.
    system::data_chunk input_{};
    system::data_chunk handshake_{};
    system::data_chunk application_{};
    system::data_chunk output_{};

    // Keys.
    record send_{};
    record receive_{};
    system::accumulator<system::sha256> transcript_{};
    schedule::secret handshake_secret_{};
    schedule::secret client_handshake_{};
    schedule::secret server_handshake_{};
    schedule::secret client_traffic_{};
    schedule::secret server_traffic_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
