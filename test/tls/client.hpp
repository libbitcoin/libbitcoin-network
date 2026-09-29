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
#ifndef LIBBITCOIN_NETWORK_TEST_TLS_CLIENT_HPP
#define LIBBITCOIN_NETWORK_TEST_TLS_CLIENT_HPP

#include "../test.hpp"

namespace test {

/// Test harness TLS 1.3 client (in memory, not thread safe). Verifies the
/// server chain against anchors (if any) and its CertificateVerify, and
/// optionally authenticates with a P-256 chain and key.
class tls_client
{
public:
    struct options
    {
        std::vector<uint16_t> suites{ network::tls::aes_128_gcm_sha256,
            network::tls::chacha20_poly1305_sha256 };
        bool share{ true };
        size_t session{};
        system::x509::certificates anchors{};
        std::vector<system::data_chunk> chain{};
        system::x509::secret key{};
        uint64_t time{};
    };

    tls_client(const options& value) NOEXCEPT;

    /// Produce the client hello into output.
    void start() NOEXCEPT;

    /// Consume server bytes. False once failed.
    bool receive(const const_byte_span& data) NOEXCEPT;

    system::data_chunk& output() NOEXCEPT;
    bool write(const const_byte_span& data) NOEXCEPT;
    system::data_chunk read() NOEXCEPT;
    void update(bool request) NOEXCEPT;
    void close() NOEXCEPT;

    bool is_established() const NOEXCEPT;
    bool is_closed() const NOEXCEPT;
    bool is_failed() const NOEXCEPT;
    uint8_t failure() const NOEXCEPT;
    bool is_retried() const NOEXCEPT;
    bool is_requested() const NOEXCEPT;
    uint16_t suite() const NOEXCEPT;

private:
    typedef network::tls::schedule::secret secret;

    bool fail(uint8_t alert) NOEXCEPT;
    bool handle(uint8_t type, const const_byte_span& content) NOEXCEPT;
    bool handle_message(uint8_t type, const const_byte_span& message,
        const const_byte_span& body) NOEXCEPT;
    bool handle_hello(const const_byte_span& message,
        const const_byte_span& body) NOEXCEPT;
    bool handle_certificate(const const_byte_span& message,
        const const_byte_span& body) NOEXCEPT;
    bool handle_verify(const const_byte_span& message,
        const const_byte_span& body) NOEXCEPT;
    bool handle_finished(const const_byte_span& message,
        const const_byte_span& body) NOEXCEPT;
    void hello(bool retry) NOEXCEPT;
    void add(const const_byte_span& message) NOEXCEPT;
    secret hash() NOEXCEPT;

    enum class state { hello, encrypted, certificate, verify, finished, done };

    const options options_;
    system::x25519::key secret_{};
    system::x25519::key public_{};
    system::data_chunk session_{};
    state state_{ state::hello };
    uint16_t suite_{};
    uint8_t failure_{};
    bool failed_{};
    bool closed_{};
    bool retried_{};
    bool requested_{};
    system::data_chunk server_key_{};
    system::data_chunk input_{};
    system::data_chunk handshake_{};
    system::data_chunk application_{};
    system::data_chunk output_{};
    network::tls::record send_{};
    network::tls::record receive_{};
    system::accumulator<system::sha256> transcript_{};
    secret handshake_secret_{};
    secret client_handshake_{};
    secret server_handshake_{};
    secret client_traffic_{};
    secret server_traffic_{};
};

} // namespace test

#endif
