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

namespace libbitcoin {
namespace network {
namespace tls {

/// Server credentials and peer verification configuration, shared by the
/// connections of a server. Not thread safe during configuration.
class BCT_API context final
{
public:
    DELETE_COPY_MOVE(context);

    typedef std::vector<system::data_chunk> chain;

    context() NOEXCEPT;
    ~context() NOEXCEPT;

    /// Set the certificate chain (leaf first) from "CERTIFICATE" blocks, of a
    /// P-256 or P-384 leaf. False if malformed or the key does not match.
    bool set_chain(const std::string& text) NOEXCEPT;

    /// Set the P-256 or P-384 private key from a key block (encrypted if
    /// password). False if malformed or the key does not match the chain.
    bool set_key(const std::string& text,
        const std::string& password) NOEXCEPT;

    /// Add verification anchors from "CERTIFICATE" blocks.
    bool add_anchors(const std::string& text) NOEXCEPT;

    /// Peer verification (request a certificate, and require it).
    void set_verify(bool request, bool require) NOEXCEPT;

    /// Fix the verification time (unix seconds), zero for the current time.
    void set_time(uint64_t seconds) NOEXCEPT;

    /// True if the chain and key are set.
    bool is_ready() const NOEXCEPT;

    const chain& certificates() const NOEXCEPT;

    /// The curve of the private key, and the key of that curve.
    system::x509::curve curve() const NOEXCEPT;
    const system::x509::secret& key() const NOEXCEPT;
    const system::x509::secret384& key384() const NOEXCEPT;

    const system::x509::certificates& anchors() const NOEXCEPT;
    bool request() const NOEXCEPT;
    bool require() const NOEXCEPT;
    uint64_t time() const NOEXCEPT;

private:
    bool matches() const NOEXCEPT;

    chain chain_{};
    system::x509::secret key_{};
    system::x509::secret384 key384_{};
    system::x509::curve curve_{};
    system::data_chunk public_key_{};
    system::x509::certificates anchors_{};
    bool has_key_{};
    uint64_t time_{};
    bool request_{};
    bool require_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
