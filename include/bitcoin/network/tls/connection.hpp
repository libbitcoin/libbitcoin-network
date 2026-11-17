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
#ifndef LIBBITCOIN_NETWORK_TLS_CONNECTION_HPP
#define LIBBITCOIN_NETWORK_TLS_CONNECTION_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/context.hpp>
#include <bitcoin/network/tls/record.hpp>
#include <bitcoin/network/tls/schedule.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// One TLS 1.3 connection (rfc8446): the record layer, alerts, handshake
/// framing, application data and key updates, under the handshake of the
/// client or server. Transport bytes are consumed by receive and produced into
/// output. Not thread safe.
class BCT_API connection
{
public:
    DELETE_COPY_MOVE(connection);

    /// Consume received transport bytes. False once failed, when output holds
    /// any alert to be sent, or once the peer has sent close_notify.
    bool receive(const const_byte_span& data) NOEXCEPT;

    /// Transport bytes to be sent (the caller consumes them).
    system::data_chunk& output() NOEXCEPT;

    /// Protect application data into output (once established).
    bool write(const const_byte_span& data) NOEXCEPT;

    /// Move up to out.size() received application bytes into out.
    size_t read(const byte_span& out) NOEXCEPT;

    /// Received application bytes not yet read.
    size_t readable() const NOEXCEPT;

    /// Queue a key update into output, optionally requesting the peer's.
    bool update(bool request) NOEXCEPT;

    /// Queue close_notify into output (once).
    void close() NOEXCEPT;

    /// The handshake is complete (peer finished verified).
    bool is_established() const NOEXCEPT;

    /// The peer sent close_notify.
    bool is_closed() const NOEXCEPT;

    /// This side sent close_notify.
    bool is_close_sent() const NOEXCEPT;

    /// The connection failed, with the alert sent or received.
    bool is_failed() const NOEXCEPT;
    uint8_t failure() const NOEXCEPT;
    bool is_failure_received() const NOEXCEPT;

    /// Negotiated cipher suite and key exchange group.
    uint16_t suite() const NOEXCEPT;
    uint16_t group() const NOEXCEPT;

    /// Peer certificate chain (empty if not authenticated).
    const system::x509::certificates& peer() const NOEXCEPT;

protected:
    typedef const_byte_span span;

    /// The context must outlive the connection.
    connection(const context& context, bool server) NOEXCEPT;
    virtual ~connection() NOEXCEPT;

    /// Handshake messages.
    /// -----------------------------------------------------------------------

    static system::data_chunk make_message(uint8_t type,
        const system::data_chunk& body) NOEXCEPT;
    static system::data_chunk make_certificate(
        const context::chain& chain) NOEXCEPT;
    static bool parse_certificate(system::x509::certificates& out,
        uint8_t& alert, const span& body) NOEXCEPT;
    static uint8_t to_alert(const code& ec) NOEXCEPT;

    /// CertificateVerify by the context key, of the transcript (4.4.3).
    system::data_chunk make_verify(std::string_view label) const NOEXCEPT;

    /// True if the leaf signed the transcript by the scheme (4.4.3).
    bool is_verified(const system::x509::certificate& leaf, uint16_t scheme,
        const span& signature, std::string_view label) const NOEXCEPT;

    /// Handshake of the role, from the received handshake message.
    virtual bool handle_message(uint8_t type, const span& message,
        const span& body) NOEXCEPT = 0;

    /// True if a compatibility change_cipher_spec may be received (5).
    virtual bool is_handshaking() const NOEXCEPT = 0;

    /// Records and keys.
    /// -----------------------------------------------------------------------

    bool fail(uint8_t alert) NOEXCEPT;
    void establish() NOEXCEPT;
    void seal(uint8_t type, const span& data) NOEXCEPT;
    void send(uint8_t type, const span& data) NOEXCEPT;
    void set_send(const schedule::secret& traffic) NOEXCEPT;
    void set_receive(const schedule::secret& traffic) NOEXCEPT;
    bool handle_key_update(const span& body) NOEXCEPT;
    void add_transcript(const span& message) NOEXCEPT;
    schedule::secret transcript() const NOEXCEPT;

    /// Application traffic secrets (7.1), from the handshake secret.
    void derive_traffic() NOEXCEPT;

    // Configuration.
    const context& context_;

    // Negotiation.
    uint16_t suite_{};
    uint16_t group_{};
    system::x509::certificates peer_{};
    tls::transcript transcript_{};

    // Keys.
    schedule::secret handshake_secret_{};
    schedule::secret client_handshake_{};
    schedule::secret server_handshake_{};
    schedule::secret client_traffic_{};
    schedule::secret server_traffic_{};

private:
    bool handle_record(uint8_t type, const span& header,
        const span& fragment) NOEXCEPT;
    bool handle_content(uint8_t type, const span& content) NOEXCEPT;
    bool handle_alert(const span& content) NOEXCEPT;
    bool handle_handshake(const span& content) NOEXCEPT;
    void send_update(bool request) NOEXCEPT;
    schedule::secret& send_traffic() NOEXCEPT;
    schedule::secret& receive_traffic() NOEXCEPT;

    // State.
    const bool server_;
    uint8_t failure_{};
    bool failed_{};
    bool failure_received_{};
    bool established_{};
    bool closed_{};
    bool close_sent_{};
    size_t receive_epoch_{};

    // Buffers.
    system::data_chunk input_{};
    system::data_chunk handshake_{};
    system::data_chunk application_{};
    system::data_chunk output_{};

    // Records.
    record send_{};
    record receive_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
