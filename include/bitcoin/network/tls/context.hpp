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
#ifndef LIBBITCOIN_NETWORK_TLS_CONTEXT_HPP
#define LIBBITCOIN_NETWORK_TLS_CONTEXT_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// Credentials and peer verification, shared by server or client connections.
/// Not thread safe during configuration.
class BCT_API context final
{
public:
    DELETE_COPY_MOVE(context);

    typedef std::vector<system::data_chunk> chain;
    typedef std::vector<uint16_t> codes;

    /// Client hello suites, groups, key share groups, schemes, session id size.
    struct offer
    {
        codes suites
        {
            aes_128_gcm_sha256,
            aes_256_gcm_sha384,
            chacha20_poly1305_sha256
        };

        codes groups
        {
            x25519_group,
            secp256r1_group,
            secp384r1_group
        };

        codes shares{ x25519_group };

        codes algorithms
        {
            ecdsa_secp256r1_sha256,
            ecdsa_secp384r1_sha384,
            rsa_pss_rsae_sha256,
            rsa_pss_rsae_sha384,
            rsa_pss_rsae_sha512
        };

        size_t session{ 32 };
    };

    context() NOEXCEPT;
    ~context() NOEXCEPT;

    /// Set chain (leaf first, P-256/P-384) from "CERTIFICATE" blocks, rest
    /// unparsed, false if malformed or key mismatch.
    bool set_chain(const std::string& text) NOEXCEPT;

    /// Set P-256/P-384 key from block (encrypted if password), false if
    /// malformed or chain mismatch.
    bool set_key(const std::string& text,
        const std::string& password) NOEXCEPT;

    /// Add verification anchors from "CERTIFICATE" blocks.
    bool add_anchors(const std::string& text) NOEXCEPT;

    /// Peer verification (request and require certificate).
    void set_verify(bool request, bool require) NOEXCEPT;

    /// Fix verification time (unix seconds), zero for current time.
    void set_time(uint64_t seconds) NOEXCEPT;

    /// Client offer.
    void set_offer(const offer& value) NOEXCEPT;

    /// Server name sent by client (sni), none if empty.
    void set_name(const std::string& name) NOEXCEPT;

    /// Client accepts any server certificate, bound by caller (e.g. Tor CERTS).
    void set_unanchored(bool value) NOEXCEPT;

    /// True if chain and key set (server).
    bool is_ready() const NOEXCEPT;

    /// True if anchored or unanchored, and any chain has its key.
    bool is_client_ready() const NOEXCEPT;

    const chain& certificates() const NOEXCEPT;

    /// Private key curve and key of that curve.
    system::x509::curve curve() const NOEXCEPT;
    const system::x509::secret& key() const NOEXCEPT;
    const system::x509::secret384& key384() const NOEXCEPT;

    const system::x509::certificates& anchors() const NOEXCEPT;
    bool request() const NOEXCEPT;
    bool require() const NOEXCEPT;
    uint64_t time() const NOEXCEPT;
    const offer& offered() const NOEXCEPT;
    const std::string& name() const NOEXCEPT;
    bool unanchored() const NOEXCEPT;

private:
    bool matches() const NOEXCEPT;

    chain chain_{};
    system::x509::secret key_{};
    system::x509::secret384 key384_{};
    system::x509::curve curve_{};
    system::data_chunk public_key_{};
    system::x509::certificates anchors_{};
    offer offer_{};
    std::string name_{};
    bool has_key_{};
    uint64_t time_{};
    bool request_{};
    bool require_{};
    bool unanchored_{};
};

} // namespace tls

/// Name and version of the tls library.
BCT_API std::string_view tls_library() NOEXCEPT;

} // namespace network
} // namespace libbitcoin

#endif
