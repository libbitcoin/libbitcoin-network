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
#ifndef LIBBITCOIN_NETWORK_CONFIG_CREDENTIAL_HPP
#define LIBBITCOIN_NETWORK_CONFIG_CREDENTIAL_HPP

#include <iostream>
#include <memory>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Container for an http basic authorization credential and its methods.
/// The value is of the form: username:password[:method[,method]...] where an
/// empty method list implies all methods (password may not contain ':').
class BCT_API credential
{
public:
    typedef std::shared_ptr<credential> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(credential);

    credential() NOEXCEPT;
    credential(const std::string& value) THROWS;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The user name.
    const std::string& username() const NOEXCEPT;

    /// The password.
    const std::string& password() const NOEXCEPT;

    /// Digest of the implied http basic authorization header value.
    const system::hash_digest& digest() const NOEXCEPT;

    /// The permitted methods, empty implies all methods.
    const system::string_list& methods() const NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------

    /// True if methods is empty or contains the method.
    bool permitted(const std::string& method) const NOEXCEPT;

    /// Digest of the http basic authorization header implied by the values.
    static system::hash_digest to_digest(const std::string& username,
        const std::string& password) NOEXCEPT;

    /// The value is of the form: username:password[:method[,method]...].
    std::string to_string() const NOEXCEPT;

    /// Operators.
    /// -----------------------------------------------------------------------

    friend std::istream& operator>>(std::istream& input,
        credential& argument) THROWS;
    friend std::ostream& operator<<(std::ostream& output,
        const credential& argument) NOEXCEPT;

private:
    // These are not thread safe.
    std::string username_;
    std::string password_;
    system::string_list methods_;
    system::hash_digest digest_;
};

typedef std::vector<credential> credentials;

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
